
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

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** EXTREMELY fast memory allocator de-allocator for many objects of the
** same size, called chunks.  All the chunks are pre-allocated and after
** that retrieved & returned to the pool extremely quickly.
**
** On average, it is about 20 times faster from malloc/free.
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

typedef struct chunk_manager_s {

    LOCK_VARIABLES;
    MEM_MON_VARIABLES;

    /*
     * The actual stack of free chunks.  The 'index' is used to pop
     * off chunks from this stack and return to the user.
     */
    void **chunks;

    /*
     * actual block size of memory
     */
    int chunk_size;

    /*
     * max capacity of the stack, it MAY be bigger than how many free
     * chunks we may have in the stack.  That happens when we expand,
     * so the stack expands but we run out of memory to allocate the
     * actual chunks.
     */
    int potential_capacity;

    /*
     * How many actual non NULL chunks exist in the entire manager,
     * used AND free
     */
    int total_chunk_count;

    /*
     * the index of the first available chunk at any time.  Basically
     * the stack pointer into the stack of free chunks.  When this
     * value reaches < 0, it is time to re-allocate more chunks.
     */
    int index;

    /*
     * By how many chunks to grow each time we need to expand.
     * If <= 0, it means the chunk manager is static and is not
     * allowed to grow.
     */
    int expansion_size;

    /*
     * statistical counters indicating how many times the object grew
     * and/or shrank
     */
    long long int grow_count, trim_count;

} chunk_manager_t;

/*
 * If a chunk manager is to be used with any of the objects,
 * then a pointer to this structure is passed, which contains 
 * all the parameters that are needed to initialize & use the
 * chunk manager.  Otherwise, a NULL is passed indicating 
 * that the object is simply using malloc'ed memory instead
 * of the chunk manager.
 */
typedef struct chunk_manager_parameters_s {

    int initial_number_of_chunks;
    int grow_size;

} chunk_manager_parameters_t;

extern int
chunk_manager_init (chunk_manager_t *cmgr,
        int make_it_thread_safe,
        int chunk_size, int initial_number_of_chunks, int grow,
        mem_monitor_t *parent_mem_monitor);

#define CHUNK_MANAGER_VARIABLES \
    chunk_manager_t chunk_structure; \
    chunk_manager_t *chunks

extern void *
chunk_manager_alloc (chunk_manager_t *cmgr);

extern void
chunk_manager_free (chunk_manager_t *cmgr, void *chunk);

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
extern int
chunk_manager_trim (chunk_manager_t *cmgr);

/*
 * Note that if there are any outstanding chunks not
 * returned back to the object, EBUSY will be returned.
 */
extern int
chunk_manager_destroy (chunk_manager_t *cmgr);

#define CHUNK_MANAGER_SETUP(obj, chunk_size, cmpp) \
    if (cmpp) { \
        int __failed__; \
        obj->chunks = &obj->chunk_structure; \
        __failed__ = chunk_manager_init(obj->chunks, 0, \
                    chunk_size, \
                    cmpp->initial_number_of_chunks, \
                    cmpp->grow_size, (obj)->mem_mon_p); \
        if (__failed__) return __failed__; \
    } else { \
        obj->chunks = 0; \
    }

#define CHUNK_ALLOC(obj, size) \
    (obj)->chunks ? \
        chunk_manager_alloc((obj)->chunks) : \
        MEM_MONITOR_ALLOC((obj), size);

#define CHUNK_FREE(obj, ptr) \
    if ((obj)->chunks) \
        chunk_manager_free((obj)->chunks, ptr); \
    else \
        MEM_MONITOR_FREE((obj), ptr);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __CHUNK_MANAGER_H__


