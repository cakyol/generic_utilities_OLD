
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

#ifndef __CHUNK_MANAGER_H__
#define __CHUNK_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"

/******************************************************************************
 *
 * This is a chunk manager which manages a many same sized chunks, 
 * dynamically allocated & freed.  It is like malloc & free but much
 * faster since most chunks are pre-allocated and disbursing them
 * is therefore quite fast.
 *
 * There is one VERY important requirement of this implementation.
 * It is the fact that when this dynamically grows, the addresses of
 * all previously allocated chunks MUST stay the same.  Therefore,
 * a simple realloc scheme could not be used since at some point,
 * a realloc may change the adresses completely, violating this
 * principle.  The reason this is required is that if the user
 * caches the pointer values of these chunks, they must never change.
 *
 * To be able to provide the above, the concept of a 'chunk group' is
 * defined where all the addresses in that group always stay the same.
 * In each group, a big block of malloced area is divided into many chunks
 * and are maintained.  If chunks are all used up and more is needed,
 * we cannot simply realloc the block and expand it.  This is because
 * the realloc may change the addresses and this would not work.  To
 * provide this expansion requirement, an expanding list of chunk groups
 * is therefore maintained.  When all the chunks in a group are exhausted,
 * a new group is created.  When a new chunk group is created, all the
 * available chunk addresses are added to the stack of free chunks in the
 * main chunk manager structure.  This causes the chunk manager to
 * pause and add new chunks to the free chunks stack when the current
 * set of chunks are depleted.
 *
 * A bigger challenge is when 'trimming' the structure is needed.
 * This is when the user decides that there are a lot of free chunks
 * around which are not needed and may have to be returned back to
 * memory.  When this is requested, all the groups are scanned and
 * for the ones who have all their chunks unused, each chunk is
 * removed off the free chunks stack and the entire group deleted.
 * This can be a bit tricky and time consuming and is therefore not
 * automatically performed but left to the user as to when it needs
 * to be run.
 *
 */

typedef struct chunk_header_s chunk_header_t;
typedef struct chunk_group_s chunk_group_t;
typedef struct chunk_manager_s chunk_manager_t;

struct chunk_header_s {

    /*
     * The chunk group I belong to so that when I am freed up,
     * I will return to the same group.
     */
    chunk_group_t *my_group;

    /*
     * this is what is returned back to the user and it must
     * always be 8 bytes aligned, hence the use of long long int.
     */
    long long int data [0];
};

/*
 * Each group has a big memory block which is divided into chunk size
 * sections.  Addresses of each of these chunks are stored in the main
 * chunk manager's free_chunks_stack array, which grows by using alloc
 * & realloc.  When the chunks in a group are all used up, a new group
 * is created and the new chunks are all added to this stack of free
 * chunks.
 */
struct chunk_group_s {

    /* The chunk manager I belong to */
    chunk_manager_t *my_manager;

    /* big bulk of the memory, all chunks adjacent in one big block */
    void *chunks_block;

    /* how many free chunks are in this group */
    int n_grp_free;

    /* next group in list */
    chunk_group_t *next;

};

struct chunk_manager_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    /* size of each chunk the user wants and how much it internally takes*/
    int chunk_size;
    int actual_chunk_size;

    /* how many chunks per group is needed */
    int chunks_per_group;

    /* total number of groups so far in the manager */
    int n_groups;

    /*
     * Total free chunks in all of the groups of the manager.  If this
     * is zero, it is time to create another group & add it to the
     * manager.
     */
    int n_cmgr_free;

    /* a stack of all the free chunks in the entire manager */
    chunk_header_t **free_chunks_stack;
    int free_chunks_stack_index;

    /* a linked list of all the groups */
    chunk_group_t *groups;

};

/*
 * Redefine these as per your own requirements
 */

#define MIN_CHUNK_SIZE          8
#define MAX_CHUNK_SIZE          256
#define MIN_CHUNKS_PER_GROUP    64
#define MAX_CHUNKS_PER_GROUP    (0x3FFF)

extern int
chunk_manager_init (chunk_manager_t *cmgrp,
    boolean make_it_thread_safe,
    int chunk_size, int chunks_per_group,
    mem_monitor_t *parent_mem_monitor);

extern void *
chunk_manager_alloc (chunk_manager_t *cmgrp);

extern void
chunk_manager_free (void *chunk);

extern int
chunk_manager_trim (chunk_manager_t *cmgrp);

extern void
chunk_manager_destroy (chunk_manager_t *cmgrp);

#ifdef __cplusplus
} // extern C
#endif

#endif // __CHUNK_MANAGER_H__


