
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

#include "chunk_manager_object.h"

static error_t
chunk_manager_expand (chunk_manager_t *cmgr, int expansion_size)
{
    int i, potential_capacity;
    void **chunks;
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

static error_t
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
    if (SUCCEEDED(chunk_manager_expand(cmgr, cmgr->expansion_size))) {
        return
            thread_unsafe_chunk_manager_alloc(cmgr);
    }

    /* not possible */
    return NULL;
}

PUBLIC error_t
chunk_manager_init (chunk_manager_t *cmgr,
	boolean make_it_thread_safe,
	int chunk_size, int initial_number_of_chunks, int expansion_size,
	mem_monitor_t *parent_mem_monitor)
{
    error_t rv;

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

    WRITE_LOCK(cmgr, NULL);
    rv = thread_unsafe_chunk_manager_alloc(cmgr);
    WRITE_UNLOCK(cmgr);
    return rv;
}

PUBLIC void
chunk_manager_free (chunk_manager_t *cmgr, void *chunk)
{
    WRITE_LOCK(cmgr, NULL);
    cmgr->chunks[++cmgr->index] = chunk;
    WRITE_UNLOCK(cmgr);
}

PUBLIC int
chunk_manager_trim (chunk_manager_t *cmgr)
{
    /* LATER */
    cmgr->trim_count++;
    return 0;
}

PUBLIC void
chunk_manager_destroy (chunk_manager_t *cmgr)
{
}







