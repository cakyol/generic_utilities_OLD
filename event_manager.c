
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

/***************************************************************************
 * Every time an event is registered, these two items are stored
 * in the appropriate lists.  Registerer can specify an arbitrary
 * user argument and a callback function to call when the event gets
 * reported. The user callback will be called with the event pointer
 * and the argument parameter supplied at the time of registration.
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

/* treat the structure as simply a sequence of bytes */
static int
compare_f_and_arg (void *p1, void *p2)
{
    int result;

    if (p1 == p2) return 0;
    result = ((f_and_arg_t*) p1)->fptr - ((f_and_arg_t*) p2)->fptr;
    if (result) return result;
    return
        ((f_and_arg_t*) p1)->extra_arg - ((f_and_arg_t*) p2)->extra_arg;
}

static void
destroy_f_and_arg_cb (void *user_data, void *extra_arg)
{
    event_manager_t *emp = (event_manager_t*) extra_arg;

    MEM_MONITOR_FREE(emp, user_data);
}

/***************************************************************************
 * This is a structure which contains a list of registered function
 * and argument pairs for a SPECIFIC object type.  It contains the
 * list of all functions to be invoked when an event matches its
 * object type.  The object type is used as a search index to quickly
 * get the list of those functions anmd arguments when an event 
 * happens.
 */

typedef struct f_and_arg_container_s {

    int object_type;
    ordered_list_t list_of_f_and_arg;
    index_obj_t *my_index;

} f_and_arg_container_t;

static int
compare_f_and_arg_container (void *p1, void *p2)
{
    return
        ((f_and_arg_container_t*) p1)->object_type -
        ((f_and_arg_container_t*) p2)->object_type;
}

/*
 * This function returns the searched container.
 * If it cannot be found AND 'create' is set, then
 * the container will be created and returned.
 */
static f_and_arg_container_t *
get_f_and_arg_container (event_manager_t *emp, int create,
    int object_type, index_obj_t *my_index)
{
    int failed;
    f_and_arg_container_t searched, *faargp;

    /* if already in the index, return it */
    searched.object_type = object_type;
    if (index_obj_search(my_index, &searched, (void**) &faargp) == 0) {
        assert(faargp);
        return faargp;
    }

    /*
     * if here, we cannot find it.  If also creation is NOT required,
     * nothing more to do, simply return NULL
     */
    if (!create) return NULL;

    /* it does not exist AND creation is needed */
    faargp = MEM_MONITOR_ALLOC(emp, sizeof(f_and_arg_container_t));
    if (faargp) {
        failed = ordered_list_init(&faargp->list_of_f_and_arg, 0,
                    compare_f_and_arg, emp->mem_mon_p);
        if (failed) {
            MEM_MONITOR_FREE(emp, faargp);
            return NULL;
        }
        faargp->object_type = object_type;
        faargp->my_index = my_index;
        return faargp;
    }

    /* cannot be found or created */
    return NULL;
}

static void
destroy_f_and_arg_container (event_manager_t *emp,
    f_and_arg_container_t *farg_cont_p)
{
    /* clean out its ordered list */
    ordered_list_destroy(&farg_cont_p->list_of_f_and_arg,
        destroy_f_and_arg_cb, emp);

    /* remove it off the index it belongs to */
    index_obj_remove(farg_cont_p->my_index, farg_cont_p, NULL);

    /* free the actual memory */
    MEM_MONITOR_FREE(emp, farg_cont_p);
}

/*
 * given the object type & event type, this function finds the correct
 * and relevant structures that needs to be operated on.
 * Note that not all returned variables are available in every context.
 * For example, when object type is 'any object', both the container &
 * index will be meaningless and hence will be set to NULL.
 *
 * Return pointer parameters may be passed in as NULL if any are
 * not needed.
 */
static int
get_relevant_structures (event_manager_t *emp,
        int event_type, int object_type,
        int create_if_missing,

        /* returns these */
        ordered_list_t **list_returned,
        f_and_arg_container_t **container_returned,
        index_obj_t **index_object_returned)
{
    index_obj_t *idxp;
    f_and_arg_container_t *found;

    /* initially nothing is known */
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

    found = get_f_and_arg_container(emp, create_if_missing,
                object_type, idxp);
    if (found) {
        safe_pointer_set(list_returned, &found->list_of_f_and_arg);
        safe_pointer_set(container_returned, found);
        safe_pointer_set(index_object_returned, idxp);
        return 0;
    }

    /* almost all failures are due to lack of memory */
    return ENOMEM;
}

static int
thread_unsafe_already_registered (event_manager_t *emp,
    int event_type, int object_type,
    event_handler_t fptr, void *extra_arg)
{
    int failed;
    f_and_arg_t faarg;
    ordered_list_t *list;
        
    failed = get_relevant_structures(emp, event_type, object_type,
                0, &list, NULL, NULL);
    //if (failed) return 0;

    assert(list);
    faarg.fptr = fptr;
    faarg.extra_arg = extra_arg;
    return (0 == ordered_list_search(list, &faarg, NULL));
}

static int
thread_unsafe_generic_register_function (event_manager_t *emp,
        int event_type, int object_type,
        event_handler_t fptr, void *extra_arg,
        int register_it)
{
    int failed;
    f_and_arg_t fandarg, *fandargp;
    index_obj_t *idxp;
    f_and_arg_container_t *contp;
    ordered_list_t *list;

    /* if we are traversing lists, block any potential changes */
    if (emp->cannot_be_modified) return EBUSY;

    failed = get_relevant_structures(emp, event_type, object_type,
                register_it, &list, &contp, &idxp);

    /*
     * if we are registering, store the function & arg pair in a new
     * f_and_arg_t structure and store it in the list.
     */
    if (register_it) {

        /* :-( */
        if (failed) return failed;
        assert(list);

        /* create new function & arg pair and store it in list */
        fandargp = create_f_and_arg(emp, fptr, extra_arg);
        if (NULL == fandargp) return ENOMEM;

        /* uniquely add it to the list */
        failed = ordered_list_add_once(list, fandargp, NULL);
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
        ordered_list_delete(list, &fandarg, (void**) &fandargp);
        if (fandargp) destroy_f_and_arg_cb(fandargp, emp);

        /*
         * if the list was part of a container (specific object type),
         * AND there is no other function & arg pairs left in the list,
         * get rid of the container object too, no need for it.
         */
        if ((list->n <= 0) && contp) {
            destroy_f_and_arg_container(emp, contp);
        }
    }

    return 0;
}

static void
execute_all_callbacks (ordered_list_t *list, event_record_t *erp)
{
    f_and_arg_t *fandargp;

    if (list) {
        FOR_ALL_ORDEREDLIST_ELEMENTS(list, fandargp) {
            fandargp->fptr(erp, fandargp->extra_arg);
        }
    }
}

static void
thread_unsafe_announce_event (event_manager_t *emp, event_record_t *erp)
{
    ordered_list_t *list = NULL;

    /*
     * starting to traverse lists, lock the lists against any changes
     * which the callback functions may attempt.
     */
    emp->cannot_be_modified = 1;

    /*
     * First, notify the event to the registrants who registered
     * to receive events for ALL/ANY object type.
     */
    get_relevant_structures(emp, ALL_OBJECT_TYPES, erp->event_type, 0,
        &list, NULL, NULL);
    execute_all_callbacks(list, erp);

    /*
     * Next, notify the event to the registrants who are registered
     * to receive events ONLY from this specific type of object.
     */
    get_relevant_structures(emp, erp->object_type, erp->event_type, 0,
        &list, NULL, NULL);
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

    failed = ordered_list_init(&emp->object_event_registrants_for_all_objects, 
            0, compare_f_and_arg, emp->mem_mon_p);
    if (failed) {
        return failed;
    }

    failed = ordered_list_init(&emp->attribute_event_registrants_for_all_objects,
            0, compare_f_and_arg, emp->mem_mon_p);
    if (failed) {
        ordered_list_destroy(&emp->object_event_registrants_for_all_objects,
            NULL, NULL);
        return failed;
    }

    failed = index_obj_init(&emp->object_event_registrants_for_one_object,
            0, compare_f_and_arg_container, 16, 16, emp->mem_mon_p);
    if (failed) {
        ordered_list_destroy(&emp->object_event_registrants_for_all_objects,
            NULL, NULL);
        ordered_list_destroy(&emp->attribute_event_registrants_for_all_objects,
            NULL, NULL);
        return failed;
    }

    failed = index_obj_init(&emp->attribute_event_registrants_for_one_object,
            0, compare_f_and_arg_container, 16, 16, emp->mem_mon_p);
    if (failed) {
        ordered_list_destroy(&emp->object_event_registrants_for_all_objects,
            NULL, NULL);
        ordered_list_destroy(&emp->attribute_event_registrants_for_all_objects,
            NULL, NULL);
        index_obj_destroy(&emp->object_event_registrants_for_one_object,
            NULL, NULL);
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






