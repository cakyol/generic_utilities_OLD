
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

#include "ntrie_object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

#define LO_NIBBLE(value)            ((value) & 0xF)
#define HI_NIBBLE(value)            ((value) >> 4)

static inline void 
ntrie_node_init (ntrie_node_t *node, int value)
{
    memset(node, 0, sizeof(ntrie_node_t));
    node->value = value;
}

static inline ntrie_node_t *
ntrie_new_node (ntrie_t *ntp, int value)
{
    ntrie_node_t *node;

#ifdef USE_CHUNK_MANAGER
    node = chunk_manager_alloc(&ntp->nodes);
#else
    node = (ntrie_node_t*) MEM_MONITOR_ALLOC(ntp, sizeof(ntrie_node_t));
#endif
    if (node) {
	node->value = value;
	node->current = 0;
    }
    return node;
}

static inline ntrie_node_t *
ntrie_add_nibble (ntrie_t *ntp, ntrie_node_t *parent, int nibble)
{
    ntrie_node_t *node;

    node = parent->children[nibble];

    /* an entry already exists */
    if (node) {
	return node;
    }

    /* new entry */
    node = ntrie_new_node(ntp, nibble);
    if (node) {
	node->parent = parent;
	parent->children[nibble] = node;
        parent->n_children++;
	ntp->node_count++;
    }

    return node;
}

static ntrie_node_t *
ntrie_add_byte (ntrie_t *ntp, ntrie_node_t *parent, int value)
{
    ntrie_node_t *new_node;

    new_node = ntrie_add_nibble(ntp, parent, LO_NIBBLE(value));
    if (NULL == new_node) {
	return NULL;
    }
    new_node = ntrie_add_nibble(ntp, new_node, HI_NIBBLE(value));
    return new_node;
}

static ntrie_node_t *
ntrie_node_insert (ntrie_t *ntp, byte *key, int key_length)
{
    ntrie_node_t *new_node = NULL, *parent;

    parent = &ntp->ntrie_root;
    while (key_length-- > 0) {
	new_node = ntrie_add_byte(ntp, parent, *key);
	if (new_node) {
	    parent = new_node;
	    key++;
	} else {
	    return NULL;
	}
    }
    return new_node;
}

static ntrie_node_t *
ntrie_node_find (ntrie_t *ntp, byte *key, int key_length)
{
    ntrie_node_t *node = &ntp->ntrie_root;

    while (1) {

	/* follow hi nibble */
	node = node->children[LO_NIBBLE(*key)];
	if (NULL == node) return NULL;

	/* follow lo nibble */
	node = node->children[HI_NIBBLE(*key)];
	if (NULL == node) return NULL;

	/* have we seen the entire pattern */
	if (--key_length <= 0) return node;

	/* next byte */
	key++;
    }

    /* not found */
    return NULL;
}

/*
** This is tricky, be careful
*/
static void 
ntrie_remove_node (ntrie_t *ntp, ntrie_node_t *node)
{
    ntrie_node_t *parent;

    /* do NOT delete root node, that is the ONLY one with no parent */
    while (node->parent) {

	/* if I am DIRECTLY in use, I cannot be deleted */
	if (node->user_data) return;

	/* if I am IN-DIRECTLY in use, I still cannot be deleted */
	if (node->n_children > 0) return;

	/* clear the parent pointer which points to me */
	parent = node->parent;
	parent->children[node->value] = NULL;
        parent->n_children--;

	/* delete myself */
#ifdef USE_CHUNK_MANAGER
	chunk_manager_free(&ntp->nodes, node);
#else
	MEM_MONITOR_FREE(ntp, node);
#endif
	ntp->node_count--;

	/* go up one more parent & try again */
	node = parent;
    }
}

static int
thread_unsafe_ntrie_insert (ntrie_t *ntp,
        void *key, int key_length, 
        void *data_to_be_inserted, void **data_found)
{
    ntrie_node_t *node;

    /* assume failure */
    *data_found = NULL;

    node = ntrie_node_insert(ntp, key, key_length);
    if (node) {
        if (node->user_data) {
            *data_found = node->user_data;
        } else {
            node->user_data = data_to_be_inserted;
        }
        return 0;
    }

    return ENOMEM;
}

static int
thread_unsafe_ntrie_search (ntrie_t *ntp,
        void *key, int key_length,
        void **data_found)
{
    ntrie_node_t *node;
    
    /* assume failure */
    *data_found = NULL;

    node = ntrie_node_find(ntp, key, key_length);
    if (node && node->user_data) {
        *data_found = node->user_data;
	return 0;
    }

    return ENODATA;
}

static int 
thread_unsafe_ntrie_remove (ntrie_t *ntp,
        void *key, int key_length, 
        void **removed_data)
{
    ntrie_node_t *node;

    /* assume failure */
    *removed_data = NULL;

    node = ntrie_node_find(ntp, key, key_length);
    if (node && node->user_data) {
        *removed_data = node->user_data;
        node->user_data = NULL;
	ntrie_remove_node(ntp, node);
	return 0;
    }
    return ENODATA;
}

/**************************** Public *****************************************/

PUBLIC int 
ntrie_init (ntrie_t *ntp,
        int make_it_thread_safe,
        mem_monitor_t *parent_mem_monitor)
{
    int rv = 0;

    MEM_MONITOR_SETUP(ntp);
    LOCK_SETUP(ntp);
    ntp->node_count = 0;
    ntrie_node_init(&ntp->ntrie_root, 0);
#ifdef USE_CHUNK_MANAGER
    rv = chunk_manager_init(&ntp->nodes, 0, sizeof(ntrie_node_t),
	    1024, 1024, ntp->mem_mon_p);
#endif
    WRITE_UNLOCK(ntp);

    return rv;
}

PUBLIC int
ntrie_insert (ntrie_t *ntp,
        void *key, int key_length,
        void *data_to_be_inserted, void **data_found)
{
    int rv;

    WRITE_LOCK(ntp);
    rv = thread_unsafe_ntrie_insert(ntp, key, key_length,
                data_to_be_inserted, data_found);
    WRITE_UNLOCK(ntp);
    return rv;
}

PUBLIC int
ntrie_search (ntrie_t *ntp,
        void *key, int key_length,
        void **data_found)
{
    int rv;

    READ_LOCK(ntp);
    rv = thread_unsafe_ntrie_search(ntp, key, key_length, data_found);
    READ_UNLOCK(ntp);
    return rv;
}

PUBLIC int
ntrie_remove (ntrie_t *ntp,
        void *key, int key_length,
        void **removed_data)
{
    int rv;

    WRITE_LOCK(ntp);
    rv = thread_unsafe_ntrie_remove(ntp, key, key_length, removed_data);
    WRITE_UNLOCK(ntp);
    return rv;
}

/*
 * This traverse function does not use recursion or a
 * separate stack.  This is very useful for very deep
 * trees where we may run out of stack or memory.  This
 * method will never run out of memory but will just take
 * longer.
 *
 * One very important point here.  Since this traversal
 * changes node values, we cannot stop until we traverse the
 * entire tree so that all nodes are reverted back to normal.
 * Therefore, if the user traverse function returns a non 0,
 * we can NOT stop traversing; we must continue.  However,
 * once the error occurs, we simply stop calling the user
 * function for the rest of the tree.
 */
PUBLIC void
ntrie_traverse (ntrie_t *triep, traverse_function_pointer tfn,
	void *user_param_1, void *user_param_2)
{
    ntrie_node_t *node, *prev;
    byte *key;
    void *key_len;
    int index = 0;
    int rv = 0;

    key = malloc(8192);
    if (NULL == key) return;
    READ_LOCK(triep);
    node = &triep->ntrie_root;
    node->current = 0;
    while (node) {
	if (node->current < NTRIE_ALPHABET_SIZE) {
	    if (index >= 1) {
		if (index & 1) {
		    /* lo nibble */
		    key[(index-1)/2] = node->value;
		} else {
		    /* hi nibble */
		    key[(index-1)/2] |= (node->value << 4);
		}
	    }
	    prev = node;
	    if (node->children[node->current]) {
		node = node->children[node->current];
		index++;
	    }
	    prev->current++;
	} else {
	    // Do your thing with the node here as long as no error occured so far
	    key_len = integer2pointer(index/2);
	    if (node->user_data && (0 == rv)) {
		rv = tfn(triep, node, node->user_data, key, key_len,
		    user_param_1, user_param_2);
	    }
	    node->current = 0;	// Reset counter for next traversal.
	    node = node->parent;
	    index--;
	}
    }
    READ_UNLOCK(triep);
    free(key);
}

PUBLIC void
ntrie_destroy (ntrie_t *ntp)
{
}

#ifdef __cplusplus
} // extern C
#endif 


