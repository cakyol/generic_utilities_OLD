
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

#ifndef __AVL_TREE_OBJECT_H__
#define __AVL_TREE_OBJECT_H__

#include "utils_common.h"
#include "lock_object.h"

#ifdef USE_CHUNK_MANAGER
#include "chunk_manager_object.h"
#endif // USE_CHUNK_MANAGER

typedef struct avl_node_s avl_node_t;

struct avl_node_s {

    avl_node_t *parent;		// parent of this node
    avl_node_t *left, *right;	// children
    datum_t data;		// opaque user data
    int balance;
};

typedef struct avl_tree_s {

    LOCK_VARIABLES;
    avl_node_t *root_node, *first_node, *last_node;
    comparison_function_t cmpf;

#ifdef USE_CHUNK_MANAGER
    chunk_manager_t nodes;
#endif // USE_CHUNK_MANAGER

    mem_monitor_t mem_mon, *mem_mon_p;
    int n;

} avl_tree_t;

static inline int
avl_tree_size (avl_tree_t *tree)
{ return tree->n; }

extern error_t 
avl_tree_init (avl_tree_t *tree,
	boolean make_it_lockable,
	comparison_function_t cmpf,
        mem_monitor_t *parent_mem_monitor);

extern error_t 
avl_tree_insert (avl_tree_t *tree,
	datum_t data,
	datum_t *exists);

extern error_t 
avl_tree_search (avl_tree_t *tree,
	datum_t searched,
	datum_t *found);

extern error_t 
avl_tree_remove (avl_tree_t *tree,
	datum_t data_to_be_removed,
	datum_t *actual_data_removed);

extern error_t
avl_tree_traverse (avl_tree_t *tree,
	traverse_function_t tfn,
	datum_t p0, datum_t p1, datum_t p2, datum_t p3);

extern datum_t *
avl_tree_get_all (avl_tree_t *tree, int *returned_count);

extern void 
avl_tree_destroy (avl_tree_t *tree);

#endif // __AVL_TREE_OBJECT_H__


