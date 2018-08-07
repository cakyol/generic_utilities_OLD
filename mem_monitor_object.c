/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Memory accounting object
**       Keeps a very accurate measurement of how much dynamic
**       memory has been used, how many times the alloc has been
**       invoked and how many times the free has been invoked.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#include "mem_monitor_object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * An extra 8 bytes is allocated at the front to store the length.
 * Make sure this is 8 bytes aligned or it crashes in 64 bit systems.
 */
void *
mem_monitor_allocate (mem_monitor_t *memp, int size)
{
    int total_size = size + sizeof(unsigned long long int);
    unsigned long long int *block = 
        (unsigned long long int*) malloc(total_size);

    if (block) {
        memset(block, 0, total_size);
        *block = total_size;
        memp->bytes_used += total_size;
        memp->allocations++;
        block++;
        return (void*) block;
    }
    return NULL;
}

void
mem_monitor_free (mem_monitor_t *memp, void *ptr)
{
    unsigned long long int *block = (unsigned long long int*) ptr;

    block--;
    memp->bytes_used -= *block;
    memp->frees++;
    free(block);
}

void *
mem_monitor_reallocate (mem_monitor_t *memp, void *ptr, int newsize)
{
    int oldsize;
    unsigned long long int *block = (unsigned long long int*) ptr;
    unsigned long long int *new_block;

    if (NULL == ptr)
        return 
            mem_monitor_allocate(memp, newsize);
    block--;
    oldsize = *block;
    newsize += sizeof(unsigned long long int);
    new_block = (unsigned long long int*) realloc((void*) block, newsize);
    if (new_block) {
        *new_block = newsize;
        memp->bytes_used -= oldsize;
        memp->bytes_used += newsize;
        new_block++;
        memp->allocations++;
        memp->frees++;
        return (void*) new_block;
    }
    return NULL;
}

#ifdef __cplusplus
} // extern C
#endif 


