
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

#include "utils_common.h"
#include "avl_tree_object.h"
#include "index_object.h"

typedef struct table_s {

    boolean use_avl_tree;
    union {
	avl_tree_t avl;
	index_obj_t idx;
    } u;

} table_t;

extern error_t 
table_init (table_t *tablep, 
	boolean make_it_lockable,
	comparison_function_t cmpf, 
        mem_monitor_t *parent_mem_monitor,
        boolean use_avl_tree);

extern error_t 
table_insert (table_t *tablep, 
	datum_t to_be_inserted, datum_t *returned);

extern error_t 
table_search (table_t *tablep, 
	datum_t searched, datum_t *returned);

extern error_t 
table_remove (table_t *tablep, 
	datum_t to_be_removed, datum_t *removed);

extern error_t
table_traverse (table_t *tablep, traverse_function_t tfn,
	datum_t p0, datum_t p1, datum_t p2, datum_t p3);

extern datum_t *
table_get_all (table_t *tablep, int *returned_count);

extern int
table_member_count (table_t *tablep);

extern uint64
table_memory_usage (table_t *tablep, double *mega_bytes);

extern void 
table_destroy (table_t *tablep);

#endif // __TABLE_H__


