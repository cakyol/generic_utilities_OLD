
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
 * generic object structure
 */
typedef struct queue_obj_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    /* queues may expand by the specified size, if non zero */
    int maximum_size;
    int expansion_increment;
    int expansion_count;

    /* how many elements are in the queue at a given instance */
    int n;

    /* read index & write index */
    int read_idx, write_idx;

    /* actual queue elements */
    void **elements;

} queue_obj_t;

extern int
queue_obj_init (queue_obj_t *qobj,
        int make_it_thread_safe,
        int maximum_size, int expansion_increment,
        mem_monitor_t *parent_mem_monitor);

extern int
queue_obj_queue (queue_obj_t *qobj,
        void *data);

extern int
queue_obj_dequeue (queue_obj_t *qobj,
        void **returned_data);

extern void
queue_obj_destroy (queue_obj_t *qobj);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __QUEUE_OBJECT_H__


