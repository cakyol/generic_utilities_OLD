
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

/*
 * adds a chunk (chp) to the end of the list.
 */
static inline void
cmgr_append_chunk_to_list (chunk_list_t *list, chunk_header_t *chp)
{
    chp->next = 0;
    if (list->n <= 0) {
        chp->prev = 0;
        list->head = list->tail = chp;
    } else {
        chp->prev = list->tail;
        list->tail->next = chp;
        list->tail = chp;
    }
    list->n++;
}

/*
 * removes the chunk (chp) out of the list.
 */
static inline void
cmgr_remove_chunk_from_list (chunk_list_t *list, chunk_header_t *chp)
{
    if (chp->next == 0) {
        if (chp->prev == 0) {
            list->head = list->tail = 0;
        } else {
            chp->prev->next = 0;
            list->tail = chp->prev;
        }
    } else {
        if (chp->prev == 0) {
            list->head = chp->next;
            chp->next->prev = 0;
        } else {
            chp->prev->next = chp->next;
            chp->next->prev = chp->prev;
        }
    }
    list->n--;
}

/*
 * creates a new fresh chunk & adds it to the end of the free list
 */
static inline int
cmgr_add_new_chunk (chunk_manager_t *cmgr)
{
    chunk_header_t *chp;

    chp = MEM_MONITOR_ALLOC(cmgr, (cmgr->chunk_size + sizeof(chunk_header_t)));
    if (0 == chp) return ENOMEM;
    cmgr_append_chunk_to_list(&cmgr->free_chunks, chp);
    return 0;
}

static void
cmgr_destroy_list (chunk_manager_t *cmgr, chunk_list_t *list)
{
    chunk_header_t *chp, *nxt;

    chp = list->head;
    while (chp) {
        nxt = chp->next;
        cmgr_remove_chunk_from_list(list, chp);
        MEM_MONITOR_FREE(cmgr, chp);
        chp = nxt;
    }
    assert(list->head == 0);
    assert(list->tail == 0);
    assert(list->n == 0);
}

/********************* public functions *********************/

/*
 * Initializes the chunk manager to specified parameters.
 * Returns 0 on success or non zero upon error.
 * How many actual chunks have been initialised is returned
 * in 'actual_chunks_created'.
 */
int
chunk_manager_init (chunk_manager_t *cmgr,
        int make_it_thread_safe,
        int chunk_size, int expansion,
        int initial_size, int *actual_chunks_created,
        mem_monitor_t *parent_mem_monitor)
{
    int failed = 0;

    MEM_MONITOR_SETUP(cmgr);
    LOCK_SETUP(cmgr);
    safe_pointer_set(actual_chunks_created, 0);
    cmgr->chunk_size = chunk_size;
    cmgr->expansion = expansion;
    cmgr->should_not_be_modified = 0;
    cmgr->free_chunks.head = cmgr->free_chunks.tail = 0;
    cmgr->used_chunks.head = cmgr->used_chunks.tail = 0;
    while (initial_size-- > 0) {
        if ((failed = cmgr_add_new_chunk(cmgr))) break;
    }
    safe_pointer_set(actual_chunks_created, cmgr->free_chunks.n);
    WRITE_UNLOCK(cmgr);
    return failed;
}

void *
chunk_manager_alloc (chunk_manager_t *cmgr) 
{
    chunk_header_t *chp;
    int i, added;

    if (cmgr->should_not_be_modified)
        return 0;

    WRITE_LOCK(cmgr);

try_again:

    if (cmgr->free_chunks.n > 0) {
        chp = cmgr->free_chunks.head;

        /* move the chunk out of free list, insert into used list */
        cmgr_remove_chunk_from_list(&cmgr->free_chunks, chp);    
        cmgr_append_chunk_to_list(&cmgr->used_chunks, chp);

        /* done */
        WRITE_UNLOCK(cmgr);
        return &chp->data[0];
    }

    /* no more free chunks, try and add some more to the pool */
    added = 0;
    for (i = 0; i < cmgr->expansion; i++) {
        if (cmgr_add_new_chunk(cmgr)) break;
        added++;
    }
    if (added) goto try_again;

    /* could not expand */
    WRITE_UNLOCK(cmgr);
    return 0;
}

int
chunk_manager_free (chunk_manager_t *cmgr, void *data)
{
    chunk_header_t *chp;

    if (cmgr->should_not_be_modified)
        return EBUSY;

    WRITE_LOCK(cmgr);
    chp = (chunk_header_t*)(((char*) data) - sizeof(chunk_header_t));

    /* move it out of used list into free list */
    cmgr_remove_chunk_from_list(&cmgr->used_chunks, chp);
    cmgr_append_chunk_to_list(&cmgr->free_chunks, chp);

    /* done */
    WRITE_UNLOCK(cmgr);

    return 0;
}

void
chunk_manager_iterate_stop (chunk_manager_t *cmgr)
{
    cmgr->should_not_be_modified = 0;
    WRITE_UNLOCK(cmgr);
}

void *
chunk_manager_iterate_start (chunk_manager_t *cmgr)
{
    WRITE_LOCK(cmgr);
    cmgr->should_not_be_modified = 1;
    cmgr->iterator = cmgr->used_chunks.head;
    if (cmgr->iterator) {
        return &cmgr->iterator->data[0];
    }

    /* end of list */
    chunk_manager_iterate_stop(cmgr);
    return 0;
}

void *
chunk_manager_iterate_next (chunk_manager_t *cmgr)
{
    if (cmgr->iterator) {
        cmgr->iterator = cmgr->iterator->next;
    }
    if (cmgr->iterator) {
        return &cmgr->iterator->data[0];
    }

    /* end of list */
    chunk_manager_iterate_stop(cmgr);
    return 0;
}

/*
 * Try & return all unused chunks back to the system.
 */
void
chunk_manager_trim (chunk_manager_t *cmgr)
{
    WRITE_LOCK(cmgr);
    cmgr_destroy_list(cmgr, &cmgr->free_chunks);
    assert(cmgr->free_chunks.n == 0);
    WRITE_UNLOCK(cmgr);
}

void
chunk_manager_destroy (chunk_manager_t *cmgr)
{
    WRITE_LOCK(cmgr);
    cmgr_destroy_list(cmgr, &cmgr->free_chunks);
    cmgr_destroy_list(cmgr, &cmgr->used_chunks);
    WRITE_UNLOCK(cmgr);
    LOCK_OBJ_DESTROY(cmgr);
    memset(cmgr, 0, sizeof(chunk_manager_t));
}

#ifdef __cplusplus
} // extern C
#endif







