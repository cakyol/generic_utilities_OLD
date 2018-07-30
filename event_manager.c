
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
 * The user callback will be called with a
 */
typedef struct event_notification_record_s {

    process_address_t pa;
    void *opaque_user_parameter;


} event_notification_record_t;

/*
 * determines whether the two processes passed in as params
 * are 'considered' to be equal
 */
static int
compare_process_ids (void *vpap1, void *vpap2)
{
    /* change this later */
    return 0;
}

/*
 * given the object type & event type, finds the correct
 * and relevant list that needs to be operated on.
 */
static linkedlist_t *
get_correct_list (event_manager_t *evrp,
        int object_type, int event_type, int create_if_missing)
{
    int rv;
    dynamic_array_t *dap;
    linkedlist_t *list;

    /*
     * return "all types of objects" appropriate list
     */
    if (ALL_OBJECT_TYPES == object_type) {

        if (is_an_object_event(event_type)) {
            return
                &evrp->all_types_object_processes;
        } else if (is_an_attribute_event(event_type)) {
            return
                &evrp->all_types_attribute_processes;
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
        dap = &evrp->specific_object_processes;
    } else if (is_an_attribute_event(event_type)) {
        dap = &evrp->specific_attribute_processes;
    } else {
        /* invalid event type */
        return NULL;
    }

    /* get the list of processes interested in events for this object type */
    rv = dynamic_array_get(dap, object_type, (void**) &list);

    /* 
     * if no such list exists, create and initialize it,
     * if creation is needed.
     *
     * Note that this list will hold all the processes
     * which are interested in being notified abou the 
     * specific event type for the specified object type.
     */
    if ((0 != rv) && create_if_missing) {
	list = MEM_MONITOR_ALLOC(evrp, sizeof(linkedlist_t));
	if (NULL == list) return NULL;
	rv = linkedlist_init(list, 0, compare_process_ids, evrp->mem_mon_p);
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
        int object_type,
        process_address_t *pap,
        int event_type,
        int register_it)
{
    void *found_pap;
    linkedlist_t *list = 
	get_correct_list(evrp, object_type, event_type, register_it);

    /*
     * if we are registering, we must create a list in the appropriate
     * dynamic array.
     */
    if (register_it) {
	if (list) {
	    return
		linkedlist_add_once(list, pap, &found_pap);
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
     * it.
     */
    if (list) {
	return
	    linkedlist_delete(list, pap, &found_pap);
    }

    return ENODATA;
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

    rv = linkedlist_init(&evrp->all_types_object_processes, 
            0, compare_process_ids, evrp->mem_mon_p);
    if (rv) {
        return rv;
    }

    rv = linkedlist_init(&evrp->all_types_attribute_processes,
            0, compare_process_ids, evrp->mem_mon_p);
    if (rv) {
        linkedlist_destroy(&evrp->all_types_object_processes);
        return rv;
    }

    rv = dynamic_array_init(&evrp->specific_object_processes,
            0, 3, evrp->mem_mon_p);
    if (rv) {
        linkedlist_destroy(&evrp->all_types_object_processes);
        linkedlist_destroy(&evrp->all_types_attribute_processes);
        return rv;
    }

    rv = dynamic_array_init(&evrp->specific_attribute_processes,
            0, 3, evrp->mem_mon_p);
    if (rv) {
        linkedlist_destroy(&evrp->all_types_object_processes);
        linkedlist_destroy(&evrp->all_types_attribute_processes);
        dynamic_array_destroy(&evrp->specific_object_processes);
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
        int object_type, process_address_t *pap)
{
    int rv;

    WRITE_LOCK(evrp);
    rv = thread_unsafe_generic_register_function(evrp, object_type, pap,
            OBJECT_EVENTS, 1);
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






