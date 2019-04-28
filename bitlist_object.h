
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
** Bit list/map object, first bit is always 0.
** All answers are adjusted to the offset
** Return values are 0 for success or an errorcode, except
** for the functions which return a position as value.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __BITLIST_OBJECT_H__
#define __BITLIST_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdlib.h>
#include <strings.h>

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"

typedef struct bitlist_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;
    int lowest_valid_bit;
    int highest_valid_bit;
    int size_in_ints;
    int bits_set_count;
    unsigned int *the_bits;

} bitlist_t;

static inline int
bitlist_count_ones (bitlist_t *bl)
{ return bl->bits_set_count; }

static inline int
bitlist_count_zeros (bitlist_t *bl)
{
    return 
        bl->highest_valid_bit - bl->lowest_valid_bit + 1 - 
        bl->bits_set_count; 
}

extern int
bitlist_init (bitlist_t *bl,
    int make_it_thread_safe,
    int lowest_valid_bit, int highest_valid_bit,
    int initialize_to_all_ones,
    mem_monitor_t *parent_mem_monitor);

extern int
bitlist_get (bitlist_t *bl, int bit_number, int *returned_bit);

extern int
bitlist_set (bitlist_t *bl, int bit_number);

extern int
bitlist_clear (bitlist_t *bl, int bit_number);

extern int
bitlist_first_set_bit (bitlist_t *bl, int *returned_bit_number);

extern int
bitlist_first_clear_bit (bitlist_t *bl, int *returned_bit_number);

extern void
bitlist_destroy (bitlist_t *bl);

#ifdef __cplusplus
} // extern C
#endif

#endif // __BITLIST_OBJECT_H__




