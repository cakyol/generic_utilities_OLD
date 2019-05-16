
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

#include "queue_object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

static int
thread_unsafe_queue_expand (queue_obj_t *qobj)
{
    int new_size;
    void **new_elements;
    int w;

    /* q is defined to be of fixed size, not allowed to expand */
    if (qobj->expansion_increment <= 0) return ENOSPC;

    /* expand the elements */
    new_size = qobj->maximum_size + qobj->expansion_increment;
    new_elements = MEM_MONITOR_ALLOC(qobj, (new_size * sizeof(void*)));
    if (NULL == new_elements) return ENOMEM;

    /* copy all unread events/data to new queue elements array */
    for (w = 0; w < qobj->n; w++) {
        new_elements[w] = qobj->elements[qobj->read_idx];
        qobj->read_idx = (qobj->read_idx + 1) % qobj->maximum_size;
    }

    /* it is now safe to adjust all relevant counters etc */
    qobj->read_idx = 0;
    qobj->write_idx = qobj->n;
    qobj->maximum_size = new_size;

    /* free up old queue array assign new one */
    MEM_MONITOR_FREE(qobj, qobj->elements);
    qobj->elements = new_elements;

    /* done */
    qobj->expansion_count++;
    return 0;
}

static int
thread_unsafe_queue_obj_queue (queue_obj_t *qobj, 
        void *data)
{
    int failed;

    /* do we have space */
    if (qobj->n < qobj->maximum_size) {
        qobj->elements[qobj->write_idx++] = data;
        qobj->n++;
        if (qobj->write_idx >= qobj->maximum_size) {
            qobj->write_idx = 0;
        }
        return 0;
    }

    /* no space, can we expand */
    failed = thread_unsafe_queue_expand(qobj);
    if (0 == failed) {
        return
            thread_unsafe_queue_obj_queue(qobj, data);
    }

    /* no way */
    return failed;
}

static int
thread_unsafe_queue_obj_dequeue (queue_obj_t *qobj,
        void **returned_data)
{
    if (qobj->n > 0) {
        *returned_data = qobj->elements[qobj->read_idx++];
        qobj->n--;
        if (qobj->read_idx >= qobj->maximum_size) {
            qobj->read_idx = 0;
        }
        return 0;
    }
    *returned_data = NULL;
    return ENODATA;
}

/************************ public functions ************************/

PUBLIC int
queue_obj_init (queue_obj_t *qobj,
        int make_it_thread_safe,
        int maximum_size, int expansion_increment,
        mem_monitor_t *parent_mem_monitor)
{
    int failed = 0;

    if (maximum_size < 1) {
        return EINVAL;
    }

    MEM_MONITOR_SETUP(qobj);
    LOCK_SETUP(qobj);

    qobj->maximum_size = maximum_size;
    qobj->expansion_increment = expansion_increment;
    qobj->expansion_count = 0;
    qobj->n = 0;
    qobj->read_idx = qobj->write_idx = 0;

    /* allocate its queue element storage */
    qobj->elements = (void**) MEM_MONITOR_ALLOC(qobj,
            (maximum_size * sizeof(void*)));
    if (NULL == qobj->elements) {
        failed = ENOMEM;
    }
    WRITE_UNLOCK(qobj);
    return failed;
}

PUBLIC int
queue_obj_queue (queue_obj_t *qobj,
        void *data)
{
    int failed;

    WRITE_LOCK(qobj);
    failed = thread_unsafe_queue_obj_queue(qobj, data);
    WRITE_UNLOCK(qobj);
    return failed;
}

PUBLIC int
queue_obj_dequeue (queue_obj_t *qobj,
        void **returned_data)
{
    int failed;

    WRITE_LOCK(qobj);
    failed = thread_unsafe_queue_obj_dequeue(qobj, returned_data);
    WRITE_UNLOCK(qobj);
    return failed;
}

PUBLIC void
queue_obj_destroy (queue_obj_t *qobj,
        destruction_handler_t dh_fptr, void *extra_arg)
{
    void *user_data;

    WRITE_LOCK(qobj);

    /* call user function for all user data still in the queue */
    if (dh_fptr) {
        while (0 == thread_unsafe_queue_obj_dequeue(qobj, &user_data)) {
            dh_fptr(user_data, extra_arg);
        }
    }

    /* delete the object itself */
    if (qobj->elements) {
        MEM_MONITOR_FREE(qobj, qobj->elements);
    }

    WRITE_UNLOCK(qobj);
    LOCK_OBJ_DESTROY(qobj);
    memset(qobj, 0, sizeof(queue_obj_t));
}

#ifdef __cplusplus
} // extern C
#endif 



