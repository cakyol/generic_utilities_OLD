
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
** @@@@@ NIBBLE RADIX TREE/TRIE
**
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
** Every lookup takes EXACTLY O(2k) array accesses, where 'k'
** is the length of the key, NOT the number of nodes.  This
** structure is probably the fastest way to insert, find and
** delete data.  Also remember that these are not 'comparisons'
** but very fast array accesses.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __NTRIE_OBJECT_H__
#define __NTRIE_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

#include "pointer_manipulations.h"
#include "mem_monitor.h"
#include "lock_object.h"
#include "function_types.h"

#ifdef USE_CHUNK_MANAGER
#include "chunk_manager_object.h"
#endif

#define NTRIE_LOW_VALUE         0
#define NTRIE_HI_VALUE		(0xF)
#define NTRIE_ALPHABET_SIZE     (NTRIE_HI_VALUE - NTRIE_LOW_VALUE + 1)

typedef struct ntrie_node_s ntrie_node_t;

struct ntrie_node_s {

    ntrie_node_t* parent;
    ntrie_node_t *children [NTRIE_ALPHABET_SIZE];
    void *user_data;
    byte current;	// used for non recursive traversal
    byte value;
    byte n_children;
};

typedef struct ntrie_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;
#ifdef USE_CHUNK_MANAGER
    chunk_manager_t nodes;
#endif
    int node_count;
    ntrie_node_t ntrie_root;

} ntrie_t;

extern int 
ntrie_init (ntrie_t *ntp, 
        int make_it_thread_safe,
        mem_monitor_t *parent_mem_monitor);

extern int 
ntrie_insert (ntrie_t *ntp,
        void *key, int key_length, 
        void *data_to_be_inserted,
        void **data_found);

extern int 
ntrie_search (ntrie_t *ntp, 
        void *key, int key_length, 
        void **data_found);

extern int 
ntrie_remove (ntrie_t *ntp, 
        void *key, int key_length, 
        void **data_removed);

extern void
ntrie_traverse (ntrie_t *ntp, traverse_function_pointer tfn,
	void *user_param_1, void *user_param_2);

extern void 
ntrie_destroy (ntrie_t *ntp);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __NTRIE_OBJECT_H__


