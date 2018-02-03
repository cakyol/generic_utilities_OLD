
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

#include "bitlist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

static inline unsigned char
quick_bit_get (unsigned char *bytes, int bit_number)
{ return bytes[bit_number >> 3] & (1 << (bit_number % 8)); }

static inline void
quick_bit_set (unsigned char *bytes, int bit_number)
{ bytes[bit_number >> 3] |= (1 << (bit_number % 8)); }

static inline void
quick_bit_clear (unsigned char *bytes, int bit_number)
{ bytes[bit_number >> 3] &= (~(1 << (bit_number % 8))); } 

static inline int
_first_clear_bit (unsigned int value)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (0 == (value & (1 << i))) return i;
    }
    return -1;
}

static int
thread_unsafe_bitlist_get (bitlist_t *bl, int bit_number, int *returned_bit)
{
    unsigned char value;

    /* check bounds */
    if (bit_number < bl->lowest_valid_bit) return ENODATA;
    if (bit_number > bl->highest_valid_bit) return ENODATA;

    /* adjust offset */
    bit_number -= bl->lowest_valid_bit;

    /* get & return it */
    value = quick_bit_get(bl->the_bits, bit_number);
    *returned_bit = value ? 1 : 0;

    /* no error */
    return 0;
}

static int
thread_unsafe_bitlist_set (bitlist_t *bl, int bit_number)
{
    unsigned char bit;

    /* check bounds */
    if (bit_number < bl->lowest_valid_bit) return ENODATA;
    if (bit_number > bl->highest_valid_bit) return ENODATA;

    /* adjust offset */
    bit_number -= bl->lowest_valid_bit;

    /* set it */
    bit = quick_bit_get(bl->the_bits, bit_number);
    if (bit == 0) {
	quick_bit_set(bl->the_bits, bit_number);
	bl->bits_set_count++;
    }

    /* no error */
    return 0;
}

static int
thread_unsafe_bitlist_clear (bitlist_t *bl, int bit_number)
{
    unsigned char value;

    /* check bounds */
    if (bit_number < bl->lowest_valid_bit) return ENODATA;
    if (bit_number > bl->highest_valid_bit) return ENODATA;

    /* adjust offset */
    bit_number -= bl->lowest_valid_bit;

    /* clear it */
    value = quick_bit_get(bl->the_bits, bit_number);
    if (value) {
	quick_bit_clear(bl->the_bits, bit_number);
	bl->bits_set_count--;
    }

    /* no error */
    return 0;
}

static int
thread_unsafe_bitlist_first_set_bit (bitlist_t *bl, int *returned_bit_number)
{
    int i, size_in_ints, first;
    int *the_ints;

    size_in_ints = bl->size_in_bytes / sizeof(int) + 1;
    the_ints = (int*) bl->the_bits;
    for (i = 0; i < size_in_ints; i++) {
        if (the_ints[i]) {
            first = ffs(the_ints[i]);
            first = (first - 1) + (i * 32) + bl->lowest_valid_bit;
            if (first > bl->highest_valid_bit) return ENODATA;
            *returned_bit_number = first;
            return 0;
        }
    }
    return ENODATA;
}

static int
thread_unsafe_bitlist_first_clear_bit (bitlist_t *bl, int *returned_bit_number)
{
    int i, size_in_ints, first;
    unsigned int *the_ints;

    size_in_ints = bl->size_in_bytes / sizeof(int) + 1;
    the_ints = (unsigned int*) bl->the_bits;
    for (i = 0; i < size_in_ints; i++) {
        if (the_ints[i] != 0xFFFFFFFF) {
            first = _first_clear_bit(the_ints[i]);
            first += (i * 32) + bl->lowest_valid_bit;
            if (first > bl->highest_valid_bit) return ENODATA;
            *returned_bit_number = first;
            return 0;
        }
    }
    return ENODATA;
}


/******* Public functions ****************************************************/

PUBLIC int
bitlist_init (bitlist_t *bl,
    int make_it_thread_safe,
    int lowest_valid_bit, int highest_valid_bit,
    int initialize_to_all_ones,
    mem_monitor_t *parent_mem_monitor)
{
    /* allow an extra 16 bytes at the end to include 2 extra long long ints */
    int size_in_bytes = ((highest_valid_bit - lowest_valid_bit) / 8) + 16;
    int i, rv = 0;
    unsigned char data;

    MEM_MONITOR_SETUP(bl);
    LOCK_SETUP(bl);

    bl->the_bits = (unsigned char*) MEM_MONITOR_ALLOC(bl, size_in_bytes);
    if (0 == bl->the_bits) {
        rv = ENOMEM;
        goto done;
    }
    bl->size_in_bytes = size_in_bytes;
    bl->lowest_valid_bit = lowest_valid_bit;
    bl->highest_valid_bit = highest_valid_bit;
    if (initialize_to_all_ones) {
	data = 0xFF;
	bl->bits_set_count = highest_valid_bit - lowest_valid_bit + 1;
    } else {
	data = 0;
	bl->bits_set_count = 0;
    }
    for (i = 0; i < size_in_bytes; i++) bl->the_bits[i] = data;
done:
    WRITE_UNLOCK(bl);
    return rv;
}

PUBLIC void
bitlist_destroy (bitlist_t *bl)
{
    if (bl->the_bits) free(bl->the_bits);
    bl->the_bits = NULL;
}

PUBLIC int
bitlist_get (bitlist_t *bl, int bit_number, int *returned_bit)
{
    int rv;

    READ_LOCK(bl);
    rv = thread_unsafe_bitlist_get(bl, bit_number, returned_bit);
    READ_UNLOCK(bl);
    return rv;
}

PUBLIC int
bitlist_set (bitlist_t *bl, int bit_number)
{
    int rv;

    WRITE_LOCK(bl);
    rv = thread_unsafe_bitlist_set(bl, bit_number);
    WRITE_UNLOCK(bl);
    return rv;
}

PUBLIC int
bitlist_clear (bitlist_t *bl, int bit_number)
{
    int rv;

    WRITE_LOCK(bl);
    rv = thread_unsafe_bitlist_clear(bl, bit_number);
    WRITE_UNLOCK(bl);
    return rv;
}

PUBLIC int
bitlist_first_set_bit (bitlist_t *bl, int *returned_bit_number)
{
    int rv;

    READ_LOCK(bl);
    rv = thread_unsafe_bitlist_first_set_bit(bl, returned_bit_number);
    READ_UNLOCK(bl);
    return rv;
}

PUBLIC int
bitlist_first_clear_bit (bitlist_t *bl, int *returned_bit_number)
{
    int rv;

    READ_LOCK(bl);
    rv = thread_unsafe_bitlist_first_clear_bit(bl, returned_bit_number);
    READ_UNLOCK(bl);
    return rv;
}

#ifdef __cplusplus
} // extern C
#endif





