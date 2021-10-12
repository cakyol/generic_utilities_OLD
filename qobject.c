
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

#include "qobject.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

static int
thread_unsafe_queue_expand (qobj_t *qobj)
{
    int new_size;
    byte *new_elements;
    int w;

    /* q is defined to be of fixed size, not allowed to expand */
    if (qobj->expansion_increment <= 0)
        return ENOSPC;

    /* expand the elements */
    new_size = qobj->maximum_size + qobj->expansion_increment;
    new_elements = MEM_MONITOR_ZALLOC(qobj, (new_size * qobj->element_size));
    if (NULL == new_elements)
        return ENOMEM;

    /* copy all unread events/data to new queue elements array */
    for (w = 0; w < qobj->n; w++) {
        copy_array_blocks(qobj->elements, qobj->read_idx,
            new_elements, w, 1, qobj->element_size);
        qobj->read_idx = (qobj->read_idx + 1) % qobj->maximum_size;
    }

    /* it is now safe to adjust all relevant counters etc */
    qobj->read_idx = 0;
    qobj->write_idx = qobj->n;
    qobj->maximum_size = new_size;

    /* free up old queue array assign new one */
    MEM_MONITOR_FREE(qobj->elements);
    qobj->elements = new_elements;

    /* done */
    qobj->expansion_count++;

    return 0;
}

static int
thread_unsafe_qobj_queue (qobj_t *qobj, void *data,
        queue_event_function_pointer fnp)
{
    int failed;

    /* do we have space */
    if (qobj->n < qobj->maximum_size) {
        copy_array_blocks(data, 0, qobj->elements, qobj->write_idx,
            1, qobj->element_size);
        qobj->n++;
        qobj->write_idx++;
        if (qobj->write_idx >= qobj->maximum_size) {
            qobj->write_idx = 0;
        }

        /* update stats */
        insertion_succeeded(qobj);

        /* notify the success if needed */
        if (fnp) {
            fnp(qobj, true, data);
        }

        return 0;
    }

    /* no space, can we expand */
    failed = thread_unsafe_queue_expand(qobj);
    if (failed) {
        insertion_failed(qobj);
        return failed;
    }

    /* we successfully expanded, try & requeue */
    return
        thread_unsafe_qobj_queue(qobj, data, fnp);
}

static int
thread_unsafe_qobj_dequeue (qobj_t *qobj,
        void *returned_data,
        queue_event_function_pointer fnp)
{
    if (qobj->n > 0) {
        copy_array_blocks(qobj->elements, qobj->read_idx,
            returned_data, 0, 1, qobj->element_size);
        qobj->n--;
        qobj->read_idx++;
        if (qobj->read_idx >= qobj->maximum_size) {
            qobj->read_idx = 0;
        }
        deletion_succeeded(qobj);
        if (fnp) {
            fnp(qobj, false, returned_data);
        }
        return 0;
    }
    deletion_failed(qobj);
    return ENODATA;
}

/************************ public functions ************************/

PUBLIC int
qobj_init (qobj_t *qobj,
        boolean make_it_thread_safe,
        boolean statistics_wanted,
        int element_size,
        int maximum_size, int expansion_increment,
        mem_monitor_t *parent_mem_monitor)
{
    int failed = 0;

    if (maximum_size < 1) {
        return EINVAL;
    }
    if (element_size < 1) {
        return EINVAL;
    }

    MEM_MONITOR_SETUP(qobj);
    LOCK_SETUP(qobj);
    STATISTICS_SETUP(qobj);

    qobj->element_size = element_size;
    qobj->maximum_size = maximum_size;
    qobj->expansion_increment = expansion_increment;
    qobj->expansion_count = 0;
    qobj->n = 0;
    qobj->read_idx = qobj->write_idx = 0;
    reset_stats(qobj);

    /* allocate its queue element storage */
    qobj->elements = MEM_MONITOR_ZALLOC(qobj, (maximum_size * element_size));
    if (NULL == qobj->elements) {
        failed = ENOMEM;
    }
    OBJ_WRITE_UNLOCK(qobj);
    return failed;
}

PUBLIC int
qobj_queue (qobj_t *qobj, void *data,
        queue_event_function_pointer fnp)
{
    int failed;

    OBJ_WRITE_LOCK(qobj);
    failed = thread_unsafe_qobj_queue(qobj, data, fnp);
    OBJ_WRITE_UNLOCK(qobj);
    return failed;
}

PUBLIC int
qobj_dequeue (qobj_t *qobj, void *returned_data,
        queue_event_function_pointer fnp)
{
    int failed;

    OBJ_WRITE_LOCK(qobj);
    failed = thread_unsafe_qobj_dequeue(qobj, returned_data, fnp);
    OBJ_WRITE_UNLOCK(qobj);
    return failed;
}

PUBLIC void
qobj_destroy (qobj_t *qobj,
        destruction_handler_t dh_fptr, void *extra_arg)
{
    byte *elemp;

    OBJ_WRITE_LOCK(qobj);

    /* call the destruction function for every element still in the queue */
    if (dh_fptr) {

        /* temp space to extract the element into */
        elemp = malloc(qobj->element_size);

        if (elemp) {
            while (0 == thread_unsafe_qobj_dequeue(qobj, elemp, null)) {
                dh_fptr(elemp, extra_arg);
            }
            free(elemp);
        }
    }

    /* delete the object itself */
    if (qobj->elements) {
        MEM_MONITOR_FREE(qobj->elements);
    }

    OBJ_WRITE_UNLOCK(qobj);
    LOCK_OBJ_DESTROY(qobj);
    memset(qobj, 0, sizeof(qobj_t));
}

#ifdef __cplusplus
} // extern C
#endif 



