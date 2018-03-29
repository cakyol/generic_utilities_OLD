
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

#include "chunk_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

static int
chunk_manager_expand (chunk_manager_t *cmgr, int expansion_size)
{
    int i, potential_capacity;
    void **chunks = NULL;           // shut the compiler up
    void *one_chunk;

    /*
     * adjust the stack which holds the free chunks
     */
    if (cmgr->potential_capacity > cmgr->total_chunk_count) {
        expansion_size = cmgr->potential_capacity - cmgr->total_chunk_count;
    } else {
        if (expansion_size <= 0) return ENOSPC;
        potential_capacity = cmgr->total_chunk_count + expansion_size;
        chunks = MEM_REALLOC(cmgr, cmgr->chunks,
                    (sizeof(void*) * potential_capacity));
        if (NULL == chunks) return ENOMEM;
        cmgr->potential_capacity = potential_capacity;
        cmgr->chunks = chunks;
        cmgr->grow_count++;
    }

    /*
     * now pre-allocate the actual chunk to each one of those stack entries
     */
    for (i = 0; i < expansion_size; i++) {
        one_chunk = MEM_MONITOR_ALLOC(cmgr, cmgr->chunk_size);
        if (NULL == one_chunk) break;
        cmgr->chunks[++cmgr->index] = one_chunk;
        cmgr->total_chunk_count++;
    }

    /* if even one chunk was allocated, return ok */
    return 
        i ? 0 : ENOMEM;
}

static void *
thread_unsafe_chunk_manager_alloc (chunk_manager_t *cmgr)
{
    /* there are still chunks available to disburse */
    if (cmgr->index >= 0) {
        return
            cmgr->chunks[cmgr->index--];
    }

    /* no more chunks, see if it can recover */
    if (0 == chunk_manager_expand(cmgr, cmgr->expansion_size)) {
        return
            thread_unsafe_chunk_manager_alloc(cmgr);
    }

    /* not possible */
    return NULL;
}

PUBLIC int
chunk_manager_init (chunk_manager_t *cmgr,
	int make_it_thread_safe,
	int chunk_size, int initial_number_of_chunks, int expansion_size,
	mem_monitor_t *parent_mem_monitor)
{
    int rv;

    /* check some sanity */
    if (chunk_size <= 0) return EINVAL;
    if (initial_number_of_chunks < 0) return EINVAL;
    if (expansion_size < 0) return EINVAL;

    MEM_MONITOR_SETUP(cmgr);
    LOCK_SETUP(cmgr);

    /* aligh chunk size to the next 8 bytes */
    chunk_size += 7;
    chunk_size &= (~7);
    cmgr->chunk_size = chunk_size;

    /* do the rest */
    cmgr->chunks = NULL;
    cmgr->potential_capacity = 0;
    cmgr->total_chunk_count = 0;
    cmgr->index = -1;   /* DONT CHANGE THIS */
    cmgr->expansion_size = expansion_size;
    cmgr->grow_count = -1;
    cmgr->trim_count = 0;
    rv = chunk_manager_expand(cmgr, initial_number_of_chunks);
    WRITE_UNLOCK(cmgr);
    return rv;
}

PUBLIC void *
chunk_manager_alloc (chunk_manager_t *cmgr)
{
    void *rv;

    WRITE_LOCK(cmgr);
    rv = thread_unsafe_chunk_manager_alloc(cmgr);
    WRITE_UNLOCK(cmgr);
    return rv;
}

PUBLIC void
chunk_manager_free (chunk_manager_t *cmgr, void *chunk)
{
    WRITE_LOCK(cmgr);
    cmgr->chunks[++cmgr->index] = chunk;
    WRITE_UNLOCK(cmgr);
}

PUBLIC int
chunk_manager_trim (chunk_manager_t *cmgr)
{
    int current_index = cmgr->index;
    int freed;
    void *chunk;

    WRITE_LOCK(cmgr);
    while (cmgr->index >= 0) {
	chunk = cmgr->chunks[cmgr->index--];
	MEM_MONITOR_FREE(cmgr, chunk);
    }
    freed = current_index - cmgr->index;
    if (freed > 0) {
	cmgr->total_chunk_count -= freed;
	cmgr->trim_count++;
    }
    WRITE_UNLOCK(cmgr);
    return 
	freed;
}

/*
 * The object cannot be destroyed if there are any chunks outstanding
 * in the system which has not been returned back to the object.
 */
PUBLIC int
chunk_manager_destroy (chunk_manager_t *cmgr)
{
    int total_elements;

    WRITE_LOCK(cmgr);
    if (cmgr->index < (cmgr->total_chunk_count - 1)) {
	WRITE_UNLOCK(cmgr);
	return EBUSY;
    }
    total_elements = cmgr->index + 1;
    assert(total_elements == chunk_manager_trim(cmgr));
    MEM_MONITOR_FREE(cmgr, cmgr->chunks);
    WRITE_UNLOCK(cmgr);
    lock_obj_destroy(cmgr->lock);

    return 0;
}

#ifdef __cplusplus
} // extern C
#endif 







