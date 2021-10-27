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
 * All we need to process memory requests.  It is a cheat sheet
 * inserted in front of the memory returned to user.  It cannot
 * however be directly inserted but instead enclosed in the
 * mem_aligner_t structure to make sure it is 64 bytes long & aligned.
 */
typedef struct mem_cheat_s {

    /* the relevant mem monitor */
    mem_monitor_t *mmp;

    /* total size of bytes used INCLUDING the secret header */
    int total_size;

} mem_cheat_t;

/*
 * This is secretly inserted to the front of the memory
 * returned to the user to store the relevant information.
 *
 * It MUST be 8 byte aligned and hence the funky structure
 * definition below, which guarantees that.
 */
typedef struct mem_aligner_s {

    union {
        byte aligner[8];
        mem_cheat_t mc;
    } u;

} mem_aligner_t;

/*
 * extract 'mmp' and TOTAL size given the user data pointer.
 */
static inline void
get_memory_info (void *ptr,
    mem_monitor_t **mmp, int *total_sizep)
{
    mem_aligner_t* map = (mem_aligner_t*)
        ((byte*) ptr - sizeof(mem_aligner_t));

    *mmp = map->u.mc.mmp;
    *total_sizep = map->u.mc.total_size;
}

/*
 * An extra mem_aligner_t is inserted into the front
 * of all memory returrned to the user so we have that information
 * available for the rest of the operations.
 */
void *
mem_monitor_allocate (mem_monitor_t *mmp,
        int size, bool initialize_to_zero)
{
    int total_size = size + sizeof(mem_aligner_t);
    mem_aligner_t *map;
    byte *block;

    block = malloc(total_size);
    if (block) {
        if (initialize_to_zero) memset(block, 0, total_size);
        map = (mem_aligner_t*) block;
        map->u.mc.mmp = mmp;
        map->u.mc.total_size = total_size;
        if (mmp) {
            mmp->bytes_used += total_size;
            mmp->allocations++;
        }
        return (void*) (block + sizeof(mem_aligner_t));
    }
    return NULL;
}

void
mem_monitor_free (void *ptr)
{
    mem_monitor_t *mmp;
    int total_size;

    get_memory_info(ptr, &mmp, &total_size);
    if (mmp) {
        mmp->bytes_used -= total_size;
        mmp->frees++;
    }
    free((byte*) ptr - sizeof(mem_aligner_t));
}

void *
mem_monitor_reallocate (mem_monitor_t *mmp,
    void *ptr, int new_data_size,
    bool initialize_to_zero)
{
    byte *old_data = (byte*) ptr;
    byte *new_data;
    int old_total_size, old_data_size, copy_size;

    /* for a null pointer, it just becomes a new alloc */
    if (NULL == ptr) {
        return
            mem_monitor_allocate(mmp, new_data_size, initialize_to_zero);
    }

    get_memory_info(ptr, &mmp, &old_total_size);
    old_data_size = old_total_size - sizeof(mem_aligner_t);

    new_data = mem_monitor_allocate(mmp, new_data_size, initialize_to_zero);
    if (NULL == new_data) return NULL;

    /*
     * copy existing data to the new block.  Note that
     * the new data size may be LESS than the old data size
     * meaning the block is shrinking.  Take the min value
     * of new_data_size & old_data_size to copy.
     */
    copy_size = (new_data_size > old_data_size) ?
                    old_data_size : new_data_size;
    memmove(new_data, old_data, copy_size);

    /* free the old block */
    mem_monitor_free(ptr);

    /* done */
    return (void*) new_data;
}

#ifdef __cplusplus
} // extern C
#endif 


