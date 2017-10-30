
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
    MEM_MON_VARIABLES;

    avl_node_t *root_node, *first_node, *last_node;
    comparison_function_t cmpf;

#ifdef USE_CHUNK_MANAGER
    chunk_manager_t nodes;
#endif // USE_CHUNK_MANAGER

    int n;

} avl_tree_t;

/**************************** Inlines ****************************************/

static inline int
avl_tree_size (avl_tree_t *tree)
{ return tree->n; }

/**************************** Initialize *************************************/

extern error_t 
avl_tree_init (avl_tree_t *tree,
	boolean make_it_thread_safe,
	comparison_function_t cmpf,
        mem_monitor_t *parent_mem_monitor);

/**************************** Insert *****************************************/

extern error_t 
avl_tree_insert (avl_tree_t *tree,
	datum_t datum_to_be_inserted,
	datum_t *datum_already_present);

error_t
avl_tree_insert_integer (avl_tree_t *tree,
        int integer_to_be_inserted,
        int *integer_already_present);

error_t
avl_tree_insert_pointer (avl_tree_t *tree,
        void *pointer_to_be_inserted,
        void **pointer_already_present);

/**************************** Search *****************************************/

extern error_t 
avl_tree_search (avl_tree_t *tree,
	datum_t datum_to_be_searched,
	datum_t *datum_found);

error_t
avl_tree_search_integer (avl_tree_t *tree,
        int integer_to_be_searched,
        int *integer_found);

error_t
avl_tree_search_pointer (avl_tree_t *tree,
        void *pointer_to_be_searched,
        void **pointer_found);

/**************************** Remove *****************************************/

extern error_t 
avl_tree_remove (avl_tree_t *tree,
	datum_t datum_to_be_removed,
	datum_t *datum_actually_removed);

extern error_t
avl_tree_remove_integer (avl_tree_t *tree,
        int integer_to_be_removed,
        int *integer_actually_removed);

extern error_t
avl_tree_remove_pointer (avl_tree_t *tree,
        void *pointer_to_be_removed,
        void **pointer_actually_removed);

/**************************** Get all entries ********************************/

extern datum_t *
avl_tree_get_all (avl_tree_t *tree, int *returned_count);

extern int *
avl_tree_get_all_integers (avl_tree_t *tree, int *returned_count);

extern void **
avl_tree_get_all_pointers (avl_tree_t *tree, int *returned_count);

/**************************** Traverse ***************************************/

extern error_t
avl_tree_traverse (avl_tree_t *tree,
	traverse_function_t tfn,
	datum_t p0, datum_t p1, datum_t p2, datum_t p3);

/**************************** Destroy ****************************************/

extern void 
avl_tree_destroy (avl_tree_t *tree);

#endif // __AVL_TREE_OBJECT_H__


