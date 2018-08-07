
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

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

/*
 * Every time an event is registered by a process, these are stored
 * in the lists.  Registerer can specify an arbitrary user pointer
 * and a callback function to call when the event gets reported.
 * The user callback will be called with the event pointer and the
 * opaque parameter supplied at the time of registration.
 */
typedef struct event_notification_record_s {

    two_parameter_function_pointer ecbf;
    void *opaque_user_parameter;

} event_notification_record_t;

/*
 * determines whether the two event notification records 
 * passed in as params are 'considered' to be equal.  
 * They are so if both the function pointers and the opaque 
 * user parameters match.
 *
 * Returns 0 for equal, -ve or +ve value if considered less
 * than or greater than.
 */
static int
compare_enrs (void *venr1, void *venr2)
{
    int diff;
    event_notification_record_t *enr1 = (event_notification_record_t*) venr1;
    event_notification_record_t *enr2 = (event_notification_record_t*) venr2;

    diff = enr1->ecbf - enr2->ecbf;
    if (diff) return diff;
    return enr1->opaque_user_parameter - enr2->opaque_user_parameter;
}

/*
 * given the object type & event type, finds the correct
 * and relevant registration list that needs to be operated on.
 */
static linkedlist_t *
get_correct_list (event_manager_t *evrp,
        int object_type, int event_type,
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
                &evrp->all_types_object_registrants;
        } else if (is_an_attribute_event(event_type)) {
            return
                &evrp->all_types_attribute_registrants;
        }

        /* invalid event type */
        return NULL;
    }

    /*
     * if here, caller is interested in the correct event list for a
     * SPECIFIC object type, we need to get it from the dynamic arrays.
     * We have to extract the list from the dynamic array indexed by the
     * specified object type.  If such an entry does not exist, we must 
     * create a new list and return it so it is ready for future use.
     */
    if (is_an_object_event(event_type)) {
        dap = &evrp->specific_object_registrants;
    } else if (is_an_attribute_event(event_type)) {
        dap = &evrp->specific_attribute_registrants;
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
     *
     * Note that this list will hold all the registrants
     * which are interested in being notified about the 
     * specific event type for the specified object type.
     */
    if ((0 != rv) && create_if_missing) {

        list = MEM_MONITOR_ALLOC(evrp, sizeof(linkedlist_t));
        if (NULL == list) return NULL;
        rv = linkedlist_init(list, 0, compare_enrs, evrp->mem_mon_p);
        if (0 != rv) {
            MEM_MONITOR_FREE(evrp, list);
            return NULL;
        }

        /* ok list is created, so now store it in the dynamic array */
        rv = dynamic_array_insert(dap, object_type, list);
        if (0 != rv) {
            MEM_MONITOR_FREE(evrp, list);
            return NULL;
        }
    }

    return list;
}

static int
thread_unsafe_generic_register_function (event_manager_t *evrp,
        int object_type, int event_type,
        two_parameter_function_pointer ecbf, void *user_param,
        int register_it)
{
    int rv;
    event_notification_record_t enr, *enrp;
    void *found;
    dynamic_array_t *dap;
    linkedlist_t *list = 
        get_correct_list(evrp, object_type, event_type, register_it, &dap);

    /*
     * if we are registering, we must create a list in the appropriate
     * dynamic array and store the info in that list if not already there.
     */
    if (register_it) {

        if (list) {
            
            /* create the event notification record & fill it */
            enrp = MEM_MONITOR_ALLOC(evrp, sizeof(event_notification_record_t));
            if (NULL == enrp) return ENOMEM;
            enrp->ecbf = ecbf;
            enrp->opaque_user_parameter = user_param;

            /* add it to the list.  If was already there, no need to store again */
            rv = linkedlist_add_once(list, enrp, &found);
            if (found) {
                MEM_MONITOR_FREE(evrp, enrp);
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
                MEM_MONITOR_FREE(evrp, list);
            }
        }
    }

    return 0;
}


/*****************************************************************************/

PUBLIC int
event_manager_init (event_manager_t *evrp,
        int make_it_thread_safe,
        mem_monitor_t *parent_mem_monitor)
{
    int rv;

    MEM_MONITOR_SETUP(evrp);
    LOCK_SETUP(evrp);

    rv = linkedlist_init(&evrp->all_types_object_registrants, 
            0, compare_enrs, evrp->mem_mon_p);
    if (rv) {
        return rv;
    }

    rv = linkedlist_init(&evrp->all_types_attribute_registrants,
            0, compare_enrs, evrp->mem_mon_p);
    if (rv) {
        linkedlist_destroy(&evrp->all_types_object_registrants);
        return rv;
    }

    rv = dynamic_array_init(&evrp->specific_object_registrants,
            0, 3, evrp->mem_mon_p);
    if (rv) {
        linkedlist_destroy(&evrp->all_types_object_registrants);
        linkedlist_destroy(&evrp->all_types_attribute_registrants);
        return rv;
    }

    rv = dynamic_array_init(&evrp->specific_attribute_registrants,
            0, 3, evrp->mem_mon_p);
    if (rv) {
        linkedlist_destroy(&evrp->all_types_object_registrants);
        linkedlist_destroy(&evrp->all_types_attribute_registrants);
        dynamic_array_destroy(&evrp->specific_object_registrants);
        return rv;
    }

    WRITE_UNLOCK(evrp);
    return rv;
}

/*
 * The following set of functions register/deregister a process
 * to be notified of the type of event for the specified object
 * type.  If 'object_type' is ALL_OBJECT_TYPES, then the target
 * will be ALL object types.
 */
PUBLIC int
register_for_object_events (event_manager_t *evrp,
        int object_type,
        two_parameter_function_pointer ecbf, void *user_param)
{
    int rv;

    WRITE_LOCK(evrp);
    rv = thread_unsafe_generic_register_function(evrp, object_type,
            OBJECT_EVENTS, ecbf, user_param, 1);
    WRITE_UNLOCK(evrp);
    return rv;
}

PUBLIC void
un_register_from_object_events (event_manager_t *evrp,
        int object_type, process_address_t *pap)
{
    WRITE_LOCK(evrp);
    (void) thread_unsafe_generic_register_function(evrp, object_type, pap,
            OBJECT_EVENTS, 0);
    WRITE_UNLOCK(evrp);
}

/*
 * This is also similar to above but it will register for all attribute
 * events including attribute add, delete and all value changes.
 */
PUBLIC int
register_for_attribute_events (event_manager_t *evrp,
        int object_type, process_address_t *pap)
{
    int rv;

    WRITE_LOCK(evrp);
    rv = thread_unsafe_generic_register_function(evrp, object_type, pap,
            ATTRIBUTE_EVENTS, 1);
    WRITE_UNLOCK(evrp);
    return rv;
}

PUBLIC void
un_register_from_attribute_events (event_manager_t *evrp,
        int object_type, process_address_t *pap)
{
    WRITE_LOCK(evrp);
    (void) thread_unsafe_generic_register_function(evrp, object_type, pap,
            ATTRIBUTE_EVENTS, 1);
    WRITE_UNLOCK(evrp);
}

PUBLIC void
event_manager_destroy (event_manager_t *evrp)
{
}

#ifdef __cplusplus
} // extern C
#endif 






