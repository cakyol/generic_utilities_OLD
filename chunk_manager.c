
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
        chp->next_chunk_header = cmgrp->free_chunks_list;
        cmgrp->free_chunks_list = chp;
        bp += cmgrp->actual_chunk_size;
    }

    /* update group related stuff */
    cgp->my_manager = cmgrp;
    cgp->n_grp_free = cmgrp->chunks_per_group;

    /* update the main manager related stuff */
    cmgrp->n_cmgr_free += cmgrp->chunks_per_group;

    /*
     * Everything in the group is now initialised, now
     * add the new group to the head of groups list
     */
    cgp->next_chunk_group = cmgrp->groups;
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
        cmgrp->free_chunks_list = chp->next_chunk_header;
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
 * This function tries to return unused chunks back to the OS.
 * It does this by returning the groups (chunk_group_t) whose
 * n_grp_free value is equal to chunk manager's chunks_per_group
 * value.  If a group has 'n_grp_free' chunks which is equal
 * to 'chunks_per_group' number, it means that none of its chunks
 * are in use, and they are all in the free list.  When this is
 * so, the whole group can be returned to the OS in one big swoop,
 * by simply freeing the 'chunks_block' pointer and later also
 * freeing the group itself.
 *
 * The function does this with the following algorithm.  It first
 * scans all the groups whose 'n_grp_free' value is the same as
 * the 'cmgrp->chunks_per_group' value.  It removes these groups
 * from the main manager's group list and places them into a new
 * temporary list.  At the end of the scan, this temp list contains
 * all the groups which can be returned back to the OS.
 *
 * There is one more important step here.  We have to make sure
 * that as we free up those groups, since all their chunks are
 * also in the main manager's free list, each one must be removed 
 * from the main manager.
 *
 * At the end of all this, all groups whose chunks are unused and
 * all those chunks themselves will have been removed from the
 * main manager.
 *
 * The return value of the function is the number of chunk_group_t
 * structures which have been returned back to the OS.
 */
static int
thread_unsafe_chunk_manager_trim (chunk_manager_t *cmgrp)
{
    chunk_group_t *cgp, *tmp_free_groups, *next_cgp;
    chunk_header_t *chp, *next_chp;
    int grps_freed;

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
     * chunk manager's list and added to 'tmp_free_groups' here.
     * What we do here is unlink the entire group chain and iterate
     * thru it one by one.  If the condition matches, we add it to
     * the temp free list, otherwise we add it back to the main
     * manager's free group list.  It is much easier to do such a
     * pass thru the list than messing around trying to delete
     * list nodes.
     */
    cgp = cmgrp->groups;
    cmgrp->groups = null;
    tmp_free_groups = null;
    grps_freed = 0;
    while (cgp) {

        next_cgp = cgp->next_chunk_group;

        /* add it to the temporary free groups list */
        if (cgp->n_grp_free >= cmgrp->chunks_per_group) {
            cgp->next_chunk_group = tmp_free_groups;
            tmp_free_groups = cgp;
            grps_freed++;
        /* put it back into the main structure's list */
        } else {
            cgp->next_chunk_group = cmgrp->groups;
            cmgrp->groups = cgp;
        }
        cgp = next_cgp;
    }

    printf("found %d groups to free\n", grps_freed);

    /*
     * Now, all groups whose chunks are all unused are in a linked
     * list headed by the 'tmp_free_groups' pointer and the rest of
     * the groups (whose chunks may have been used) are now chained
     * back to the 'cmgrp->groups'.
     *
     * We now have to go thru and take all the chunks of these
     * free groups out of the main chunk manager's free chunks list.
     *
     * For every free chunk in the main manager's list, check if
     * it belongs to any of the groups.  If it does, remove it from
     * the main manager's list.  To make this simple, we unlink the
     * main manager's 'free_chunks_list' completely and add all the
     * non free chunks back reconstructing the list.  Since we now 
     * do not include any of the ones which are in those groups, at
     * the end, we end up only with all the chunks which do not belong
     * to any of the groups about to be returned to the OS.
     */
    printf("reconstructing free_chunks_list\n");
    chp = cmgrp->free_chunks_list;
    cmgrp->n_cmgr_free = 0;
    cmgrp->free_chunks_list = null;
    while (chp) {
        next_chp = chp->next_chunk_header;
        cgp = tmp_free_groups;
        while (cgp) {

            /* this chp IS in a group to be returned, so skip it */
            if (chp->my_group == cgp) {
                goto try_next_chp;
            }
            cgp = cgp->next_chunk_group;
        }

        /*
         * if we are here, this chp is not in any group to be freed,
         * so we can safely insert it back into the main manager's
         * list of free chunks.
         */
        chp->next_chunk_header = cmgrp->free_chunks_list;
        cmgrp->free_chunks_list = chp;
        (cmgrp->n_cmgr_free)++;
try_next_chp:
        chp = next_chp;
    }
    printf("reconstructing finished\n");

    /*
     * we have now taken all the chunks out of the main manager's free
     * list which belonged to any of the groups to be returned.
     * We no longer need those groups now, so return them back to
     * the OS.
     */
    cgp = tmp_free_groups;
    while (cgp) {
        next_cgp = cgp->next_chunk_group;
        MEM_MONITOR_FREE(cgp->chunks_block);
        MEM_MONITOR_FREE(cgp);
        cgp = next_cgp;
    }

    return grps_freed;
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

    /* place it back into the head of free chunks list */
    chp->next_chunk_header = cmgrp->free_chunks_list;
    cmgrp->free_chunks_list = chp;

    OBJ_WRITE_UNLOCK(cmgrp);
}

PUBLIC int
chunk_manager_trim (chunk_manager_t *cmgrp)
{
    int grps_freed;

    OBJ_WRITE_LOCK(cmgrp);
    grps_freed = thread_unsafe_chunk_manager_trim(cmgrp);
    OBJ_WRITE_UNLOCK(cmgrp);

    return grps_freed;
}

PUBLIC void
chunk_manager_destroy (chunk_manager_t *cmgrp)
{
    chunk_group_t *grp, *next_grp;

    OBJ_WRITE_LOCK(cmgrp);
    grp = cmgrp->groups;
    while (grp) {
        next_grp = grp->next_chunk_group;
        MEM_MONITOR_FREE(grp->chunks_block);
        MEM_MONITOR_FREE(grp);
        grp = next_grp;
    }
    memset(cmgrp, 0, sizeof(chunk_manager_t));
}

#ifdef __cplusplus
} // extern C
#endif





