
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

#include "chunk_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

static int
chunk_manager_add_group_failed (chunk_manager_t *cmgrp)
{
    byte *bp;
    chunk_header_t *chp;
    chunk_group_t *cgp;
    int new_stack_size, i, stack_idx;
    chunk_header_t **new_stack;

    /* allocate chunk group structure itself */
    cgp = MEM_MONITOR_ALLOC(cmgrp, sizeof(chunk_group_t));
    if (NULL == cgp) return ENOMEM;

    /* allocate the big chunk block */
    cgp->chunks_block = MEM_MONITOR_ALLOC(cmgrp,
            (cmgrp->actual_chunk_size * cmgrp->chunks_per_group));
    if (NULL == cgp->chunks_block) {
        MEM_MONITOR_FREE(cgp);
        return ENOMEM;
    }

    /*
     * now expand the free chunks stack on the main chunk manager so
     * that we can add the new chunks to it from this new group.
     * Note that what we are adding to the stack are pointers to
     * chunk headers.
     */
    new_stack_size = (cmgrp->n_cmgr_total + cmgrp->chunks_per_group) *
        sizeof(chunk_header_t*);
    new_stack = MEM_MONITOR_REALLOC(cmgrp,
            cmgrp->free_chunks_stack, new_stack_size);
    if (NULL == new_stack) {
        MEM_MONITOR_FREE(cgp->chunks_block);
        MEM_MONITOR_FREE(cgp);
        return ENOMEM;
    }
    cmgrp->free_chunks_stack = new_stack;

    /* All the needed memory is allocated, now update everything else */

    /* update group related stuff */
    cgp->my_manager = cmgrp;
    cgp->n_grp_free = cmgrp->chunks_per_group;

    /* update the top manager related stuff */
    cmgrp->n_cmgr_free += cmgrp->chunks_per_group;
    cmgrp->n_cmgr_total += cmgrp->chunks_per_group;
    cmgrp->n_groups += 1;

    /*
     * add all the new chunk addresses to the stack of
     * free chunks on the top manager structure.
     */
    bp = cgp->chunks_block;
    stack_idx = cmgrp->free_chunks_stack_index;
    for (i = 0; i < cmgrp->chunks_per_group; i++) {
        chp = (chunk_header_t*) bp;
        chp->my_group = cgp;
        cmgrp->free_chunks_stack[stack_idx++] = chp;
        bp += cmgrp->actual_chunk_size;
    }

    /*
     * Everything in the group is now initialised, now
     * add the new group to the head of groups list
     */
    cgp->next = cmgrp->groups;
    cmgrp->groups = cgp;

    return 0;
}

/*
 * Grab a chunk from the head of the free chunks list,
 * and return it to the caller, adjusting counters & head.
 */
static inline void *
thread_unsafe_chunk_manager_alloc (chunk_manager_t *cmgrp)
{
    chunk_header_t *chp;

    /* If there are any free chunks available on the stack, pop one */
    if (cmgrp->n_cmgr_free > 0) {
        assert(cmgrp->free_chunks_stack_index <= cmgrp->n_cmgr_total);
        chp = cmgrp->free_chunks_stack[cmgrp->free_chunks_stack_index];
        (cmgrp->free_chunks_stack_index)++;
        (chp->my_group->n_grp_free)--;
        (cmgrp->n_cmgr_free)--;
        return &(chp->data[0]);
    }

    /*
     * if we are here, no more free chunks left, so create a new
     * group and recursively call the function again.
     */
    if (chunk_manager_add_group_failed(cmgrp)) {
        return null;
    }

    /* new group created, recursive call will return successfully */
    return
        thread_unsafe_chunk_manager_alloc(cmgrp);
}

/*
 * Returns how many chunks were returned back to the os
 * This must be a multiple of chunks_per_group since when
 * chunks are returned back to the OS, entire groups are
 * freed up.
 */
static int
thread_unsafe_chunk_manager_trim (chunk_manager_t *cmgrp)
{
    return 0;
}

/***************************** 80 column separator ****************************/

PUBLIC int
chunk_manager_init (chunk_manager_t *cmgrp,
    boolean make_it_thread_safe,
    int chunk_size, int chunks_per_group,
    mem_monitor_t *parent_mem_monitor)
{
    /* basic sanity checks */
    if ((chunk_size < MIN_CHUNK_SIZE) ||
        (chunk_size > MAX_CHUNK_SIZE)) {
            return EINVAL;
    }
    if ((chunks_per_group < MIN_CHUNKS_PER_GROUP) ||
        (chunks_per_group > MAX_CHUNKS_PER_GROUP)) {
            return EINVAL;
    }

    /* clear absolutely everything */
    memset(cmgrp, 0, sizeof(chunk_manager_t));

    MEM_MONITOR_SETUP(cmgrp);
    LOCK_SETUP(cmgrp);

    /* align the size to the next 8 bytes */
    cmgrp->chunk_size = chunk_size;
    cmgrp->actual_chunk_size =
        ((chunk_size + 7) & ~7) + sizeof(chunk_header_t);

    cmgrp->chunks_per_group = chunks_per_group;

    OBJ_WRITE_UNLOCK(cmgrp);

    return 0;
}

PUBLIC void *
chunk_manager_alloc (chunk_manager_t *cmgrp)
{
    void *ptr;

    OBJ_WRITE_LOCK(cmgrp);
    ptr = thread_unsafe_chunk_manager_alloc(cmgrp);
    OBJ_WRITE_UNLOCK(cmgrp);

    return ptr;
}

PUBLIC void
chunk_manager_free (void *chunk)
{
    chunk_header_t *chp;
    chunk_manager_t *cmgrp;

    /* get the hidden chunk header and the chunk manager pointer */
    chp = (chunk_header_t*) (((byte*) chunk) - sizeof(chunk_header_t));
    cmgrp = chp->my_group->my_manager;

    OBJ_WRITE_LOCK(cmgrp);

    /* group is being returned a chunk */
    (chp->my_group->n_grp_free)++;

    /* chunk manager is being returned one */
    (cmgrp->n_cmgr_free)++;

    /* place it into the free chunks stack */
    (cmgrp->free_chunks_stack_index)--;
    cmgrp->free_chunks_stack[cmgrp->free_chunks_stack_index] = chp;

    OBJ_WRITE_UNLOCK(cmgrp);
}

PUBLIC int
chunk_manager_trim (chunk_manager_t *cmgrp)
{
    int chunks_returned;

    OBJ_WRITE_LOCK(cmgrp);
    chunks_returned = thread_unsafe_chunk_manager_trim(cmgrp);
    OBJ_WRITE_UNLOCK(cmgrp);

    return chunks_returned;
}

PUBLIC void
chunk_manager_destroy (chunk_manager_t *cmgrp)
{
    chunk_group_t *grp, *next;

    OBJ_WRITE_LOCK(cmgrp);
    grp = cmgrp->groups;
    while (grp) {
        next = grp->next;
        MEM_MONITOR_FREE(grp->chunks_block);
        MEM_MONITOR_FREE(grp);
        grp = next;
    }
    MEM_MONITOR_FREE(cmgrp->free_chunks_stack);
    memset(cmgrp, 0, sizeof(chunk_manager_t));
}

#ifdef __cplusplus
} // extern C
#endif





