
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
** @@@@@ TRIE OBJECT
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#include "trie_object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

static trie_node_t *
trie_new_child (trie_t *triep, int value)
{
    trie_node_t *node;
    
    node = memory_allocate(triep->memp, sizeof(trie_node_t));
    if (NULL == node) return NULL;
    node->children = memory_allocate(triep->memp,
	sizeof(trie_node_t*) * triep->alphabet_size);
    if (NULL == node->children) {
	memory_free(triep->memp, node);
	return NULL;
    }
    node->value = value;
    triep->node_count++;
    return node;
}

static trie_node_t *
trie_add_child (trie_t *triep, trie_node_t *parent, int value)
{
    trie_node_t *node;
    int index;

    index = triep->tic(value);
    node = parent->children[index];

    /* an entry already exists */
    if (node) {
	return node;
    }

    /* new entry */
    node = trie_new_child(triep, value);
    if (node) {
	node->parent = parent;
	parent->children[index] = node;
	parent->n_children++;
    }

    return node;
}

static trie_node_t *
trie_node_insert (trie_t *triep, byte *key, int key_length)
{
    trie_node_t *new_node = NULL, *parent;

    parent = &triep->root_node;
    while (key_length-- > 0) {
	new_node = trie_add_child(triep, parent, *key);
	if (new_node) {
	    parent = new_node;
	    key++;
	} else {
	    return NULL;
	}
    }
    return new_node;
}

static trie_node_t *
trie_node_find (trie_t *triep, byte *key, int key_length)
{
    trie_node_t *node = &triep->root_node;
    int index;

    while (node) {

	index = triep->tic(*key);
	node = node->children[index];
	if (NULL == node) return NULL;

	/* have we seen the entire pattern */
	if (--key_length <= 0) return node;

	/* next byte */
	key++;
    }

    /* not found */
    return NULL;
}

static error_t
trie_node_traverse (trie_t *triep, trie_node_t *node, 
    trie_traverse_function_pointer tfn,
    byte *key, int level,
    void *p0, void *p1, void *p2, void *p3)
{ 
    int i, n, rc;

    key[level] = node->value;
    if (node->user_data) {

        /* length of key is level+1 */
	rc = (tfn)(triep, node, key, level+1, node->user_data,
		p0, p1, p2, p3);

        /* stop traversing */
        if (rc != ok) return error;

    }
    for (i = n = 0; (i < triep->alphabet_size) && (n < node->n_children); i++) {
	if (node->children[i]) {
	    if (trie_node_traverse(triep, node->children[i], tfn,
		key, level+1, p0, p1, p2, p3) != ok)
		    return error;
	    n++;
	}
    }
    return ok;
}

/*
** This is tricky, be careful
*/
static void 
trie_remove_node (trie_t *triep, trie_node_t *node)
{
    trie_node_t *parent;
    int index;

    /* do NOT delete root node, that is the ONLY one with no parent */
    while (node->parent) {

	/* if I am DIRECTLY in use, I cannot be deleted */
	if (node->user_data) return;

	/* if I am IN-DIRECTLY in use, I still cannot be deleted */
	if (node->n_children > 0) return;

	/* clear the parent pointer which points to me */
	parent = node->parent;
	index = triep->tic(node->value);
	parent->children[index] = NULL;
	parent->n_children--;

	/* delete myself */
	memory_free(triep->memp, node->children);
	memory_free(triep->memp, node);
	(triep->node_count)--;

	/* go up one more parent & try again */
	node = parent;
    }
}

/********************************** PUBLIC **********************************/

PUBLIC error_t 
trie_init (trie_t *triep, int alphabet_size, trie_index_converter tic,
	mem_monitor_t *parent_mem_monitor)
{
    mem_monitor_init(&triep->memory);
    triep->memp = 
	parent_mem_monitor ? parent_mem_monitor : &triep->memory;
    triep->node_count = 0;
    triep->alphabet_size = alphabet_size;
    triep->tic = tic;
    memset(&triep->root_node, 0, sizeof(trie_node_t));
    triep->root_node.children = 
	memory_allocate(triep->memp, (sizeof(trie_node_t*) * triep->alphabet_size));
    if (NULL == triep->root_node.children) return error;
    return ok;
}

PUBLIC error_t
trie_insert (trie_t *triep, void *key, int key_length, 
    void *user_data, void **found_data)
{
    trie_node_t *node;

    /* assume failure */
    SAFE_POINTER_SET(found_data, NULL);

    node = trie_node_insert(triep, key, key_length);
    if (node) {
        if (node->user_data) {
            SAFE_POINTER_SET(found_data, node->user_data);
        } else {
            node->user_data = user_data;
        }
        return ok;
    }

    return error;
}

PUBLIC error_t
trie_search (trie_t *triep, void *key, int key_length, void **found_data)
{
    trie_node_t *node;
    
    /* assume failure */
    SAFE_POINTER_SET(found_data, NULL);

    node = trie_node_find(triep, key, key_length);
    if (node && node->user_data) {
        SAFE_POINTER_SET(found_data, node->user_data);
	return ok;
    }

    return error;
}

PUBLIC error_t
trie_traverse (trie_t *triep, trie_traverse_function_pointer tfn,
    void *p0, void *p1, void *p2, void *p3)
{
    int i, level;
    byte *key;
    trie_node_t *node;
    error_t err = ok;

    key = malloc(2 * 8192);
    if (NULL == key) return error;
    level = 0;
    node = &triep->root_node;
    for (i = 0; i < triep->alphabet_size; i++) {
	if (node->children[i]) {
	    if (trie_node_traverse(triep, node->children[i], tfn, key, level,
		p0, p1, p2, p3) != ok) {
		    err = error;
		    goto DONE;
	    }
	}
    }
DONE:
    free(key);
    return err;
}

PUBLIC int 
trie_remove (trie_t *triep, void *key, int key_length, void **removed_data)
{
    trie_node_t *node;

    /* assume failure */
    SAFE_POINTER_SET(removed_data, NULL);

    node = trie_node_find(triep, key, key_length);
    if (node && node->user_data) {
        SAFE_POINTER_SET(removed_data, node->user_data);
        node->user_data = NULL;
	trie_remove_node(triep, node);
	return ok;
    }

    return error;
}

PUBLIC int
trie_node_size (trie_t *triep)
{
    return
    	sizeof(trie_node_t) +
	triep->alphabet_size * sizeof(trie_node_t*);
}

PUBLIC void
trie_reset (trie_t *triep)
{
    // LATER
}

PUBLIC void
trie_destroy (trie_t *triep)
{
    // LATER
}

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ NIBBLE BASED TRIE: 'ntrie' object, alphabet size is always 16
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

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

    node = (ntrie_node_t*) memory_allocate(ntp->memp, sizeof(ntrie_node_t));
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
	memory_free(ntp->memp, node);
	ntp->node_count--;

	/* go up one more parent & try again */
	node = parent;
    }
}

static error_t
ntrie_node_traverse (ntrie_t *triep, ntrie_node_t *node, 
    trie_traverse_function_pointer tfn,
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
    return ok;
}

PUBLIC void 
ntrie_init (ntrie_t *ntp, mem_monitor_t *parent_mem_monitor)
{
    mem_monitor_init(&ntp->memory);
    ntp->memp =
	parent_mem_monitor ? parent_mem_monitor : &ntp->memory;
    ntp->node_count = 0;
    ntrie_node_init(&ntp->ntrie_root, 0);
}

PUBLIC error_t
ntrie_insert (ntrie_t *ntp, void *key, int key_length, 
    void *user_data, void **found_data)
{
    ntrie_node_t *node;

    /* assume failure */
    SAFE_POINTER_SET(found_data, NULL);

    node = ntrie_node_insert(ntp, key, key_length);
    if (node) {
        if (node->user_data) {
            SAFE_POINTER_SET(found_data, node->user_data);
        } else {
            node->user_data = user_data;
        }
        return ok;
    }

    return error;
}

PUBLIC int
ntrie_search (ntrie_t *ntp, void *key, int key_length, void **found_data)
{
    ntrie_node_t *node;
    
    /* assume failure */
    SAFE_POINTER_SET(found_data, NULL);

    node = ntrie_node_find(ntp, key, key_length);
    if (node && node->user_data) {
        SAFE_POINTER_SET(found_data, node->user_data);
	return ok;
    }

    return error;
}

PUBLIC int 
ntrie_remove (ntrie_t *ntp, void *key, int key_length, void **removed_data)
{
    ntrie_node_t *node;

    /* assume failure */
    SAFE_POINTER_SET(removed_data, NULL);

    node = ntrie_node_find(ntp, key, key_length);
    if (node && node->user_data) {
        SAFE_POINTER_SET(removed_data, node->user_data);
        node->user_data = NULL;
	ntrie_remove_node(ntp, node);
	return ok;
    }
    return error;
}

PUBLIC error_t
ntrie_traverse (ntrie_t *ntriep, trie_traverse_function_pointer tfn,
    void *p0, void *p1, void *p2, void *p3)
{
    int i;
    byte *key;
    ntrie_node_t *node;
    error_t err = ok;

    key = malloc(2 * 8192);
    if (NULL == key) return error;
    node = &ntriep->ntrie_root;
    for (i = 0; i < FASTMAP_ALPHABET_SIZE; i++) {
	if (node->children[i]) {
	    if (ntrie_node_traverse(ntriep, node->children[i], tfn, 
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

PUBLIC uint64
ntrie_memory_usage (ntrie_t *ntp, double *mega_bytes)
{
    uint64 size = sizeof(ntrie_t) + ntp->memp->bytes_used;

    SAFE_POINTER_SET(mega_bytes, ((double) size / (double) MEGA));
    return size;
}

PUBLIC void
ntrie_reset (ntrie_t *ntp)
{
    // LATER
}

PUBLIC void
ntrie_destroy (ntrie_t *ntp)
{
    // LATER
}

#ifdef __cplusplus
} // extern C
#endif 


