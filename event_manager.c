
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
 * in the appropriate lists.  Registerer can specify an arbitrary
 * user pointer and a callback function to call when the event gets
 * reported. The user callback will be called with the event pointer
 * and the opaque parameter supplied at the time of registration.
 * The name 'f_and_arg' stands for 'function and argument'.
 */
typedef struct f_and_arg_s {

    event_handler_t fptr;
    void *extra_arg;

} f_and_arg_t;

static f_and_arg_t *
create_f_and_arg (event_manager_t *emp,
        event_handler_t fptr, void *extra_arg)
{
    f_and_arg_t *new_one;

    new_one = MEM_MONITOR_ALLOC(emp, sizeof(f_and_arg_t));
    if (new_one) {
        new_one->fptr = fptr;
        new_one->extra_arg = extra_arg;
    }
    return new_one;
}

static int
compare_f_and_arg (void *p1, void *p2)
{
    int result;

    result = ((f_and_arg_t*) p1)->fptr - ((f_and_arg_t*) p2)->fptr;
    if (result) return result;
    return
        ((f_and_arg_t*) p1)->extra_arg - ((f_and_arg_t*) p2)->extra_arg;
}

static int
identical_f_and_arg (f_and_arg_t *fa1, f_and_arg_t *fa2)
{
    return
        (fa1->fptr == fa2->fptr) &&
        (fa1->extra_arg = fa2->extra_arg);
}

static void
destroy_f_and_arg (void *user_data, void *extra_arg)
{
    event_manager_t *emp = (event_manager_t*) extra_arg;

    MEM_MONITOR_FREE(emp, user_data);
}

/*
 * This is a structure accessed by the object type which stores a list
 * of the f_and_arg_s structures(function npointers and arguments).
 * It is stored in an index object and searched using the object type.
 */
typedef struct f_and_arg_container_s {

    int object_type;
    linkedlist_t list_of_f_and_arg;

} f_and_arg_container_t;

static int
compare_f_and_arg_container (void *p1, void *p2)
{
    return
        ((f_and_arg_container_t*) p1)->object_type -
        ((f_and_arg_container_t*) p2)->object_type;
}

static f_and_arg_container_t *
create_f_and_arg_container (event_manager_t *emp, int object_type)
{
    f_and_arg_container_t *new_one;
    int failed;

    new_one = MEM_MONITOR_ALLOC(emp, sizeof(f_and_arg_container_t));
    if (new_one) {
        new_one->object_type = object_type;
        failed = linkedlist_init(&new_one->list_of_f_and_arg, 0,
                    compare_f_and_arg, emp->mem_mon_p);
        if (failed) {
            MEM_MONITOR_FREE(emp, new_one);
            new_one = NULL;
        }
    }
    return new_one;
}

static void
destroy_f_and_arg_container (event_manager_t *emp,
    f_and_arg_container_t *farg_cont_p)
{
    linkedlist_destroy(&farg_cont_p->list_of_f_and_arg,
            destroy_f_and_arg, emp);
    MEM_MONITOR_FREE(emp, farg_cont_p);
}

/*
 * given the object type & event type, this function finds the correct
 * and relevant registration list that needs to be operated on.  It
 * also returns the container & the index object which that list was
 * in.  It is useful to have those, especially when we are about to
 * delete something.  Note that not all returned variables are available
 * in every context.  For example, when object type is 'any object',
 * both the container & index will be meaningless and hence will be
 * set to NULL.
 *
 * Note that all the pointer parameters to be returned may be passed
 * in as NULL if they are not needed.
 */
static int
get_correct_list (event_manager_t *emp,
        int event_type, int object_type,
        int create_if_missing,
        linkedlist_t **list_returned,
        f_and_arg_container_t **container_returned,
        index_obj_t **index_object_returned)
{
    int failed;
    index_obj_t *idxp;
    f_and_arg_container_t searched, *found, *new_one;

    safe_pointer_set(list_returned, NULL);
    safe_pointer_set(container_returned, NULL);
    safe_pointer_set(index_object_returned, NULL);

    /*
     * attribute events are usually more likely to occur statistically,
     * so check those first to have a hit more quickly
     */
    if (is_an_attribute_event(event_type)) {
        if (ALL_OBJECT_TYPES == object_type) {
            safe_pointer_set(list_returned,
                    &emp->attribute_event_registrants_for_all_objects);
            return 0;
        }
        idxp = &emp->attribute_event_registrants_for_one_object;
    } else if (is_an_object_event(event_type)) {
        if (ALL_OBJECT_TYPES == object_type) {
            safe_pointer_set(list_returned,
                    &emp->object_event_registrants_for_all_objects);
            return 0;
        }
        idxp = &emp->object_event_registrants_for_one_object;
    } else {
        return EINVAL;
    }

    /* Get the container, this is searched by the specific object type */
    searched.object_type = object_type;
    failed = index_obj_search(idxp, &searched, (void**) &found);
    if (found) {
        assert(!failed);
        safe_pointer_set(list_returned, &found->list_of_f_and_arg);
        safe_pointer_set(container_returned, found);
        safe_pointer_set(index_object_returned, idxp);
        return 0;
    }

    /* not found, create if requested */
    if (create_if_missing) {
        new_one = create_f_and_arg_container(emp, object_type);
        if (NULL == new_one) return ENOMEM;
        failed = index_obj_insert(idxp, new_one, (void**) &found);
        if (failed) {
            destroy_f_and_arg_container(emp, new_one);
            return failed;
        }
        safe_pointer_set(list_returned, &new_one->list_of_f_and_arg);
        safe_pointer_set(container_returned, new_one);
        safe_pointer_set(index_object_returned, idxp);
        return 0;
    }

    return ENODATA;
}

static int
thread_unsafe_already_registered (event_manager_t *emp,
    int event_type, int object_type,
    event_handler_t fptr, void *extra_arg)
{
    int failed;
    f_and_arg_t faat;
    linkedlist_t *list;
        
    failed = get_correct_list(emp, event_type, object_type, 0, &list, NULL, NULL);
    if (failed) return 0;
    assert(list);
    faat.fptr = fptr;
    faat.extra_arg = extra_arg;
    return (0 == linkedlist_search(list, &faat, NULL));
}

static int
thread_unsafe_generic_register_function (event_manager_t *emp,
        int event_type, int object_type,
        event_handler_t fptr, void *extra_arg,
        int register_it)
{
    int failed;
    f_and_arg_t fandarg, *fandargp;
    void *found;
    index_obj_t *idxp;
    f_and_arg_container_t *contp;
    linkedlist_t *list;

    /* if we are traversing lists, block any potential changes */
    if (emp->cannot_be_modified) return EBUSY;

    failed = get_correct_list(emp, event_type, object_type, register_it,
                &list, &contp, &idxp);

    /*
     * if we are registering, we must create a list in the appropriate
     * container and store the info in that list if not already there.
     */
    if (register_it) {

        /* during registration, missing list is always a failure */
        if (NULL == list) {
            assert(failed);
            return failed;
        }

        /* create new function & arg pair and store it in list */
        fandargp = MEM_MONITOR_ALLOC(emp, sizeof(f_and_arg_t));
        if (NULL == fandargp) return ENOMEM;
        fandargp->fptr = fptr;
        fandargp->extra_arg = extra_arg;

        /* uniquely add it to the list */
        failed = linkedlist_add_once(list, fandargp, NULL);
        if (failed) {
            MEM_MONITOR_FREE(emp, fandargp);
            return failed;
        }

        /* success */
        return 0;
    }

    /*
     * if we are here, we are UN registering.  In this case, if list
     * was not available in the first place, it is no big deal and we
     * should not unnecessarily create a list.  We do return ENODATA
     * but it actually satifies what was intended: to delete the registration.
     * Not having one at all in the first place is as good as deleting
     * it.  If it was indeed IN the list, we delete it from the list.
     * After the deletion from the list, if there are no elements left
     * in the list, we can also delete the container itself from the
     * index object.
     */
    if (list) {

        /* delete it from the list */
        fandarg.fptr = fptr;
        fandarg.extra_arg = extra_arg;
        linkedlist_delete(list, &fandarg, &found);

        /* if nothing else remains in the list, delete the container itself */
        if (idxp && (list->n <= 0)) {
            failed = index_obj_remove(idxp, 
            failed = dynamic_array_remove(idxp, object_type, &found);
            if (0 == failed) {
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
    f_and_arg_t *fandargp;

    if (list) {
        FOR_ALL_LINKEDLIST_ELEMENTS(list, fandargp) {
            fandargp->ehfp(erp, fandargp->extra_arg);
        }
    }
}

static void
thread_unsafe_announce_event (event_manager_t *emp, event_record_t *erp)
{
    linkedlist_t *list;
    index_obj_t *dummy;

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
    int failed;

    MEM_MONITOR_SETUP(emp);
    LOCK_SETUP(emp);

    /* until initialisation is finished, dont allow registrations */
    emp->cannot_be_modified = 1;

    failed = linkedlist_init(&emp->object_event_registrants_for_all_objects, 
            0, compare_errs, emp->mem_mon_p);
    if (failed) {
        return failed;
    }

    failed = linkedlist_init(&emp->attribute_event_registrants_for_all_objects,
            0, compare_errs, emp->mem_mon_p);
    if (failed) {
        linkedlist_destroy(&emp->object_event_registrants_for_all_objects);
        return failed;
    }

    failed = dynamic_array_init(&emp->object_event_registrants_for_one_object,
            0, 3, emp->mem_mon_p);
    if (failed) {
        linkedlist_destroy(&emp->object_event_registrants_for_all_objects);
        linkedlist_destroy(&emp->attribute_event_registrants_for_all_objects);
        return failed;
    }

    failed = dynamic_array_init(&emp->attribute_event_registrants_for_one_object,
            0, 3, emp->mem_mon_p);
    if (failed) {
        linkedlist_destroy(&emp->object_event_registrants_for_all_objects);
        linkedlist_destroy(&emp->attribute_event_registrants_for_all_objects);
        dynamic_array_destroy(&emp->object_event_registrants_for_one_object);
        return failed;
    }

    /* ok we are cleared to use the object now */
    emp->cannot_be_modified = 0;

    WRITE_UNLOCK(emp);
    return failed;
}

PUBLIC int
already_registered (event_manager_t *emp,
    int event_type, int object_type,
    event_handler_t ehfp, void *extra_arg)
{
    int failed;

    READ_LOCK(emp);
    failed = thread_unsafe_already_registered(emp, event_type, object_type,
            ehfp, extra_arg);
    READ_UNLOCK(emp);
    return failed;
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
        event_handler_t ehfp, void *extra_arg)
{
    int failed;

    WRITE_LOCK(emp);
    failed = thread_unsafe_generic_register_function(emp, OBJECT_EVENTS,
            object_type, ehfp, extra_arg, 1);
    WRITE_UNLOCK(emp);
    return failed;
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
        int object_type, event_handler_t ehfp)
{
    WRITE_LOCK(emp);
    (void) thread_unsafe_generic_register_function(emp, OBJECT_EVENTS,
            object_type, ehfp, NULL, 0);
    WRITE_UNLOCK(emp);
}

/*
 * This is also similar to above but it will register for all attribute
 * events including attribute add, delete and all value changes.
 */
PUBLIC int
register_for_attribute_events (event_manager_t *emp,
        int object_type,
        event_handler_t ehfp, void *extra_arg)
{
    int failed;

    WRITE_LOCK(emp);
    failed = thread_unsafe_generic_register_function(emp, ATTRIBUTE_EVENTS,
            object_type, ehfp, extra_arg, 1);
    WRITE_UNLOCK(emp);
    return failed;
}

PUBLIC void
un_register_from_attribute_events (event_manager_t *emp,
        int object_type, event_handler_t ehfp)
{
    WRITE_LOCK(emp);
    (void) thread_unsafe_generic_register_function(emp, ATTRIBUTE_EVENTS,
            object_type, ehfp, NULL, 0);
    WRITE_UNLOCK(emp);
}

PUBLIC void
event_manager_destroy (event_manager_t *emp)
{
}

#ifdef __cplusplus
} // extern C
#endif 






