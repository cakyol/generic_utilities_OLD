
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

#ifndef __QUEUE_OBJECT_H__
#define __QUEUE_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <string.h>

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"

/*
 * generic queue structure.
 * queue elements are each a fixed number of bytes,
 * determined at initialization time.
 *
 * The qobject operations all take a callback function pointer.
 * This function will be invoked if the q operation succeeds.
 * The reason for this is to ensure that any event based mechanisms
 * which 'wake up' when a q operation occurs, gets notified.
 * This is just for user convenience, for better event management.
 * If this notification is not needed, NULL can be specified as
 * the function pointer.
 */
typedef struct qobj_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    /* queues may expand by the specified size, if non zero */
    int maximum_size;
    int expansion_increment;
    int expansion_count;

    /* how many elements are in the queue at a given instance */
    int n;

    /* the size in bytes of each queue element */
    int element_size;

    /* read index & write index */
    int read_idx, write_idx;

    /* actual queue elements */
    byte *elements;

    statistics_block_t stats;

} qobj_t;

/*
 * This is the prototype of the function passed in by the user to
 * report the queueing/dequeuiung event.  This is typically used
 * when a process/thread needs to know when something has been queued
 * or dequeued from the object so that a 'wakeup' may be scheduled.
 *
 * The parameters are:
 *  - the 'q' is the queue object in question to which the event applies.
 *  - 'queued' is 1 for a queue event and 0 is a dequeueing event.
 *  - 'element' is the pointer to the element which got subjected to the
 *    event.
 *  - If the event fails (either queueing or dequeuing), then the function
 *    will NOT be called.  The call only happens for a SUCCESSFULL outcome.
 */
typedef void (*queue_event_function_pointer)
    (qobj_t *q, bool queued, void *element);

/*
 * initialize a q object.
 *
 * 'make_it_thread_safe' will lock it for
 * multi threaded and/or shared memory applications.
 *
 * 'element_size' is the size of each queue element in bytes.
 *
 * 'maximum_size' is the initial creation size of the queue
 * as to how many elements its capacity is.
 *
 * 'expansion_increment', if not 0, specifies by how many
 * elements it will expand each time it needs to (when full).
 * If this value is 0, the q will NOT be allowed to expand
 * and further queueing operations will fail.
 *
 */
extern int
qobj_init (qobj_t *qobj,
        int make_it_thread_safe,
        int element_size,
        int maximum_size, int expansion_increment,
        mem_monitor_t *parent_mem_monitor);

extern int
qobj_queue (qobj_t *qobj, void *elementp,
        queue_event_function_pointer fnp);

extern int
qobj_dequeue (qobj_t *qobj, void *returned_element,
        queue_event_function_pointer fnp);

extern void
qobj_destroy (qobj_t *qobj,
        destruction_handler_t dh_fptr, void *extra_arg);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __QUEUE_OBJECT_H__


