
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol, gee.akyol@gmail.com, gee_akyol@yahoo.com
** Copyright: Cihangir Metin Akyol, April 2014 -> ....
**
** All this code has been personally developed by and belongs to 
** Mr. Cihangir Metin Akyol.  It has been developed in his own 
** personal time using his own personal resources.  Therefore,
** it is NOT owned by any establishment, group, company or 
** consortium.  It is the sole property and work of the named
** individual.
**
** It CAN be used by ANYONE or ANY company for ANY purpose as long 
** as ownership and/or patent claims are NOT made to it by ANYONE
** or ANY ENTITY.
**
** It ALWAYS is and WILL remain the sole property of Cihangir Metin Akyol.
**
** For proper indentation/viewing, regardless of which editor is being used,
** no tabs are used, ONLY spaces are used and the width of lines never
** exceed 80 characters.  This way, every text editor/terminal should
** display the code properly.  If modifying, please stick to this
** convention.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __ANOTHER_MALLOC_H__
#define __ANOTHER_MALLOC_H__

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * When allocating, the correct pool with the minimum acceptable
 * size is found and the chunk is allocated from the head of its
 * free list ('head').  Also when freeing a chunk, it is also
 * returned back to the 'head' of the pool.  This makes it extremely
 * fast.
 */

typedef struct size_count_tuple_s {

    int size;
    int count;

} size_count_tuple_t;

typedef struct chunk_s chunk_t;
typedef struct mempool_s mempool_t;

/*
 * A memory block/chunk allocated from a pool.
 * The user sees ONLY the part past the 'start'.
 * This will be 8 byte aligned.
 *
 * The 'next' pointer simply points to the next free available
 * chunk.  The 'poolp' points to the pool this chunk belongs to,
 * where it should be returned to when freed.  This makes it
 * a very fast free operation to simply re-insert it to the head
 * of the free list of that pool.
 *
 * Note that user does NOT see or need anything before 'start'.
 */
struct chunk_s {

    /* the memory pool this chunk belongs to (used when freeing) */
    mempool_t *poolp;

    /* next free chunk in the list (used for allocating) */
    chunk_t *next;

    /*
     * This is what the user sees, ONLY,
     * an 8 byte aligned chunk of memory
     */
    unsigned char data [0] __attribute__((aligned(8)));
};

/*
 * Defines ONE memory pool
 */
struct mempool_s {
    
    /*
     * The original chunk size the user requested.
     * Note that as the pool is populated, this will be expanded
     * to the next multiple of 8 bytes.
     */
    int user_requested_size;

    /*
     * Adjusted size of each chunk after the user requested size
     * has been increased to the next multiple of 8 bytes AND the
     * pre chunk header has been added to it.
     */
    int chunk_size;

    /* how many chunks this pool has */
    int chunk_count;

    /*
     * The entire malloced block for this pool.
     * When destroying a pool, the entire set of
     * chunks can be destroyed just by freeing this single
     * block.  One does not have to free traversing
     * any lists, one chunk at a time.
     */
    void *block;

    /* linked list of all the chunks */
    chunk_t *head;
};

/*
 * Defines a SET of memory pools
 */
typedef struct memory_s {

    /* how many pools are in this memory object */
    int num_pools;

    /* an array of pools, size defined at initialization time */
    mempool_t *pools;

} memory_t;

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* __ANOTHER_MALLOC_H__ */


