
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol (gee.akyol@gmail.com, gee_akyol@yahoo.com)
** Copyright: Cihangir Metin Akyol, March 2016
**
** This code is developed by and belongs to Cihangir Metin Akyol.  
** It is NOT owned by any company or consortium.  It is the sole
** property and work of one individual.
**
** It can be used by ANYONE or ANY company for ANY purpose as long 
** as NO ownership and/or patent claims are made to it by such persons 
** or companies.
**
** It ALWAYS is and WILL remain the property of Cihangir Metin Akyol.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#include <stdio.h>
#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

/*
 * Every time an event is registered, these two items are stored
 * in the lists.  Registerer can specify an arbitrary user pointer
 * and a callback function to call when the event gets reported.
 * The user callback will be called with the event pointer and the
 * opaque parameter supplied at the time of registration.
 */
typedef struct event_registration_record_s {

    two_parameter_function_pointer ecbf;
    void *opaque_user_parameter;

} event_registration_record_t;

/*
 * determines whether the two event notification records 
 * passed in as void* params are 'considered' to be equal.  
 * They are so if their function pointers AND the opaque
 * user parameters are the same.
 *
 * Returns 0 for equal, -ve or +ve value if considered less
 * than or greater than.
 */
static int
compare_errs (void *errp1, void *errp2)
{
    int diff;

    diff =
        ((event_registration_record_t*) errp1)->ecbf -
        ((event_registration_record_t*) errp2)->ecbf;
    if (diff) return diff;
    return
        ((event_registration_record_t*) errp1)->opaque_user_parameter -
        ((event_registration_record_t*) errp2)->opaque_user_parameter;
}

/*
 * given the object type & event type, this function finds the correct
 * and relevant registration list that needs to be operated on.
 */
static linkedlist_t *
get_correct_list (event_manager_t *emp,
        int event_type, int object_type,
        int create_if_missing,
        dynamic_array_t **dynamic_array_returned)
{
    int rv;
    dynamic_array_t *dap;
    linkedlist_t *list;

    *dynamic_array_returned = NULL;

    /*
     * return "all types of objects" appropriate list
     */
    if (ALL_OBJECT_TYPES == object_type) {

        if (is_an_object_event(event_type)) {
            return
                &emp->all_types_object_registrants;
        } else if (is_an_attribute_event(event_type)) {
            return
                &emp->all_types_attribute_registrants;
        }

        /* invalid event type */
        return NULL;
    }

    /*
     * if here, caller is interested in the correct event list for a
     * SPECIFIC object type, we need to get it from the dynamic arrays.
     * We have to extract the list from the dynamic array indexed by the
     * specified object type.  If such an entry does not exist AND is 
     * requested to be created then we must create a new list and return 
     * it so it is ready for future use.
     */
    if (is_an_object_event(event_type)) {
        dap = &emp->specific_object_registrants;
    } else if (is_an_attribute_event(event_type)) {
        dap = &emp->specific_attribute_registrants;
    } else {
        /* invalid event type */
        return NULL;
    }

    *dynamic_array_returned = dap;

    /* get the list of registrants interested in events for this object type */
    rv = dynamic_array_get(dap, object_type, (void**) &list);

    /* 
     * if no such list exists, create and initialize it,
     * if creation is needed.
     */
    if ((0 != rv) && create_if_missing) {

        list = MEM_MONITOR_ALLOC(emp, sizeof(linkedlist_t));
        if (NULL == list) return NULL;
        rv = linkedlist_init(list, 0, compare_errs, emp->mem_mon_p);
        if (0 != rv) {
            MEM_MONITOR_FREE(emp, list);
            return NULL;
        }

        /* ok list is created, so now store it in the dynamic array */
        rv = dynamic_array_insert(dap, object_type, list);
        if (0 != rv) {
            MEM_MONITOR_FREE(emp, list);
            return NULL;
        }
    }

    return list;
}

static int
thread_unsafe_already_registered (event_manager_t *emp,
    int event_type, int object_type,
    two_parameter_function_pointer ecbf, void *opaque_user_parameter)
{
    dynamic_array_t *dap;
    event_registration_record_t errt;
    void *errtp_found;
    linkedlist_t *list = 
        get_correct_list(emp, event_type, object_type, 0, &dap);

    if (NULL == list) return 0;
    errt.ecbf = ecbf;
    errt.opaque_user_parameter = opaque_user_parameter;
    return
        0 == linkedlist_search(list, &errt, &errtp_found);
}

static int
thread_unsafe_generic_register_function (event_manager_t *emp,
        int event_type, int object_type,
        two_parameter_function_pointer ecbf, void *user_param,
        int register_it)
{
    int rv;
    event_registration_record_t enr, *errp;
    void *found;
    dynamic_array_t *dap;
    linkedlist_t *list;

    /* if we are traversing lists, block any potential changes */
    if (emp->cannot_be_modified) return EBUSY;

    list = get_correct_list(emp, event_type, object_type, register_it, &dap);

    /*
     * if we are registering, we must create a list in the appropriate
     * dynamic array and store the info in that list if not already there.
     */
    if (register_it) {

        if (list) {
            
            /* create the event notification record & fill it */
            errp = MEM_MONITOR_ALLOC(emp, sizeof(event_registration_record_t));
            if (NULL == errp) return ENOMEM;
            errp->ecbf = ecbf;
            errp->opaque_user_parameter = user_param;

            /* add it to the list.  If was already there, no need to store again */
            rv = linkedlist_add_once(list, errp, &found);
            if (found) {
                MEM_MONITOR_FREE(emp, errp);
                rv = 0;
            }

            /* return appropriate result */
            return rv;
        }

        /* this fails only if no memory was available */
        return ENOMEM;
    }

    /*
     * if we are here, we are UN registering.  In this case, if list
     * was not available in the first place, it is no big deal and we
     * should not unnecessarily create a list.  We do return ENODATA
     * but it actually satifies what was intended: to delete the registration.
     * Not having one at all in the first place is as good as deleting
     * it.  If it was indeed IN the list, we delete it from the list.
     * After the deletion from the list, if there are no elements left
     * in the list, we can also delete the list itself from the dynamic
     * array.
     */
    if (list) {

        /* delete it from the list */
        enr.ecbf = ecbf;
        enr.opaque_user_parameter = user_param;
        rv = linkedlist_delete(list, &enr, &found);

        /* if nothing else remains in the list, delete the list itself */
        if (dap && (list->n <= 0)) {
            rv = dynamic_array_remove(dap, object_type, &found);
            if (0 == rv) {
                assert(found == list);
                linkedlist_destroy(list);
                MEM_MONITOR_FREE(emp, list);
            }
        }
    }

    return 0;
}

static void
execute_all_callbacks (linkedlist_t *list, event_record_t *erp)
{
    event_registration_record_t *errp;

    if (list) {
        FOR_ALL_LINKEDLIST_ELEMENTS(list, errp) {
            errp->ecbf(erp, errp->opaque_user_parameter);
        }
    }
}

static void
thread_unsafe_announce_event (event_manager_t *emp, event_record_t *erp)
{
    linkedlist_t *list;
    dynamic_array_t *dummy;

    /*
     * starting to traverse links, block any changes which may occur
     * if any callback function attempts to perform any registrations
     * and/or changes to the lists.
     */
    emp->cannot_be_modified = 1;

    /*
     * First, notify the event to the registrants who registered
     * to receive events for ALL the object types.
     */
    list = get_correct_list(emp, ALL_OBJECT_TYPES, erp->event_type, 0, &dummy);
    execute_all_callbacks(list, erp);

    /*
     * Next, notify the event to the registrants who are registered
     * to receive events ONLY from this specific type of object.
     */
    list = get_correct_list(emp, erp->object_type, erp->event_type, 0, &dummy);
    execute_all_callbacks(list, erp);

    /* ok traversal complete, modifications to lists can now happen */
    emp->cannot_be_modified = 0;
}

/*****************************************************************************/

PUBLIC int
event_manager_init (event_manager_t *emp,
        int make_it_thread_safe,
        mem_monitor_t *parent_mem_monitor)
{
    int rv;

    MEM_MONITOR_SETUP(emp);
    LOCK_SETUP(emp);

    /* until initialisation is finished, dont allow registrations */
    emp->cannot_be_modified = 1;

    rv = linkedlist_init(&emp->all_types_object_registrants, 
            0, compare_errs, emp->mem_mon_p);
    if (rv) {
        return rv;
    }

    rv = linkedlist_init(&emp->all_types_attribute_registrants,
            0, compare_errs, emp->mem_mon_p);
    if (rv) {
        linkedlist_destroy(&emp->all_types_object_registrants);
        return rv;
    }

    rv = dynamic_array_init(&emp->specific_object_registrants,
            0, 3, emp->mem_mon_p);
    if (rv) {
        linkedlist_destroy(&emp->all_types_object_registrants);
        linkedlist_destroy(&emp->all_types_attribute_registrants);
        return rv;
    }

    rv = dynamic_array_init(&emp->specific_attribute_registrants,
            0, 3, emp->mem_mon_p);
    if (rv) {
        linkedlist_destroy(&emp->all_types_object_registrants);
        linkedlist_destroy(&emp->all_types_attribute_registrants);
        dynamic_array_destroy(&emp->specific_object_registrants);
        return rv;
    }

    /* ok we are cleared to use the object now */
    emp->cannot_be_modified = 0;

    WRITE_UNLOCK(emp);
    return rv;
}

PUBLIC int
already_registered (event_manager_t *emp,
    int event_type, int object_type,
    two_parameter_function_pointer ecbf, void *opaque_user_parameter)
{
    int rv;

    READ_LOCK(emp);
    rv = thread_unsafe_already_registered(emp, event_type, object_type,
            ecbf, opaque_user_parameter);
    READ_UNLOCK(emp);
    return rv;
}

/*
 * The following set of functions register/deregister a process
 * to be notified of the type of event for the specified object
 * type.  If 'object_type' is ALL_OBJECT_TYPES, then the target
 * will be ALL object types.
 */
PUBLIC int
register_for_object_events (event_manager_t *emp,
        int object_type,
        two_parameter_function_pointer ecbf, void *user_param)
{
    int rv;

    WRITE_LOCK(emp);
    rv = thread_unsafe_generic_register_function(emp, OBJECT_EVENTS,
            object_type, ecbf, user_param, 1);
    WRITE_UNLOCK(emp);
    return rv;
}

PUBLIC void
announce_event (event_manager_t *emp, event_record_t *erp)
{
    WRITE_LOCK(emp);
    thread_unsafe_announce_event(emp, erp);
    WRITE_UNLOCK(emp);
}

PUBLIC void
un_register_from_object_events (event_manager_t *emp,
        int object_type, two_parameter_function_pointer ecbf)
{
    WRITE_LOCK(emp);
    (void) thread_unsafe_generic_register_function(emp, OBJECT_EVENTS,
            object_type, ecbf, NULL, 0);
    WRITE_UNLOCK(emp);
}

/*
 * This is also similar to above but it will register for all attribute
 * events including attribute add, delete and all value changes.
 */
PUBLIC int
register_for_attribute_events (event_manager_t *emp,
        int object_type,
        two_parameter_function_pointer ecbf, void *user_param)
{
    int rv;

    WRITE_LOCK(emp);
    rv = thread_unsafe_generic_register_function(emp, ATTRIBUTE_EVENTS,
            object_type, ecbf, user_param, 1);
    WRITE_UNLOCK(emp);
    return rv;
}

PUBLIC void
un_register_from_attribute_events (event_manager_t *emp,
        int object_type, two_parameter_function_pointer ecbf)
{
    WRITE_LOCK(emp);
    (void) thread_unsafe_generic_register_function(emp, ATTRIBUTE_EVENTS,
            object_type, ecbf, NULL, 0);
    WRITE_UNLOCK(emp);
}

PUBLIC void
event_manager_destroy (event_manager_t *emp)
{
}

#ifdef __cplusplus
} // extern C
#endif 






