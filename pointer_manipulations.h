
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol, gee.akyol@gmail.com
** Copyright: Cihangir Metin Akyol, March 2016, November 2017
**
** This code is developed by and belongs to Cihangir Metin Akyol.  
** It is NOT owned by any company or consortium.  It is the sole
** property and work of one individual.
**
** It can be used by ANYONE or ANY company for ANY purpose as long 
** as ownership and/or patent claims are NOT made to it by ANYONE
** or any ANY ENTITY.
**
** It ALWAYS is and WILL remain the property of Cihangir Metin Akyol.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __POINTER_MANIPULATIONS_H__
#define __POINTER_MANIPULATIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char byte;

/*
 * Sets a pointer to an integer value.  This can be directly done when
 * a pointer is the same size as an integer but is usually complained 
 * about by most compilers otherwise.  This little trick solves that.
 */
static inline void *
integer2pointer (long long int value)
{
    byte *ptr = 0;
    ptr += value;
    return ptr;
}

static inline long long int
pointer2integer (void *ptr)
{
    byte *zero = 0;
    return ((byte*) ptr) - zero;
}

#define safe_pointer_set(ptr, value) \
    if (ptr) *(ptr) = (value)

#ifdef __cplusplus
} // extern C
#endif 

#endif // __POINTER_MANIPULATIONS_H__




