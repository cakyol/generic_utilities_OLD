
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

#ifndef __TRIE_OBJECT_H__
#define __TRIE_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "utils_common.h"
#include "lock_object.h"

/*
** This trie can be used for ANY object of ANY size.
** You have to specify the "CONSECUTIVE" alphabet size
** and a function which converts from the input byte
** to the 'consecutive' trie alphabet index when
** initializing.
*/

typedef struct trie_node_s trie_node_t;
struct trie_node_s {

    trie_node_t **children;
    trie_node_t *parent;
    datum_t user_data;
    int n_children;
    int value;
};

typedef int (*trie_index_converter)(int);

typedef struct trie_s {

    LOCK_VARIABLES;
    MEM_MON_VARIABLES;

    int alphabet_size;
    trie_index_converter tic;
    int node_count;
    trie_node_t root_node;

} trie_t;

extern error_t 
trie_init (trie_t *triep,
        boolean make_it_thread_safe,
        int alphabet_size, 
        trie_index_converter tic,
        mem_monitor_t *parent_mem_monitor);

extern error_t 
trie_insert (trie_t *triep, void *key, int key_length, 
    datum_t user_data, datum_t *found_data);

extern error_t
trie_search (trie_t *triep, void *key, int key_length, datum_t *found_data);

extern error_t
trie_traverse (trie_t *triep, trie_traverse_function_pointer tfn,
    datum_t p0, datum_t p1, datum_t p2, datum_t p3);

extern error_t 
trie_remove (trie_t *triep, void *key, int key_length, datum_t *removed_data);

extern int
trie_node_size (trie_t *triep);

extern void 
trie_reset (trie_t *triep);

extern void 
trie_destroy (trie_t *triep);

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

/*
** This trie can be used for ANY object of ANY size.
**
** It represents ALL objects simply as a sequence of
** nibbles (half bytes).  This keeps the alphabet size
** always to 16 (4 bits) and thus saving a lot of storage.
**
** Since it uses a node per nibble, operations on a single byte
** always takes 2 actions; One for the LSB nibble and one for 
** the MSB nibble.  This way, anything can be parsed one byte
** at a time.  It doubles the lookup time but saves a lot
** in storage.  But since each lookup is now array based, it
** is extremely fast.  The disadvantage of using 2 traversal
** per byte is completely offseted by the speed of an array
** lookup per each node.
**
** Every lookup takes EXACTLY O(2n) array accesses, where 'n'
** is the length of the key, NOT the number of nodes.  This
** structure is probably the fastest way to insert, find and
** delete a key, without using lots of memory.
*/

#define LOWEST_FASTMAP_ALPHABET_VALUE			0
#define HIGHEST_FASTMAP_ALPHABET_VALUE			(0xF)
#define FASTMAP_ALPHABET_SIZE				\
    (HIGHEST_FASTMAP_ALPHABET_VALUE - LOWEST_FASTMAP_ALPHABET_VALUE + 1)

typedef struct ntrie_node_s ntrie_node_t;

struct ntrie_node_s {

    ntrie_node_t *children [FASTMAP_ALPHABET_SIZE];
    ntrie_node_t* parent;
    datum_t user_data;
    byte value;
    byte n_children;
};

typedef struct ntrie_s {

    mem_monitor_t memory, *memp;
    int node_count;
    ntrie_node_t ntrie_root;

} ntrie_t;

extern void 
ntrie_init (ntrie_t *ntp, mem_monitor_t *parent_mem_monitor);

extern error_t 
ntrie_insert (ntrie_t *ntp, void *key, int key_length, 
    void *user_data, void **found_data);

extern error_t 
ntrie_search (ntrie_t *ntp, void *key, int key_length, void **found_data);

extern error_t 
ntrie_remove (ntrie_t *ntp, void *key, int key_length, void **removed_data);

extern error_t
ntrie_traverse (ntrie_t *triep, trie_traverse_function_pointer tfn,
    void *p0, void *p1, void *p2, void *p3);

extern uint64
ntrie_memory_usage (ntrie_t *ntp, double *mega_bytes);

extern void 
ntrie_reset (ntrie_t *ntp);

extern void 
ntrie_destroy (ntrie_t *ntp);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __TRIE_OBJECT_H__


