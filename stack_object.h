
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

#ifndef __STACK_OBJECT_H__
#define __STACK_OBJECT_H__

#include "utils_common.h"
#include "lock_object.h"

typedef struct stack_obj_s {

    LOCK_VARIABLES;
    MEM_MON_VARIABLES;

    int maximum_size;
    int expansion_size;
    int expansion_count;
    int n;
    datum_t *elements;

} stack_obj_t;

extern error_t
stack_obj_init (stack_obj_t *stk,
	boolean make_it_thread_safe,
	int maximum_size,
	int expansion_size);

extern error_t
stack_obj_push (stack_obj_t *stk,
	datum_t data);

extern error_t
stack_obj_pop (stack_obj_t *stk,
	datum_t *returned_data);

extern void
stack_obj_destroy (stack_obj_t *stk);

/* integer or pointer specific functions */

extern error_t
stack_obj_push_integer (stack_obj_t *stk,
	int integer);

extern error_t
stack_obj_pop_integer (stack_obj_t *stk,
	int *returned_integer);

extern error_t
stack_obj_push_pointer (stack_obj_t *stk,
	void *pointer);

extern error_t
stack_obj_pop_pointer (stack_obj_t *stk,
	void **returned_pointer);

#endif // __STACK_OBJECT_H__


