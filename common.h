
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
** For proper indentation viewing, no tabs are used.  This way, every
** text editor should display the code properly.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

/*
 * This file is a collection of the most common types/definitions which
 * is used by almost all the generic utilities.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This supresses the compiler error messages for unused variables,
 * if the error detections are fully turned on during compilations.
 */
#ifndef SUPPRESS_COMPILER_UNUSED_VARIABLE_WARNING
#define SUPPRESS_COMPILER_UNUSED_VARIABLE_WARNING(x)	((void)(x))
#endif

typedef unsigned char byte;

typedef int
(*one_parameter_function_pointer)(void *arg1);

typedef int
(*two_parameter_function_pointer)(void *arg1, void *arg2);

typedef int
(*three_parameter_function_pointer)
    (void *arg1, void *arg2, void *arg3);

typedef int
(*four_parameter_function_pointer)
    (void *arg1, void *arg2, void *arg3, void *arg4);

typedef int
(*five_parameter_function_pointer)
    (void *arg1, void *arg2, void *arg3, void *arg4,
        void *arg5);

typedef int
(*six_parameter_function_pointer)
    (void *arg1, void *arg2, void *arg3, void *arg4,
        void *arg5, void *arg6);

typedef int (*seven_parameter_function_pointer)
    (void *arg1, void *arg2, void *arg3, void *arg4,
        void *arg5, void *arg6, void *arg7);

typedef one_parameter_function_pointer simple_function_pointer;

typedef two_parameter_function_pointer object_comparer;

typedef seven_parameter_function_pointer traverse_function_pointer;

/*
 * This is a function prototype which will be called when an object
 * is being destroyed.  It can be any object being destroyed.
 * It can be an avl tree, index object, list object etc.  The idea
 * is, as the object itself is being destroyed (its nodes being freed
 * back to storage), this is called with the actual 'user_data' stored on 
 * that node, in case user wants to also destroy/free their own
 * data itself.  It will be called one node at a time as the destruction
 * happens.  When this is called, the user MUST be aware that their data
 * has already been REMOVED from whatever object it was a part of, and
 * pretty much 'dangling' by itself.  Typically user will want to 
 * free up his/her object but this may not always be the case.
 * The FIRST parameter passed into this function is the user data itself
 * and the second parameter is whatever the user supplied at the time of
 * the destruction call.  It goes without saying that this function
 * should NOT make any object calls.
 */
typedef void (*destruction_handler_t)(void *user_data, void *extra_arg);

/*
 * Sets a pointer to an integer value.  This can be directly done when
 * a pointer is the same size as an integer but is usually complained 
 * about by most compilers otherwise.  This little trick solves that.
 */
static inline void*
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

#define pointer_from_integer(i)         integer2pointer((long long int) (i))
#define integer_from_pointer(p)         pointer2integer((void*) (p))

#define safe_pointer_set(ptr, value)    if (ptr) *(ptr) = (value)

#ifdef __cplusplus
} /* extern C */
#endif 

#endif /* __COMMON_H__ */



