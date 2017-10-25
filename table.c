
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

#include "table.h"

PUBLIC error_t
table_init (table_t *tablep,
	boolean make_it_thread_safe,
	comparison_function_t cmpf, 
        mem_monitor_t *parent_mem_monitor,
	boolean use_avl_tree)
{
    tablep->use_avl_tree = use_avl_tree;
    if (use_avl_tree) {
	return
	    avl_tree_init(&tablep->u.avl, 
                make_it_thread_safe, cmpf, parent_mem_monitor);
    }
    return
	index_obj_init(&tablep->u.idx, 
                make_it_thread_safe, cmpf, 128, 128, parent_mem_monitor);
}

PUBLIC error_t
table_insert (table_t *tablep,
	datum_t to_be_inserted,
	datum_t *returned)
{
    if (tablep->use_avl_tree) {
	assert(ok == avl_tree_insert(&tablep->u.avl, to_be_inserted, returned));
    } else {
	assert(ok == index_obj_insert(&tablep->u.idx, to_be_inserted, returned));
    }
    return 0;
}

PUBLIC error_t
table_search (table_t *tablep,
	datum_t searched, datum_t *returned)
{
    if (tablep->use_avl_tree) {
	return
	    avl_tree_search(&tablep->u.avl, searched, returned);
    }
    return 
	index_obj_search(&tablep->u.idx, searched, returned);
}

PUBLIC error_t
table_remove (table_t *tablep,
	datum_t to_be_removed,
	datum_t *removed)
{
    if (tablep->use_avl_tree) {
	return
	    avl_tree_remove(&tablep->u.avl, to_be_removed, removed);
    }
    return
	index_obj_remove(&tablep->u.idx, to_be_removed, removed);
}

PUBLIC error_t
table_traverse (table_t *tablep, traverse_function_t tfn,
    datum_t p0, datum_t p1, datum_t p2, datum_t p3)
{
    if (tablep->use_avl_tree) {
	return
	    avl_tree_traverse(&tablep->u.avl, tfn, p0, p1, p2, p3);
    }

    return
	index_obj_traverse(&tablep->u.idx, tfn, p0, p1, p2, p3);
}

PUBLIC datum_t *
table_get_all (table_t *tablep, int *returned_count)
{
    if (tablep->use_avl_tree) {
	return
	    avl_tree_get_all(&tablep->u.avl, returned_count);
    }
    return 
	index_obj_get_all(&tablep->u.idx, returned_count);
}

PUBLIC int
table_member_count (table_t *tablep)
{
    return
	(tablep->use_avl_tree) ? 
	    tablep->u.avl.n : tablep->u.idx.n;
}

PUBLIC uint64
table_memory_usage (table_t *tablep, double *mega_bytes)
{
    int64 size;
    
    if (tablep->use_avl_tree) {
        OBJECT_MEMORY_USAGE(&tablep->u.avl, size, *mega_bytes);
    } else {
        OBJECT_MEMORY_USAGE(&tablep->u.idx, size, *mega_bytes);
    }

    return size;
}

PUBLIC void
table_destroy (table_t *tablep)
{
    if (tablep->use_avl_tree) {
	avl_tree_destroy(&tablep->u.avl);
    } else {
	index_obj_destroy(&tablep->u.idx);
    }
}


