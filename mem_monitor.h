
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Memory book keeping object
**       Use these instead of malloc/free and it keeps a very accurate
**	 count of how much memory has been used, how many times
**	 allocation has been called and how many times free has been 
**	 called.
**
**       It also zeroes out the allocated memory too.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __MEM_MONITOR_H__
#define __MEM_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>

typedef struct mem_monitor_s {

    unsigned long long bytes_used;
    unsigned long long allocations;
    unsigned long long frees;

} mem_monitor_t;

extern void
mem_monitor_init (mem_monitor_t *mem);

extern void *
mem_monitor_allocate (mem_monitor_t *mem, int size);

extern void *
mem_monitor_reallocate (mem_monitor_t *mem, void *ptr, int newsize);

extern void
mem_monitor_free (mem_monitor_t *mem, void *ptr);

#define MEM_MON_VARIABLES \
    mem_monitor_t mem_mon, *mem_mon_p

#define MEM_MONITOR_SETUP(objp) \
    do { \
        mem_monitor_init(&objp->mem_mon); \
        objp->mem_mon_p = \
            parent_mem_monitor ? \
                parent_mem_monitor : &objp->mem_mon; \
    } while (0)

#define MEM_MONITOR_ALLOC(objp, size) \
    mem_monitor_allocate(objp->mem_mon_p, size)

#define MEM_REALLOC(objp, oldp, newsize) \
    mem_monitor_reallocate(objp->mem_mon_p, oldp, newsize)

#define MEM_MONITOR_FREE(objp, ptr) \
    mem_monitor_free(objp->mem_mon_p, ptr)

#define OBJECT_MEMORY_USAGE(objp, size_in_bytes, size_in_megabytes) \
    do { \
        size_in_bytes = ((unsigned long long int) (sizeof(*(objp)) + \
            (objp)->mem_mon_p->bytes_used)); \
        size_in_megabytes = ((double) size_in_bytes / (double) (1024 * 1024)); \
    } while (0)

#ifdef __cplusplus
} // extern C
#endif 

#endif // __MEM_MONITOR_H__

