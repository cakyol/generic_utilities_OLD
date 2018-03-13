
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
**
** @@@@@ Selector object (table) between an AVL tree or an index object
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __TABLE_H__
#define __TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "avl_tree_object.h"
#include "index_object.h"

typedef struct table_s {

    int use_avl_tree;
    union {
	avl_tree_t avl;
	index_obj_t idx;
    } u;

} table_t;

extern int 
table_init (table_t *tablep, 
	int make_it_thread_safe,
	comparison_function_pointer cmpf, 
        mem_monitor_t *parent_mem_monitor,
        int use_avl_tree);

extern int 
table_insert (table_t *tablep, 
	void *to_be_inserted, void **returned);

extern int 
table_search (table_t *tablep, 
	void *searched, void **returned);

extern int 
table_remove (table_t *tablep, 
	void *to_be_removed, void **removed);

extern int
table_traverse (table_t *tablep, traverse_function_pointer tfn,
	void *p0, void *p1, void *p2, void *p3);

extern void **
table_get_all (table_t *tablep, int *returned_count);

extern int
table_member_count (table_t *tablep);

extern unsigned long long int
table_memory_usage (table_t *tablep, double *mega_bytes);

extern void 
table_destroy (table_t *tablep);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __TABLE_H__


