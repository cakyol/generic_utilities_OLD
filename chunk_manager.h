
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

typedef struct chunk_header_s chunk_header_t;
typedef struct chunk_list_s chunk_list_t;
typedef struct chunk_manager_s chunk_manager_t;

struct chunk_header_s {

    chunk_header_t *prev, *next;
    long long int data [0];
};

struct chunk_list_s {

    chunk_header_t *head, *tail;
    int n;
};

struct chunk_manager_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;
    int chunk_size;
    int expansion;
    int should_not_be_modified;
    chunk_list_t free_chunks;

    /*
     * The 'used_list' is maintained for a very useful purpose.
     * Whatever data structure the chunks are used for, they can be
     * traversed easily by simply traversing this list one chunk
     * at a time.  Since it is much easier to traverse a linked
     * list, it is very convenient.
     */
    chunk_list_t used_chunks;

    /* used for traversing */
    chunk_header_t *iterator;
};

int
chunk_manager_init (chunk_manager_t *cmgr,
        int make_it_thread_safe,
        int chunk_size, int expansion,
        int initial_size, int *actual_chunks_available,
        mem_monitor_t *parent_mem_monitor);

void *
chunk_manager_alloc (chunk_manager_t *cmgr);

int
chunk_manager_free (chunk_manager_t *cmgr, void *data);

void *
chunk_manager_iterate_start (chunk_manager_t *cmgr);

void *
chunk_manager_iterate_next (chunk_manager_t *cmgr);

void
chunk_manager_iterate_stop (chunk_manager_t *cmgr);

void
chunk_manager_trim (chunk_manager_t *cmgr);

void
chunk_manager_destroy (chunk_manager_t *cmgr);

#ifdef __cplusplus
} // extern C
#endif

#endif /* __CHUNK_MANAGER_H__ */


