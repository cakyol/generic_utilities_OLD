
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

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** This is a generic event manager which manages the lists containing
** all the callbacks interested in receiving all kinds of events.
** Currently events are divided into two categories:
**
**  - object events are events which are generated when an object
**    is created or deleted.
**
**  - attribute events are events which are generated when a new
**    attribute id is added to an object or an existing attribute id
**    is deleted from an object or when a value changes in the attribute.
**
** In addition there are two ways of registering for either type of events.
** One is to register in a way so that events are reported for
** ALL object types.  The other is such that events involving ONLY
** one type of object is of interest.
**
** So, a user can register/un-register to receive events for all the following 
** combinations:
**  - object events for ALL object types
**  - object events for a specific object type
**  - attribute events for ALL object types
**  - attribute events for a specific object type
**
** Note that a user may register multiple times for different specific
** object types.  However, when a user registers for events
** for ALL objects, there is NO need to register later for a specific
** object type for the same event later.  The event manager will ensure
** that the registrations make sense by enforcing the following rules:
**
**  - If a user first registers for an event for all objects, and later 
**    registers for the same events for one specific object type,
**    since he is already registered for all objects, the second per
**    object registration will simply be ignored.
**
**  - If a user first registers for events for a specific object type
**    and later he registers for the same events for all object types,
**    then the first registration will be deleted, since now he has
**    already registered for all object types.
**
**  - If a user first registers for events for a specific object type
**    and later UN-registers for all object types, the first registration
**    will be deleted.
**
**  - If a user first registers for events for all objects and later
**    un-registers for one specific object type, the request will be 
**    ignored.
**
**  - If a user wants to transition from a position of getting events
**    for all objects to getting events for only specific objects, he
**    must explicitly first un-register from all object types and then
**    register for the specific objects later.  This is the only time
**    he has to explicity do multiple actions.  The system will NOT
**    automatically do that.  Failure to do so may cause the user
**    to be notified multiple times with the same event.
**
**  - What "defines" registration is the callback function being registered.
**    The function pointer is the unique identifier which defines a
**    registration.
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

#include "lock_object.h"
#include "linkedlist_object.h"
#include "dynamic_array_object.h"
#include "object_types.h"

/*
 * These are *ALL* the possible events which can happen
 * These definitions are typically used with the event
 * manager & object manager code.
 */

#define OBJECT_CREATED                  (1 << 0)
#define OBJECT_DESTROYED                (1 << 1)
#define OBJECT_EVENTS \
    (OBJECT_CREATED | OBJECT_DESTROYED)

#define ATTRIBUTE_INSTANCE_ADDED        (1 << 5)
#define ATTRIBUTE_INSTANCE_DELETED      (1 << 6)
#define ATTRIBUTE_INSTANCE_EVENTS \
    (ATTRIBUTE_INSTANCE_ADDED | ATTRIBUTE_INSTANCE_DELETED)

#define ATTRIBUTE_VALUE_ADDED           (1 << 10)
#define ATTRIBUTE_VALUE_DELETED         (1 << 11)
#define ATTRIBUTE_VALUE_EVENTS \
    (ATTRIBUTE_VALUE_ADDED | ATTRIBUTE_VALUE_DELETED)

#define ATTRIBUTE_EVENTS \
    (ATTRIBUTE_INSTANCE_EVENTS | ATTRIBUTE_VALUE_EVENTS)

static inline int
is_an_object_event (int event)
{ return (event & OBJECT_EVENTS); }

static inline int
is_an_attribute_value_event (int event)
{ return (event & ATTRIBUTE_VALUE_EVENTS); }

static inline int
is_an_attribute_instance_event (int event)
{ return (event & ATTRIBUTE_INSTANCE_EVENTS); }

static inline int
is_an_attribute_event (int event)
{ return (event & ATTRIBUTE_EVENTS); }

/*
 * This structure is used to notify every possible event that could
 * possibly happen in the system.  ALL the events fall into the
 * events category above.
 *
 * The structure includes all the information needed to process
 * the event completely and it obviously should NOT contain any pointers
 * in case events are notified across different processes.
 *
 * The 'event_type' field determines which part of the rest of the structure
 * elements are needed (or ignored).
 *
 * One possible explanation is needed in the case of an attribute event.
 * If this was an attribute event which involves a variable size
 * attribute value, the length will be specified in 'attribute_value_length'
 * and the byte stream would be available from '&attribute_value_data'
 * onwards.  This makes the structure of variable size, based on
 * the parsed parameters, with a complex attribute directly attached to the
 * end of the structure.
 */
typedef struct event_record_s {

    /*
     * this MUST be first field since during reads & writes, it needs
     * to be determined at the very beginning.  It will also be used
     * to copy the entire structure since its length is variable.
     */
    int total_length;

    /*
     * What is this event ?  Chosen from one of the
     * possibilities above.
     */
    int event_type;

    /* optional: which database this event/command applies to */
    int database_id;

    /* object to which the event is directly relevant to */
    int object_type;
    int object_instance;

    /*
     * if the event involves another object, this is that OTHER object.
     * Not used if another object is not involved.
     *
     * One example of when it is used is during an object creation event.
     * In this situation, the 'object_type' & 'object_instance' will
     * represent the object actually created and 'related_object_type'
     * and 'related_object_instance' will represent its parent.
     *
     * An example of when it is NOT needed is during an object deletion
     * event.  In this case, no other object is involved and these
     * are not used.
     */
    int related_object_type;
    int related_object_instance;

    /* if the event involves an attribute, these are also used */
    int attribute_id;
    int attribute_value_length;
    long long int attribute_value_data;
    unsigned char extra_data [0];

} event_record_t;

typedef struct event_manager_s {

    LOCK_VARIABLES;
    MEM_MON_VARIABLES;

    /*
     * while traversing lists and calling the user registered callbacks,
     * it has to be ensured that the callbacks can NOT register or
     * deregister to corrupt the lists.
     */
    int cannot_be_modified;

    /*
     * list of registrants interested in object creation
     * and deletion events for ALL types of objects.
     */
    linkedlist_t all_types_object_registrants;

    /*
     * list of registrants interested in attribute
     * events (attribute id add/delete, attribute value
     * add/delete) for ALL types of objects.
     */
    linkedlist_t all_types_attribute_registrants;
    
    /*
     * list of registrants interested in object
     * events for ONE specific type of object.
     * Array index is the object_type.
     */
    dynamic_array_t specific_object_registrants;
    
    /*
     * list of registrants interested in attribute
     * events for ONE specific type of object.
     * Array index is the object_type.
     */
    dynamic_array_t specific_attribute_registrants;

} event_manager_t;

extern int
event_manager_init (event_manager_t *emp,
    int make_it_thread_safe,
    mem_monitor_t *parent_mem_monitor);

/*
 * This function registers the caller to be notified of object events
 * (object creation & deletion) for either one object or all objects.
 * If 'object_type' is ALL_OBJECT_TYPES, then object events for all
 * objects will be reported.  If it is a specific object, then object
 * events only for that object type will be reported.
 *
 * When a caller registers for events for an object, he supplies
 * an event callback function and an opaque parameter.  When the
 * event occurs, the callback will be called with the FIRST parameter
 * being the event_record_t parameter and the SECOND parameter
 * being what has been registered here as 'user_param'.
 *
 * Note that since this is a registration only for object events,
 * the event record passed into the callback function will be 
 * guaranteed to be of only object creation & deletion events.
 */
extern int
register_for_object_events (event_manager_t *emp,
    int object_type, 
    two_parameter_function_pointer ecbf, void *user_param);

/*
 * unregisters from object events for the specified object type.
 * Basically, reverse of the above.
 */
extern void
un_register_from_object_events (event_manager_t *emp,
    int object_type, two_parameter_function_pointer ecbf);

/*
 * Same concept as above but this time registration is only for attribute
 * events, which are attribute id add/delete and attribute value add/delete.
 *
 * Similar to above, the event record passed into this callback function
 * will be guaranteed to be only of attribute events.
 */
extern int
register_for_attribute_events (event_manager_t *emp,
    int object_type, 
    two_parameter_function_pointer ecbf, void *user_param);

/*
 * reverse of the above
 */
extern void
un_register_from_attribute_events (event_manager_t *emp,
    int object_type, two_parameter_function_pointer ecbf);

/*
 * The user calls this when he wants to report the occurence of an event.
 * Based on the event type, object type in the event record, all the
 * registered functions will be invoked one by one.
 */
extern void
notify_event (event_manager_t *emp, event_record_t *erp);

extern void
event_manager_destroy (event_manager_t *emp);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __EVENT_MANAGER_H__


