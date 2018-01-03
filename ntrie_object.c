
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

    node = (ntrie_node_t*) MEM_MONITOR_ALLOC(ntp, sizeof(ntrie_node_t));
    if (node) {
	node->value = value;
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
	MEM_MONITOR_FREE(ntp, node);
	ntp->node_count--;

	/* go up one more parent & try again */
	node = parent;
    }
}

#if 0

static int
ntrie_node_traverse (ntrie_t *triep, ntrie_node_t *node, 
    trie_traverse_function_t tfn,
    byte *key, int level,
    void *p0, void *p1, void *p2, void *p3)
{ 
    int i, n, index, rc;

    index = level / 2;
    if (level & 1) {
        key[index] |= (node->value << 4);
        if (node->user_data) {
            rc = (tfn)(triep, node, key, index+1, node->user_data,
		    p0, p1, p2, p3);
            if (rc != ok) return error;
        }
    } else {
        key[index] = node->value;
    }

    for (i = n = 0; (i < FASTMAP_ALPHABET_SIZE) && (n < node->n_children); i++) {
	if (node->children[i]) {
	    if (ntrie_node_traverse(triep, node->children[i], tfn, 
		key, level+1, p0, p1, p2, p3) != ok) {
		    return error;
	    }
	    n++;
	}
    }
    return 0;
}

#endif // 0

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
    LOCK_SETUP(ntp);
    MEM_MONITOR_SETUP(ntp);
    ntp->node_count = 0;
    ntrie_node_init(&ntp->ntrie_root, 0);

    return 0;
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

#if 0

PUBLIC int
ntrie_traverse (ntrie_t *ntp, trie_traverse_function_t tfn,
    void *p0, void *p1, void *p2, void *p3)
{
    int i;
    byte *key;
    ntrie_node_t *node;
    int err = ok;

    key = malloc(2 * 8192);
    if (NULL == key) return error;
    node = &ntp->ntrie_root;
    for (i = 0; i < FASTMAP_ALPHABET_SIZE; i++) {
	if (node->children[i]) {
	    if (ntrie_node_traverse(ntp, node->children[i], tfn, 
		key, 0, p0, p1, p2, p3) != ok) {
		    err = error;
		    goto DONE;
	    }
	}
    }
DONE:
    free(key);
    return err;
}

#endif // 0

PUBLIC void
ntrie_destroy (ntrie_t *ntp)
{
    // LATER
}

#ifdef __cplusplus
} // extern C
#endif 


