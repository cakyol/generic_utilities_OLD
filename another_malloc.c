
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

#include "another_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * initialize a pool with all its chunks.  Each chunk can hold
 * data of size 'size' and there will be 'count' of these chunks
 * in this pool.
 *
 * 0 will be returned if successful, else an error code.
 */
static int
initialize_one_pool (mempool_t *poolp, int size, int count)
{
    int i, total_size, aligned_data_size, chunk_size;
    byte *block, *ptr;
    chunk_t *chunkp;

    /* empty the pool first */
    memset(poolp, 0, sizeof(mempool_t));

    /* sanity */
    if (size <= 0) return EINVAL;
    if (count <= 0) return EINVAL;

    /* adjusted sizes */
    aligned_data_size = (size + 7) & ~7;
    chunk_size = aligned_data_size + sizeof(chunk_t);
    total_size = chunk_size * count;

    /* allocate the big block for the entire pool and fill fields */
    block = malloc(total_size);
    poolp->user_requested_size = size;
    poolp->chunk_size = chunk_size;
    poolp->chunk_count = count;
    poolp->block = block;
    poolp->head = (chunk_t*) block;

    /* cannot proceed if so */
    if (NULL == block) return ENOMEM;

    /* now partition each chunk in the big block */
    ptr = block;
    for (i = 0; i < count; i++) {
        chunkp = (chunk_t*) ptr;
        chunkp->poolp = poolp;
        ptr += chunk_size;
        chunkp->next = (chunk_t*) ptr;
    }

    /* last chunk */
    chunkp->next = NULL;

    return 0;
}

PUBLIC int
memory_initialize (memory_t *memp,
        bool make_it_thread_safe,
        size_count_tuple_t tuples [], int count)
{
    int i;

    if (count <= 0) return EINVAL;
    
    /*
     *
     *
     * IMPORTANT: SORT THE POOLS BASED ON SIZE
     *
     *
     */

    memp->num_pools = 0;
    memp->pools = malloc(count * sizeof(mempool_t));
    if (NULL == memp->pools) return ENOMEM;
    memp->num_pools = count;
    for (i = 0; i < count; i++) {
        initialize_one_pool(&memp->pools[i],
            tuples[i].size, tuples[i].count);
    }

    return 0;
}

PUBLIC void *
memory_allocate (memory_t *memp, int size)
{
    int i;
    mempool_t *poolp;
    chunk_t *chunkp;

    /*
     * pools are ordered based on size so that if a
     * particular sized pool is exhausted, a chunk is
     * allocated from the next higher sized pool.
     */
    for (i = 0; i < memp->num_pools; i++) {
        poolp = &memp->pools[i];
        if ((poolp->user_requested_size >= size) && poolp->head) {
            chunkp = poolp->head;
            poolp->head = chunkp->next;
            return &chunkp->data[0];
        }
    }
    return NULL;
}

PUBLIC void
memory_free (void *ptr)
{
    byte *bptr = ((byte*) ptr) - (int) (sizeof(chunk_t));
    chunk_t *chunkp = (chunk_t*) bptr;

    chunkp->next = chunkp->poolp->head;
    chunkp->poolp->head = chunkp;
}

PUBLIC void
memory_destroy (memory_t *memp)
{
    int i;

    for (i = 0; i < memp->num_pools; i++) {
        free(memp->pools[i].block);
    }
    free(memp->pools);
    memp->pools = NULL;
    memp->num_pools = 0;
}

#ifdef __cplusplus
} // extern C
#endif


