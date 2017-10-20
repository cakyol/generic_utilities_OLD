
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

/*
 * determines whether the two processes passed in as params
 * are 'considered' to be equal
 */
static int
compare_process_ids (datum_t d1, datum_t d2)
{
    /* change this later */
    return 0;
}

static sll_object_t *
get_correct_list (event_manager_t *evrp,
        int object_type, int event_type)
{
    error_t rv;
    dynamic_array_t *dap;
    sll_object_t *list;
    datum_t list_datum;

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

    if (FAILED(dynamic_array_get(dap, object_type, &list_datum))) {
        
        list = MEM_ALLOC(evrp, sizeof(sll_object_t));
        if (NULL == list)
            return NULL;

        rv = sll_object_init(list, false,
                compare_process_ids, evrp->mem_mon_p);
        if (FAILED(rv)) {
            MEM_FREE(evrp, list);
            return NULL;
        }

        list_datum.pointer = list;
        rv = dynamic_array_insert(dap, object_type, list_datum);
        if (FAILED(rv)) {
            MEM_FREE(evrp, list);
            return NULL;
        }

        return list;
    }
        
    return list_datum.pointer;
}

static error_t
thread_unsafe_generic_register_function (event_manager_t *evrp,
        int object_type,
        process_address_t *pap,
        int event_type,
        boolean register_it)
{
    sll_object_t *list;
    datum_t address_datum, deleted_address_datum;

    list = get_correct_list(evrp, object_type, event_type);
    if (list) {
        address_datum.pointer = pap;
        if (register_it) {
            return
                sll_object_add_once(list, address_datum);
        }

        /* 
         * un-register, does not matter if it fails, since it means
         * that the process was not in the list in the first place,
         * the intended result is achieved either way.
         */
        sll_object_delete(list, address_datum, &deleted_address_datum);

        return 0;

    } else {

        /* 
         * list is NULL ? it does not matter if it was 
         * an un-registration request but an error if it
         * was a registration request.
         */
        if (register_it) return ENOSPC;
    }

    /* everything ok */
    return 0;
}


/*****************************************************************************/

PUBLIC error_t
event_manager_init (event_manager_t *evrp,
	boolean make_it_thread_safe,
	mem_monitor_t *parent_mem_monitor)
{
    error_t rv;

    OBJECT_LOCK_SETUP(evrp);
    MEM_MONITOR_SETUP(evrp);

    rv = sll_object_init(&evrp->all_types_object_processes, 
            false, compare_process_ids, evrp->mem_mon_p);
    if (rv) {
        return rv;
    }

    rv = sll_object_init(&evrp->all_types_attribute_processes,
            false, compare_process_ids, evrp->mem_mon_p);
    if (rv) {
        sll_object_destroy(&evrp->all_types_object_processes);
        return rv;
    }

    rv = dynamic_array_init(&evrp->specific_object_processes,
            false, 3, evrp->mem_mon_p);
    if (rv) {
        sll_object_destroy(&evrp->all_types_object_processes);
        sll_object_destroy(&evrp->all_types_attribute_processes);
        return rv;
    }

    rv = dynamic_array_init(&evrp->specific_attribute_processes,
            false, 3, evrp->mem_mon_p);
    if (rv) {
        sll_object_destroy(&evrp->all_types_object_processes);
        sll_object_destroy(&evrp->all_types_attribute_processes);
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
 * will be ALL object types.  If 'activate' is true, the notifications
 * will start, else if false, notifications will be stopped.
 */
PUBLIC error_t
register_for_object_events (event_manager_t *evrp,
        int object_type, process_address_t *pap)
{
    error_t rv;

    WRITE_LOCK(evrp, NULL);
    rv = thread_unsafe_generic_register_function(evrp, object_type, pap,
            OBJECT_EVENTS, true);
    WRITE_UNLOCK(evrp);
    return rv;
}

PUBLIC void
un_register_from_object_events (event_manager_t *evrp,
        int object_type, process_address_t *pap)
{
    WRITE_LOCK(evrp, NULL);
    (void) thread_unsafe_generic_register_function(evrp, object_type, pap,
            OBJECT_EVENTS, false);
    WRITE_UNLOCK(evrp);
}

/*
 * This is also similar to above but it will register for all attribute
 * events including attribute add, delete and all value changes.
 */
PUBLIC error_t
register_for_attribute_events (event_manager_t *evrp,
        int object_type, process_address_t *pap)
{
    error_t rv;

    WRITE_LOCK(evrp, NULL);
    rv = thread_unsafe_generic_register_function(evrp, object_type, pap,
            ATTRIBUTE_EVENTS, true);
    WRITE_UNLOCK(evrp);
    return rv;
}

PUBLIC void
un_register_from_attribute_events (event_manager_t *evrp,
        int object_type, process_address_t *pap)
{
    WRITE_LOCK(evrp, NULL);
    (void) thread_unsafe_generic_register_function(evrp, object_type, pap,
            ATTRIBUTE_EVENTS, true);
    WRITE_UNLOCK(evrp);
}

PUBLIC void
event_manager_destroy (event_manager_t *evrp)
{
}





