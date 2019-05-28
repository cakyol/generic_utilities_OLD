
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

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <assert.h>

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"
#include "chunk_manager.h"

typedef struct avl_node_s avl_node_t;

struct avl_node_s {

    avl_node_t *parent;         // parent of this node
    avl_node_t *left, *right;   // children
    void *user_data;            // opaque user data
    int balance;
};

typedef struct avl_tree_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    avl_node_t *root_node;
    object_comparer cmpf;
    int cannot_be_modified;
    int n;

} avl_tree_t;

/**************************** Inlines ****************************************/

static inline int
avl_tree_size (avl_tree_t *tree)
{ return tree->n; }

/**************************** Initialize *************************************/

extern int 
avl_tree_init (avl_tree_t *tree,
        int make_it_thread_safe,
        object_comparer cmpf,
        mem_monitor_t *parent_mem_monitor);

/**************************** Insert *****************************************/

extern int 
avl_tree_insert (avl_tree_t *tree,
        void *data_to_be_inserted,
        void **data_already_present);

/**************************** Search *****************************************/

extern int 
avl_tree_search (avl_tree_t *tree,
        void *data_to_be_searched,
        void **data_found);

/**************************** Remove *****************************************/

extern int 
avl_tree_remove (avl_tree_t *tree,
        void *data_to_be_removed,
        void **data_actually_removed);

/**************************** Get all entries ********************************/

extern void **
avl_tree_get_all (avl_tree_t *tree, int *returned_count);

/**************************** Traverse ***************************************/

extern int
avl_tree_traverse (avl_tree_t *tree,
        traverse_function_pointer tfn,
        void *p0, void *p1, void *p2, void *p3);

/**************************** Destroy ****************************************/

/*
 * Note that this destroys ONLY the contents of the object, NOT
 * the object itself, since it is not known whether this object
 * was statically or dynamically created.  It is up to the user
 * to free up the object itself (or not).  Note however that once
 * the object is destroyed, it is rendered unusable and MUST be
 * re-initialized if it needs to be reused again.
 */
extern void 
avl_tree_destroy (avl_tree_t *tree,
        destruction_handler_t dcbf, void *extra_arg);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __AVL_TREE_OBJECT_H__


