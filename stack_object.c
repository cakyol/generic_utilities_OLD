
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

#include "stack_object.h"

static error_t
thread_unsafe_stack_obj_push (stack_obj_t *stk, 
	datum_t data)
{
    int new_maximum_size;
    datum_t *new_elements;

    /* enuf space, simply push it */
    if (stk->n < stk->maximum_size) {
	stk->elements[stk->n++] = data;
	return 0;
    }

    /* stack full and cannot expand */
    if (stk->expansion_size <= 0) {
	return ENOSPC;
    }

    /* try & expand */
    new_maximum_size = stk->maximum_size + stk->expansion_size;
    new_elements = realloc(stk->elements, (new_maximum_size * sizeof(datum_t)));
    if (NULL == new_elements) {
	return ENOMEM;
    }
    stk->elements = new_elements;
    stk->maximum_size = new_maximum_size;
    stk->expansion_count++;
    return
	thread_unsafe_stack_obj_push(stk, data);
}

static error_t
thread_unsafe_stack_obj_pop (stack_obj_t *stk,
	datum_t *returned_data)
{
    if (stk->n > 0) {
	SAFE_POINTER_SET(returned_data, stk->elements[--stk->n]);
	return 0;
    }
    SAFE_NULLIFY_DATUMP(returned_data);
    return ENODATA;
}

/************************ public functions ************************/

PUBLIC error_t
stack_obj_init (stack_obj_t *stk,
	boolean make_it_lockable,
	int maximum_size,
	int expansion_size)
{
    error_t rv = 0;

    if (maximum_size <= 0) {
	return EINVAL;
    }
    if (expansion_size < 0) {
	return EINVAL;
    }
    LOCK_SETUP(stk);
    stk->maximum_size = maximum_size;
    stk->expansion_size = expansion_size;
    stk->expansion_count = 0;
    stk->n = 0;
    stk->elements = malloc(maximum_size * sizeof(datum_t));
    if (NULL == stk->elements) {
	rv = ENOMEM;
    }
    WRITE_UNLOCK(stk);
    return rv;
}

PUBLIC error_t
stack_obj_push (stack_obj_t *stk,
	datum_t data)
{
    error_t rv;

    WRITE_LOCK(stk, NULL);
    rv = thread_unsafe_stack_obj_push(stk, data);
    WRITE_UNLOCK(stk);
    return rv;
}

PUBLIC error_t
stack_obj_pop (stack_obj_t *stk,
	datum_t *returned_data)
{
    error_t rv;

    WRITE_LOCK(stk, NULL);
    rv = thread_unsafe_stack_obj_pop(stk, returned_data);
    WRITE_UNLOCK(stk);
    return rv;
}

PUBLIC void
stack_obj_destroy (stack_obj_t *stk)
{
    if (stk->elements) {
	free(stk->elements);
    }
    LOCK_OBJ_DESTROY(stk);
    memset(stk, 0, sizeof(stack_obj_t));

}

PUBLIC error_t
stack_obj_push_integer (stack_obj_t *stk,
	int integer)
{
    datum_t data;

    data.integer = integer;
    return
	stack_obj_push(stk, data);
}

PUBLIC error_t
stack_obj_pop_integer (stack_obj_t *stk,
	int *returned_integer)
{
    datum_t data;
    error_t rv = stack_obj_pop(stk, &data);

    *returned_integer = data.integer;
    return rv;
}

PUBLIC error_t
stack_obj_push_pointer (stack_obj_t *stk,
	void *pointer)
{
    datum_t data;

    data.pointer = pointer;
    return
	stack_obj_push(stk, data);
}

PUBLIC error_t
stack_obj_pop_pointer (stack_obj_t *stk,
	void **returned_pointer)
{
    datum_t data;
    error_t rv = stack_obj_pop(stk, &data);

    *returned_pointer = data.pointer;
    return rv;
}



