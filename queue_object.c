
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

static error_t
thread_unsafe_queue_obj_queue (queue_obj_t *qobj, 
	datum_t data)
{
    if (qobj->n < qobj->maximum_size) {
	qobj->elements[qobj->write_idx] = data;
	qobj->n++;
	qobj->write_idx = (qobj->write_idx + 1) % qobj->maximum_size;
	return 0;
    }
    return ENOSPC;
}

static error_t
thread_unsafe_queue_obj_dequeue (queue_obj_t *qobj,
	datum_t *returned_data)
{
    if (qobj->n > 0) {
	*returned_data = qobj->elements[qobj->read_idx];
	qobj->n--;
	qobj->read_idx = (qobj->read_idx + 1) % qobj->maximum_size;
	return 0;
    }
    returned_data->pointer = NULL;
    return ENODATA;
}

/************************ public functions ************************/

PUBLIC error_t
queue_obj_init (queue_obj_t *qobj,
	boolean make_it_lockable,
	int maximum_size)
{
    error_t rv = 0;

    if (maximum_size < 1) {
	return EINVAL;
    }

    LOCK_SETUP(qobj);
    qobj->maximum_size = maximum_size;
    qobj->n = 0;
    qobj->read_idx = qobj->write_idx = 0;

    /* allocate its queue element storage */
    qobj->elements = (datum_t*) malloc(maximum_size * sizeof(datum_t));
    if (NULL == qobj->elements) {
	rv = ENOMEM;
    }
    WRITE_UNLOCK(qobj);
    return rv;
}

PUBLIC error_t
queue_obj_queue (queue_obj_t *qobj,
	datum_t data)
{
    error_t rv;

    WRITE_LOCK(qobj, NULL);
    rv = thread_unsafe_queue_obj_queue(qobj, data);
    WRITE_UNLOCK(qobj);
    return rv;
}

PUBLIC error_t
queue_obj_dequeue (queue_obj_t *qobj,
	datum_t *returned_data)
{
    error_t rv;

    WRITE_LOCK(qobj, NULL);
    rv = thread_unsafe_queue_obj_dequeue(qobj, returned_data);
    WRITE_UNLOCK(qobj);
    return rv;
}

PUBLIC void
queue_obj_destroy (queue_obj_t *qobj)
{
    if (qobj->elements) {
	free(qobj->elements);
    }
    LOCK_OBJ_DESTROY(qobj);
    memset(qobj, 0, sizeof(queue_obj_t));
}

PUBLIC error_t
queue_obj_queue_integer (queue_obj_t *qobj,
	int integer)
{
    datum_t data;

    data.integer = integer;
    return
	queue_obj_queue(qobj, data);
}

PUBLIC error_t
queue_obj_dequeue_integer (queue_obj_t *qobj,
	int *returned_integer)
{
    datum_t data;
    error_t rv;

    rv = queue_obj_dequeue(qobj, &data);
    *returned_integer = data.integer;
    return rv;
}

PUBLIC error_t
queue_obj_queue_pointer (queue_obj_t *qobj,
	void *pointer)
{
    datum_t data;

    data.pointer = pointer;
    return
	queue_obj_queue(qobj, data);
}

PUBLIC error_t
queue_obj_dequeue_pointer (queue_obj_t *qobj,
	void **returned_pointer)
{
    datum_t data;
    error_t rv;

    rv = queue_obj_dequeue(qobj, &data);
    *returned_pointer = data.pointer;
    return rv;
}




