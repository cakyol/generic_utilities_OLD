
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"

typedef struct stack_obj_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    int maximum_size;
    int expansion_size;
    int expansion_count;
    int n;
    void **elements;

} stack_obj_t;

extern int
stack_obj_init (stack_obj_t *stk,
        int make_it_thread_safe,
        int maximum_size,
        int expansion_size,
        mem_monitor_t *parent_mem_monitor);

extern int
stack_obj_push (stack_obj_t *stk,
        void *data);

extern int
stack_obj_pop (stack_obj_t *stk,
        void **returned_data);

extern void
stack_obj_destroy (stack_obj_t *stk,
        destruction_handler_t dh_fptr, void *extra_arg);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __STACK_OBJECT_H__


