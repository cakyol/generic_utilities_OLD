
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

#include "avl_tree_object.h"

static inline avl_node_t *
get_first (avl_node_t *node)
{
    while (node->left) node = node->left;
    return node;
}

static inline avl_node_t *
get_last (avl_node_t *node)
{
    while (node->right) node = node->right;
    return node;
}

static inline void 
set_child (avl_node_t *child, avl_node_t *parent, 
    boolean left)
{
    if (left)
	parent->left = child;
    else
	parent->right = child;
}

static inline avl_node_t *
avl_tree_next (avl_node_t *node)
{
    avl_node_t *parent;

    if (node->right)
	return 
	    get_first(node->right);

    while ((parent = node->parent) && 
	(parent->right == node))
	    node = parent;

    return parent;
}

static inline avl_node_t *
avl_tree_prev (avl_node_t *node)
{
    avl_node_t *parent;

    if (node->left)
	return 
	    get_last(node->left);

    while ((parent = node->parent) && 
	(parent->left == node))
	    node = parent;
	
    return parent;
}

static inline void 
rotate_left (avl_node_t *node, avl_tree_t *tree)
{
    avl_node_t *p = node;
    avl_node_t *q = node->right;
    avl_node_t *parent = p->parent;

    if (parent) {
	if (parent->left == p)
	    parent->left = q;
	else
	    parent->right = q;
    } else
	tree->root_node = q;
    q->parent = parent;
    p->parent = q;
    p->right = q->left;
    if (p->right)
	p->right->parent = p;
    q->left = p;
}

static inline void 
rotate_right (avl_node_t *node, avl_tree_t *tree)
{
    avl_node_t *p = node;
    avl_node_t *q = node->left;
    avl_node_t *parent = p->parent;

    if (parent) {
	if (parent->left == p)
	    parent->left = q;
	else
	    parent->right = q;
    } else
	tree->root_node = q;
    q->parent = parent;
    p->parent = q;
    p->left = q->right;
    if (p->left)
	p->left->parent = p;
    q->right = p;
}

static avl_node_t *
avl_lookup_engine (avl_tree_t *tree,
	datum_t searched,
	avl_node_t **pparent, avl_node_t **unbalanced, 
        boolean *is_left)
{
    avl_node_t *node = tree->root_node;
    int res = 0;

    *pparent = NULL;
    *unbalanced = node;
    *is_left = FALSE;

    while (node) {
	if (node->balance) *unbalanced = node;
	res = (tree->cmpf)(searched, node->data);
	if (res == 0) return node;
	*pparent = node;
	if ((*is_left = (res < 0))) {
	    node = node->left;
	} else {
	    node = node->right;
	}
    }
    return NULL;
}

static inline void
free_avl_node (avl_tree_t *tree, avl_node_t *node)
{
#ifdef USE_CHUNK_MANAGER
    chunk_manager_free(&tree->nodes, node);
#else
    MEM_MONITOR_FREE(tree, node);
#endif
    tree->n--;
}

static inline avl_node_t *
new_avl_node (avl_tree_t *tree, datum_t data)
{
    avl_node_t *node;

#ifdef USE_CHUNK_MANAGER
    node = chunk_manager_alloc(&tree->nodes);
#else
    node = MEM_MONITOR_ALLOC(tree, sizeof(avl_node_t));
#endif
    if (node) {
        node->parent = node->left = node->right = NULL;
        node->balance = 0;
        node->data = data;
        tree->n++;
    }
    return node;
}

/*
 * used to recursively destroy (free up) all nodes below 'node'
 */
static void 
avl_node_destroy_nodes (avl_tree_t *tree,
	avl_node_t *node, 
	boolean leave_parent_consistent)
{
    avl_node_t *parent, *left, *right;

    // end node
    if (NULL == node) return;

    left = node->left;
    right = node->right;

    if (leave_parent_consistent) {
	parent = node->parent;
	if (parent) {
	    if (parent->left == node) {
		parent->left = NULL;
	    } else if (parent->right == node) {
		parent->right = NULL;
	    } else {
		assert(0);
	    }
	} else {
	    tree->root_node = NULL;
	}
    }

    // done with this one.  Freeing this here before we get deep into
    // recursion frees up the nodes much more quickly without building
    // up all the stack of nodes during recursion.
    //
    free_avl_node(tree, node);

    // recurse on; since we will be deleting all these
    // nodes, there is no need to leave the parent's
    // pointers consistent.
    //
    avl_node_destroy_nodes(tree, left, false);
    avl_node_destroy_nodes(tree, right, false);
}

static error_t 
thread_unsafe_avl_tree_insert (avl_tree_t *tree,
    datum_t datum_to_be_inserted,
    datum_t *datum_already_present)
{
    avl_node_t *found, *parent, *unbalanced, *node;
    boolean is_left;

    // assume the entry is not present initially
    NULLIFY_DATUMP(datum_already_present);

    found = avl_lookup_engine(tree, datum_to_be_inserted,
		&parent, &unbalanced, &is_left);

    if (found) {
        *datum_already_present = found->data;
	return 0;
    }

    /* get a new node */
    node = new_avl_node(tree, datum_to_be_inserted);
    if (NULL == node) {
	return ENOMEM;
    }

    if (!parent) {
	tree->root_node = node;
	tree->first_node = tree->last_node = node;
	return 0;
    }

    if (is_left) {
	if (parent == tree->first_node)
	    tree->first_node = node;
    } else {
	if (parent == tree->last_node)
	    tree->last_node = node;
    }
    node->parent = parent;
    set_child(node, parent, is_left);
    for (;;) {
	if (parent->left == node)
	    (parent->balance)--;
	else
	    (parent->balance)++;
	if (parent == unbalanced)
	    break;
	node = parent;
	parent = parent->parent;
    }

    switch (unbalanced->balance) {
    case  1: 
    case -1:
    case 0:
	break;
	
    case 2: 
	{
	    avl_node_t *right = unbalanced->right;

	    if (right->balance == 1) {
		unbalanced->balance = 0;
		right->balance = 0;
	    } else {
		switch (right->left->balance) {
		case 1:
		    unbalanced->balance = -1;
		    right->balance = 0;
		    break;
		case 0:
		    unbalanced->balance = 0;
		    right->balance = 0;
		    break;
		case -1:
		    unbalanced->balance = 0;
		    right->balance = 1;
		    break;
		}
		right->left->balance = 0;
		rotate_right(right, tree);
	    }
	    rotate_left(unbalanced, tree);
	    break;
	}
    case -2: 
	{
	    avl_node_t *left = unbalanced->left;

	    if (left->balance == -1) {
		unbalanced->balance = 0;
		left->balance = 0;
	    } else {
		switch (left->right->balance) {
		case 1:
		    unbalanced->balance = 0;
		    left->balance = -1;
		    break;
		case 0:
		    unbalanced->balance = 0;
		    left->balance = 0;
		    break;
		case -1:
		    unbalanced->balance = 1;
		    left->balance = 0;
		    break;
		}
		left->right->balance = 0;
		rotate_left(left, tree);
	    }
	    rotate_right(unbalanced, tree);
	    break;
	}
    }
    return 0;
}

static error_t 
thread_unsafe_avl_tree_remove (avl_tree_t *tree,
	datum_t data_to_be_removed,
	datum_t *actual_data_removed)
{
    avl_node_t *node, *to_be_deleted;
    avl_node_t *parent, *unbalanced;
    avl_node_t *left;
    avl_node_t *right;
    avl_node_t *next;
    boolean is_left;

    /* find the matching node first */
    node = avl_lookup_engine(tree, data_to_be_removed,
            &parent, &unbalanced, &is_left);

    /* not there */
    if (!node) {
        SAFE_NULLIFY_DATUMP(actual_data_removed);
	return ENODATA;
    }

    /* if we are here, we found it */
    SAFE_DATUMP_SET(actual_data_removed, node->data);

    /* cache it for later freeing */
    to_be_deleted = node;

    parent = node->parent;
    left = node->left;
    right = node->right;

    if (node == tree->first_node)
	tree->first_node = avl_tree_next(node);
    if (node == tree->last_node)
	tree->last_node = avl_tree_prev(node);

    if (!left)
	next = right;
    else if (!right)
	next = left;
    else
	next = get_first(right);

    if (parent) {
	is_left = (parent->left == node);
	set_child(next, parent, is_left);
    } else
	tree->root_node = next;

    if (left && right) {
	next->balance = node->balance;
	next->left = left;
	left->parent = next;
	if (next != right) {
	    parent = next->parent;
	    next->parent = node->parent;
	    node = next->right;
	    parent->left = node;
	    next->right = right;
	    right->parent = next;
	    is_left = TRUE;
	} else {
	    next->parent = parent;
	    parent = next;
	    node = parent->right;
	    is_left = FALSE;
	}
	assert(parent != NULL);
    } else
	node = next;

    if (node)
	node->parent = parent;

    while (parent) {

	int balance;
	    
	node = parent;
	parent = parent->parent;
	if (is_left) {
	    is_left = (parent && (parent->left == node));
	    balance = ++node->balance;
	    if (balance == 0)		/* case 1 */
		continue;
	    if (balance == 1) {		/* case 2 */
		goto END_OF_DELETE;
	    }
	    right = node->right;
	    switch (right->balance) {
	    case 0:				/* case 3.1 */
		node->balance = 1;
		right->balance = -1;
		rotate_left(node, tree);
		goto END_OF_DELETE;
	    case 1:				/* case 3.2 */
		node->balance = 0;
		right->balance = 0;
		break;
	    case -1:			/* case 3.3 */
		switch (right->left->balance) {
		case 1:
		    node->balance = -1;
		    right->balance = 0;
		    break;
		case 0:
		    node->balance = 0;
		    right->balance = 0;
		    break;
		case -1:
		    node->balance = 0;
		    right->balance = 1;
		    break;
		}
		right->left->balance = 0;
		rotate_right(right, tree);
	    }
	    rotate_left(node, tree);
	} else {
	    is_left = (parent && (parent->left == node));
	    balance = --node->balance;
	    if (balance == 0)
		continue;
	    if (balance == -1) {
		goto END_OF_DELETE;
	    }
	    left = node->left;
	    switch (left->balance) {
	    case 0:
		node->balance = -1;
		left->balance = 1;
		rotate_right(node, tree);
		goto END_OF_DELETE;
	    case -1:
		node->balance = 0;
		left->balance = 0;
		break;
	    case 1:
		switch (left->right->balance) {
		case 1:
		    node->balance = 0;
		    left->balance = -1;
		    break;
		case 0:
		    node->balance = 0;
		    left->balance = 0;
		    break;
		case -1:
		    node->balance = 1;
		    left->balance = 0;
		    break;
		}
		left->right->balance = 0;
		rotate_left(left, tree);
	    }
	    rotate_right(node, tree);
	}
    }

END_OF_DELETE:
		
    free_avl_node(tree, to_be_deleted);
    if (tree->n <= 0) {
	assert(tree->root_node == NULL);
    } else {
	assert(tree->root_node != NULL);
    }
    return 0;
}

/*
 * morris traverse: preorder tree traversal without 
 * using recursion and without stack.  No extra storage
 * is needed, therefore it is frugal in memory usage.
 */
error_t
thread_unsafe_morris_traverse (avl_tree_t *tree, avl_node_t *root,
        traverse_function_t tfn,
        datum_t p0, datum_t p1, datum_t p2, datum_t p3)
{
    error_t rv;
    avl_node_t *current;

    /* if the starting root is NULL, start from top of tree */
    if (NULL == root) {
        root = tree->root_node;
    }

    while (root) {
        if (root->left == NULL) {
            rv = tfn(tree, root, root->data, p0, p1, p2, p3);
            if (rv) return rv;
            root = root->right;
        } else {
            current = root->left;
            while (current->right && (current->right != root)) {
                current = current->right;
            }

            if (current->right == root) {
                current->right = NULL;
                root = root->right;
            } else {
                rv = tfn(tree, root, root->data, p0, p1, p2, p3);
                if (rv) return rv;
                current->right = root;
                root = root->left;
            }
        }
    }
    return 0;
}

/**************************** Initialize *************************************/

PUBLIC error_t
avl_tree_init (avl_tree_t *tree,
	boolean make_it_thread_safe,
	comparison_function_t cmpf,
        mem_monitor_t *parent_mem_monitor)
{
    if (NULL == cmpf) return EINVAL;
    LOCK_SETUP(tree);
    MEM_MONITOR_SETUP(tree);
    tree->cmpf = cmpf;

#ifdef USE_CHUNK_MANAGER
    chunk_manager_init(&tree->nodes, 
        false, 
        sizeof(avl_node_t), 256, 256, tree->memp);
#endif // USE_CHUNK_MANAGER

    tree->n = 0;
    tree->root_node = tree->first_node = tree->last_node = NULL;
    WRITE_UNLOCK(tree);

    return 0;
}

/**************************** Insert *****************************************/

PUBLIC error_t
avl_tree_insert (avl_tree_t *tree,
	datum_t datum_to_be_inserted,
	datum_t *datum_already_present)
{
    error_t rv;

    WRITE_LOCK(tree, NULL);
    rv = thread_unsafe_avl_tree_insert(tree,
            datum_to_be_inserted, datum_already_present);
    WRITE_UNLOCK(tree);
    return rv;
}

PUBLIC error_t
avl_tree_insert_integer (avl_tree_t *tree,
        int integer_to_be_inserted,
        int *integer_already_present)
{
    error_t rv;
    datum_t datum_to_be_inserted, datum_already_present;

    datum_to_be_inserted.integer = integer_to_be_inserted;
    rv = avl_tree_insert(tree, datum_to_be_inserted, &datum_already_present);
    *integer_already_present = datum_already_present.integer;
    return rv;
}

PUBLIC error_t
avl_tree_insert_pointer (avl_tree_t *tree,
        void *pointer_to_be_inserted,
        void **pointer_already_present)
{
    error_t rv;
    datum_t datum_to_be_inserted, datum_already_present;

    datum_to_be_inserted.pointer = pointer_to_be_inserted;
    rv = avl_tree_insert(tree, datum_to_be_inserted, &datum_already_present);
    *pointer_already_present = datum_already_present.pointer;
    return rv;
}

/**************************** Search *****************************************/

PUBLIC error_t 
avl_tree_search (avl_tree_t *tree, 
	datum_t datum_to_be_searched,
	datum_t *datum_found)
{
    error_t rv;
    avl_node_t *parent, *unbalanced, *node;
    boolean is_left;

    READ_LOCK(tree);
    node = avl_lookup_engine(tree, datum_to_be_searched, 
                &parent, &unbalanced, &is_left);
    if (node) {
        *datum_found = node->data;
	rv = 0;
    } else {
	NULLIFY_DATUMP(datum_found);
	rv = ENODATA;
    }
    READ_UNLOCK(tree);
    return rv;
}

PUBLIC error_t
avl_tree_search_integer (avl_tree_t *tree,
        int integer_to_be_searched,
        int *integer_found)
{
    error_t rv;
    datum_t datum_to_be_searched, datum_found;

    datum_to_be_searched.integer = integer_to_be_searched;
    rv = avl_tree_search(tree, datum_to_be_searched, &datum_found);
    *integer_found = datum_found.integer;
    return rv;
}

PUBLIC error_t
avl_tree_search_pointer (avl_tree_t *tree,
        void *pointer_to_be_searched,
        void **pointer_found)
{
    error_t rv;
    datum_t datum_to_be_searched, datum_found;

    datum_to_be_searched.pointer = pointer_to_be_searched;
    rv = avl_tree_search(tree, datum_to_be_searched, &datum_found);
    *pointer_found = datum_found.pointer;
    return rv;
}

/**************************** Remove *****************************************/

PUBLIC error_t
avl_tree_remove (avl_tree_t *tree,
	datum_t datum_to_be_removed,
	datum_t *datum_actually_removed)
{
    error_t rv;

    WRITE_LOCK(tree, NULL);
    rv = thread_unsafe_avl_tree_remove(tree,
		datum_to_be_removed, datum_actually_removed);
    WRITE_UNLOCK(tree);
    return rv;
}

PUBLIC error_t
avl_tree_remove_integer (avl_tree_t *tree,
        int integer_to_be_removed,
        int *integer_actually_removed)
{
    error_t rv;
    datum_t datum_to_be_removed, datum_actually_removed;

    datum_to_be_removed.integer = integer_to_be_removed;
    rv = avl_tree_remove(tree, datum_to_be_removed, &datum_actually_removed);
    *integer_actually_removed = datum_actually_removed.integer;
    return rv;
}

PUBLIC error_t
avl_tree_remove_pointer (avl_tree_t *tree,
        void *pointer_to_be_removed,
        void **pointer_actually_removed)
{
    error_t rv;
    datum_t datum_to_be_removed, datum_actually_removed;

    datum_to_be_removed.pointer = pointer_to_be_removed;
    rv = avl_tree_remove(tree, datum_to_be_removed, &datum_actually_removed);
    *pointer_actually_removed = datum_actually_removed.pointer;
    return rv;
}

/**************************** Get all entries ********************************/

///// static void
///// avl_node_get_all (avl_node_t *node, datum_t *storage_area, int *index)
///// {
/////     if (NULL == node) return;
/////     storage_area[*index] = node->data;
/////     (*index)++;
/////     avl_node_get_all(node->left, storage_area, index);
/////     avl_node_get_all(node->right, storage_area, index);
///// }
///// 
///// PUBLIC datum_t *
///// avl_tree_get_all (avl_tree_t *tree, int *returned_count)
///// {
/////     datum_t *storage_area;
/////     int index = 0;
///// 
/////     READ_LOCK(tree);
/////     storage_area = malloc((tree->n + 1) * sizeof(datum_t));
/////     if (NULL == storage_area) {
///// 	*returned_count = 0;
/////     } else {
///// 	avl_node_get_all(tree->root_node, storage_area, &index);
///// 	*returned_count = index;
/////     }
/////     READ_UNLOCK(tree);
/////     return storage_area;
///// }

/*
 * This is much faster & more efficient with morris traverse
 */
PUBLIC datum_t *
avl_tree_get_all (avl_tree_t *tree, int *returned_count)
{
    datum_t *storage_area;
    int index;
    avl_node_t *root, *current;

    READ_LOCK(tree);
    storage_area = malloc((tree->n + 1) * sizeof(datum_t));
    if (NULL == storage_area) {
        *returned_count = 0;
        READ_UNLOCK(tree);
        return NULL;
    }

    index = 0;
    root = tree->root_node;
    while (root) {
        if (root->left == NULL) {
            storage_area[index++] = root->data;
            root = root->right;
        } else {
            current = root->left;
            while (current->right && (current->right != root)) {
                current = current->right;
            }
            if (current->right == root) {
                current->right = NULL;
                root = root->right;
            } else {
                storage_area[index++] = root->data;
                current->right = root;
                root = root->left;
            }
        }
    }
    *returned_count = index;
    READ_UNLOCK(tree);

    return storage_area;
}

PUBLIC int *
avl_tree_get_all_integers (avl_tree_t *tree, int *returned_count)
{
    *returned_count = 0;
    return NULL;
}

PUBLIC void **
avl_tree_get_all_pointers (avl_tree_t *tree, int *returned_count)
{
    *returned_count = 0;
    return NULL;
}

/**************************** Traverse ***************************************/

///// static error_t
///// thread_unsafe_recursive_traverse (avl_tree_t *tree,
///// 	avl_node_t *node,
///// 	traverse_function_t tfn,
///// 	datum_t p0, datum_t p1, datum_t p2, datum_t p3)
///// {
/////     // end of branch
/////     if (NULL == node)
///// 	return 0;
///// 
/////     // apply traverse function to current node
/////     if (FAILED((tfn)(tree, node, node->data, p0, p1, p2, p3))) 
///// 	return EFAULT;
///// 
/////     // traverse recursively left subtree
/////     if (FAILED(thread_unsafe_recursive_traverse(tree, node->left, tfn,
///// 		p0, p1, p2, p3)))
///// 	return EFAULT;
///// 
/////     // traverse recursively right subtree
/////     return
///// 	thread_unsafe_recursive_traverse(tree, node->right, tfn, p0, p1, p2, p3);
///// 
///// }
///// 
///// PUBLIC error_t
///// avl_tree_recursive_traverse (avl_tree_t *tree,
///// 	traverse_function_t tfn,
///// 	datum_t p0, datum_t p1, datum_t p2, datum_t p3)
///// {
/////     error_t rv;
///// 
/////     READ_LOCK(tree);
/////     rv = thread_unsafe_recursive_traverse(tree, tree->root_node,
///// 		tfn, p0, p1, p2, p3);
/////     READ_UNLOCK(tree);
/////     return rv;
///// }

PUBLIC error_t
avl_tree_traverse (avl_tree_t *tree,
	traverse_function_t tfn,
	datum_t p0, datum_t p1, datum_t p2, datum_t p3)
{
    error_t rv;

    READ_LOCK(tree);
    rv = thread_unsafe_morris_traverse(tree, tree->root_node,
		tfn, p0, p1, p2, p3);
    READ_UNLOCK(tree);
    return rv;
}

/**************************** Destroy ****************************************/

PUBLIC void
avl_tree_destroy (avl_tree_t *tree)
{
    WRITE_LOCK(tree, NULL);
    avl_node_destroy_nodes(tree, tree->root_node, false);
    assert(tree->n == 0);
    tree->root_node = tree->first_node = tree->last_node = NULL;
    tree->cmpf = NULL;
    WRITE_UNLOCK(tree);
    LOCK_OBJ_DESTROY(tree);
}


