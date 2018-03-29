
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

#ifndef __EVENT_MANAGER_H__
#define __EVENT_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "utils_common.h"
#include "lock_object.h"
#include "sll_object.h"
#include "dynamic_array_object.h"
#include "event_types.h"
#include "object_types.h"

/*
 * Basic primitive of how to address/send to a specific process
 * Details of this can be decided later, it can be a message
 * or shared memory interface.
 */
typedef struct process_address_s {

    int whatever;

} process_address_t;

/*
 * This is a generic event manager which manages the lists containing
 * all the processes interested in receiving all kinds of events.
 * Currently events are roughly divided into two categories:
 *
 *  - object events are events which are generated when an object
 *    is created or deleted.
 *
 *  - attribute events are events which are generated when a new
 *    attribute id is added to an object or an existing attribute id
 *    is deleted from an object or when a value changes in any attribute.
 *
 * In addition there are two ways of registering for either type of events.
 * One is to register in a way so that events are reported for
 * ALL object types.  The other is such that events involving ONLY
 * one type of object is of interest.
 *
 * So, a user can register to receive events for all the following 
 * combinations:
 *  - object events for ALL object types
 *  - object events for a specific object type
 *  - attribute events for ALL object types
 *  - attribute events for a specific object type
 *
 * Note that a user may register multiple times for different specific
 * object types.  However, when a user registers for events
 * for ALL objects, there is NO need to register later for a specific
 * object type for the same event later.  In fact that will cause a
 * problem where an event will be reported twice.  Therefore, users
 * must ensure that their process is not registered multiple times
 * in any event type.  The event manager currently does not check for
 * this error (later release feature).
 *
 * If the user changes its interest level from all object types to
 * a specific object type OR vice versa, the previous registration
 * must be cancelled by calling the corresponding "un-register"
 * function first.
 */
typedef struct event_manager_s {

    LOCK_VARIABLES;
    MEM_MON_VARIABLES;

    /*
     * list of processes interested in object creation
     * and deletion events for ALL types of objects.
     */
    sll_object_t all_types_object_processes;

    /*
     * list of processes interested in attribute
     * events (attribute id add/delete, attribute value
     * add/delete) for ALL types of objects.
     */
    sll_object_t all_types_attribute_processes;
    
    /*
     * list of processes interested in object
     * events for ONE specific type of object.
     * Array index is the object_type.
     */
    dynamic_array_t specific_object_processes;
    
    /*
     * list of processes interested in attribute
     * events for ONE specific type of object.
     * Array index is the object_type.
     */
    dynamic_array_t specific_attribute_processes;

} event_manager_t;

extern error_t
event_manager_init (event_manager_t *evrp,
    boolean make_it_thread_safe,
    mem_monitor_t *parent_mem_monitor);

extern error_t
register_for_object_events (event_manager_t *evrp,
        int object_type, process_address_t *pap);

extern void
un_register_from_object_events (event_manager_t *evrp,
        int object_type, process_address_t *pap);

extern error_t
register_for_attribute_events (event_manager_t *evrp,
        int object_type, process_address_t *pap);

extern void
un_register_from_attribute_events (event_manager_t *evrp,
        int object_type, process_address_t *pap);

extern void
event_manager_destroy (event_manager_t *evrp);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __EVENT_MANAGER_H__


