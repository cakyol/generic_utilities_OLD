
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

#ifndef __ANOTHER_MALLOC_H__
#define __ANOTHER_MALLOC_H__

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * During initialization, signifies a buffer pool.  'size'
 * is the number of bytes each block in this pool is and
 * 'count' signifies how many of such blocks are in this pool.
 */
typedef struct size_count_tuple_s {
    int size;
    int count;
} size_count_tuple_t;

/*
 * The main user structure
 */
typedef struct memory_s {

    /* how many pools are in this memory object */
    int num_pools;

    /*
     * An array of pools, size defined at initialization time
     * This is deliberately defined as a void pointer so that
     * the internal pool structure does not have to be exposed
     * in this h file
     * */
    void *pools;

} memory_t;

extern int
memory_initialize (memory_t *memp,
        bool make_it_thread_safe,
        size_count_tuple_t tuples [], int count);

extern void *
memory_allocate (memory_t *memp, int size);

extern void
memory_free (void *ptr);

extern void
memory_destroy (memory_t *memp);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* __ANOTHER_MALLOC_H__ */


