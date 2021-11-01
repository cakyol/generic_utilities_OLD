
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
    int i;

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
     * run thru the newly allocated block and partition each chunk
     * and add it to the head of the free chunks list in the main
     * structure.
     */
    bp = cgp->chunks_block;
    for (i = 0; i < cmgrp->chunks_per_group; i++) {
        chp = (chunk_header_t*) bp;
        chp->my_group = cgp;
        chp->next = cmgrp->free_chunks_list;
        cmgrp->free_chunks_list = chp;
        bp += cmgrp->actual_chunk_size;
    }

    /* update group related stuff */
    cgp->my_manager = cmgrp;
    cgp->n_grp_free = cmgrp->chunks_per_group;

    /* update the main manager related stuff */
    cmgrp->n_cmgr_free += cmgrp->chunks_per_group;
    cmgrp->n_cmgr_total += cmgrp->chunks_per_group;
    cmgrp->n_groups += 1;

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

    /* pop the first chunk from the free chunks list */
    if (cmgrp->n_cmgr_free > 0) {
        chp = cmgrp->free_chunks_list;
        cmgrp->free_chunks_list = chp->next;
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

#if 0

/*
 * All the groups which need freeing up back to the OS are
 * now in this array with the count.
 * They have been removed from the chunk manager groups list
 * and instead placed into this array.
 */
static void
chunk_groups_trim (chunk_manager_t *cmgrp,
    chunk_group_t *grps[], int count )
{
}

/*
 * Returns how many chunk groups were returned back to the os
 * since none of their chunks were used.
 */
static int
thread_unsafe_chunk_manager_trim (chunk_manager_t *cmgrp)
{
    chunk_group_t *cgp, *prev_grp, **free_grps;
    int free_grp_count;

    /*
     * since we free up entire groups of chunks, there must at least
     * be cmgrp->chunks_per_group free chunks before we even bother
     * checking.  If this is not the case, it is impossible for any
     * group to have all its chunks free.
     */
    if (cmgrp->n_cmgr_free < cmgrp->chunks_per_group)
        return 0;

    /*
     * Every group which has all its chunks free is taken off the
     * chunk manager's list and added to a local array here.  The
     * array expands as more groups are added.  At the end, this
     * array will have all the groups whose chunks are all free
     * so that they can be released back to the OS.
     *
     * Remember one little detail :-)  As we take these groups
     * off, their free chunks (all of them), must also be removed
     * from the chunk manager's free_chunks_stack.  They are no
     * longer available or free.  Unfortunately, this is a very
     * heavy operation and every free chunk in the free chunks stack
     * must be checked if it belongs to any group being freed and
     * if so, it must be removed from the stack.
     */
    cgp = cmgrp->groups;
    free_grps = null;
    free_grp_count = 0;
    prev_grp = null;
    while (cgp) {

        /* this group must have all its chunks free */
        if (cgp->n_grp_free >= cmgrp->chunks_per_group) {

            /* add it to the self expanding array */
            free_grp_count++;
            free_grps = realloc(free_grps,
                    (free_grp_count * sizeof(chunk_group_t*)));
            if (free_grps) {
                free_grps[free_grp_count - 1] = cgp;
            } else {
                /* no more array memory, break out of loop */
                break;
            }

            /* take it off the chunk manager's group list */
            if (prev_grp) {
                prev_grp->next = cgp->next;
            } else {
                assert(cmgrp->groups == cgp);
                cmgrp->groups = cgp->next;
            }
        }
        prev_grp = cgp;
        cgp = cgp->next;
    }

    /*
     * all groups to be freed are now unlinked from the chunk manager
     * and are all in the free_grps array.  There are 'free_grp_count'
     * many of them.
     */
    if (free_grps && free_grp_count) {
        chunk_groups_trim(cmgrp, free_grps, free_grp_count);
        free(free_grps);
        return free_grp_count;
    }

    return 0;
}
#endif // 0

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

    /* place it back into the free chunks list */
    chp->next = cmgrp->free_chunks_list;
    cmgrp->free_chunks_list = chp;

    OBJ_WRITE_UNLOCK(cmgrp);
}

PUBLIC int
chunk_manager_trim (chunk_manager_t *cmgrp)
{
    int chunks_returned = 0;

    OBJ_WRITE_LOCK(cmgrp);
    //chunks_returned = thread_unsafe_chunk_manager_trim(cmgrp);//
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
    memset(cmgrp, 0, sizeof(chunk_manager_t));
}

#ifdef __cplusplus
} // extern C
#endif





