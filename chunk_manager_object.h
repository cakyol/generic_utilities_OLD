
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

#ifndef __CHUNK_MANAGER_OBJECT_H__
#define __CHUNK_MANAGER_OBJECT_H__

#include "utils_common.h"
#include "lock_object.h"

typedef struct chunk_manager_s {

    LOCK_VARIABLES;

    /*
     * memory usage counters
     */
    mem_monitor_t mem_mon, *mem_mon_p;

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
    uint64 grow_count, trim_count;

} chunk_manager_t;

extern error_t
chunk_manager_init (chunk_manager_t *cmgr,
	boolean make_it_thread_safe,
        int chunk_size, int initial_number_of_chunks, int grow,
        mem_monitor_t *parent_mem_monitor);

extern void *
chunk_manager_alloc (chunk_manager_t *cmgr);

extern void
chunk_manager_free (chunk_manager_t *cmgr, void *chunk);

extern int
chunk_manager_trim (chunk_manager_t *cmgr);

extern void
chunk_manager_destroy (chunk_manager_t *cmgr);

extern uint64
chunk_manager_memory_usage (chunk_manager_t *cmgr, double *mega_bytes);

#endif // __CHUNK_MANAGER_OBJECT_H__


