
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

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

static int
thread_unsafe_stack_obj_push (stack_obj_t *stk, 
	void *data)
{
    int new_maximum_size;
    void **new_elements;

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
    new_elements = MEM_REALLOC(stk,
                        stk->elements, (new_maximum_size * sizeof(void*)));
    if (NULL == new_elements) {
	return ENOMEM;
    }
    stk->elements = new_elements;
    stk->maximum_size = new_maximum_size;
    stk->expansion_count++;
    return
	thread_unsafe_stack_obj_push(stk, data);
}

static int
thread_unsafe_stack_obj_pop (stack_obj_t *stk,
	void **returned_data)
{
    if (stk->n > 0) {
	*returned_data = stk->elements[--stk->n];
	return 0;
    }
    *returned_data = NULL;
    return ENODATA;
}

/************************ public functions ************************/

PUBLIC int
stack_obj_init (stack_obj_t *stk,
	int make_it_thread_safe,
	int maximum_size,
	int expansion_size,
        mem_monitor_t *parent_mem_monitor)
{
    int rv = 0;

    if (maximum_size <= 0) {
	return EINVAL;
    }
    if (expansion_size < 0) {
	return EINVAL;
    }
    LOCK_SETUP(stk);
    MEM_MONITOR_SETUP(stk);
    stk->maximum_size = maximum_size;
    stk->expansion_size = expansion_size;
    stk->expansion_count = 0;
    stk->n = 0;
    stk->elements = MEM_MONITOR_ALLOC(stk, maximum_size * sizeof(void*));
    if (NULL == stk->elements) {
	rv = ENOMEM;
    }
    WRITE_UNLOCK(stk);
    return rv;
}

PUBLIC int
stack_obj_push (stack_obj_t *stk,
	void *data)
{
    int rv;

    WRITE_LOCK(stk);
    rv = thread_unsafe_stack_obj_push(stk, data);
    WRITE_UNLOCK(stk);
    return rv;
}

PUBLIC int
stack_obj_pop (stack_obj_t *stk,
	void **returned_data)
{
    int rv;

    WRITE_LOCK(stk);
    rv = thread_unsafe_stack_obj_pop(stk, returned_data);
    WRITE_UNLOCK(stk);
    return rv;
}

PUBLIC void
stack_obj_destroy (stack_obj_t *stk)
{
    if (stk->elements) {
	MEM_MONITOR_FREE(stk, stk->elements);
    }
    LOCK_OBJ_DESTROY(stk);
    memset(stk, 0, sizeof(stack_obj_t));

}

#ifdef __cplusplus
} // extern C
#endif 





