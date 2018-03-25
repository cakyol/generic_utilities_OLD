
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

    for (i = 0; i < expansion_size; i++) {
        one_chunk = MEM_MONITOR_ALLOC(cmgr, cmgr->chunk_size);
        if (NULL == one_chunk) break;
        chunks[++cmgr->index] = one_chunk;
        cmgr->total_chunk_count++;
    }

    /* if even one chunk was allocated, return ok */
    return 
        i ? 0 : ENOMEM;
}

static int
thread_unsafe_chunk_manager_init (chunk_manager_t *cmgr,
        int chunk_size, int initial_number_of_chunks, int expansion_size,
        mem_monitor_t *parent_mem_monitor)
{
    if (chunk_size <= 0) return EINVAL;
    MEM_MONITOR_SETUP(cmgr);
    cmgr->chunks = NULL;
    cmgr->chunk_size = chunk_size;
    cmgr->potential_capacity = 0;
    cmgr->total_chunk_count = 0;
    cmgr->index = -1;   /* DONT CHANGE THIS */
    cmgr->expansion_size = expansion_size;
    cmgr->grow_count = -1;
    cmgr->trim_count = 0;
    return
        chunk_manager_expand(cmgr, initial_number_of_chunks);
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

    LOCK_SETUP(cmgr);
    rv = thread_unsafe_chunk_manager_init(cmgr,
	    chunk_size, initial_number_of_chunks,
	    expansion_size, parent_mem_monitor);
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

/*
 * This is an interesting function.  It should be called whenever
 * the user thinks that his/her system has 'settled' into a stable
 * state such that no more chunk allocations & deallocations will
 * happen.  In such a state, if the chunk manager is holding on to
 * a lot of unused chunks, they can all be freed up saving memory.
 * However, this is completely under the control of the user.
 * If it gets mistakenly called, the object may start behaving
 * really inefficiently & thrashing between freeing up chunks and
 * reallocating them coz the user asks for them again.
 *
 * When this function executes, it frees up ALL the unused
 * chunks which are free on the stack.  Not even a single spare chunk
 * will be left.  All of them (if any) will be returned back to
 * the system.  Therefore, use it very carefully.
 *
 * The function return value indicates how many chunks have actually
 * been returned to the system.
 */
PUBLIC int
chunk_manager_trim (chunk_manager_t *cmgr)
{
    int current_index = cmgr->index;
    int relinquished;
    void *chunk;

    WRITE_LOCK(cmgr);
    while (cmgr->index >= 0) {
	chunk = cmgr->chunks[cmgr->index--];
	MEM_MONITOR_FREE(cmgr, chunk);
    }
    relinquished = current_index - cmgr->index;
    if (relinquished > 0) {
	cmgr->total_chunk_count -= relinquished;
	cmgr->trim_count++;
    }
    WRITE_UNLOCK(cmgr);
    return 
	relinquished;
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







