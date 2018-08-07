
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

#ifndef __FUNCTION_TYPES_H__
#define __FUNCTION_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*one_parameter_function_pointer)(void *arg1);

typedef int (*two_parameter_function_pointer)(void *arg1, void *arg2);

typedef int (*three_parameter_function_pointer)(void *arg1, void *arg2, void *arg3);

typedef int (*four_parameter_function_pointer)
                (void *arg1, void *arg2, void *arg3, void *arg4);

typedef int (*five_parameter_function_pointer)
                (void *arg1, void *arg2, void *arg3, void *arg4, void *arg5);

typedef int (*six_parameter_function_pointer)
                (void *arg1, void *arg2, void *arg3,
                 void *arg4, void *arg5, void *arg6);

typedef int (*seven_parameter_function_pointer)
                (void *arg1, void *arg2, void *arg3,
                 void *arg4, void *arg5, void *arg6,
                 void *arg7);

typedef one_parameter_function_pointer simple_function_pointer;

typedef two_parameter_function_pointer object_comparer;

typedef seven_parameter_function_pointer traverse_function_pointer;

#if 0

/*
 * A generic function pointer used in traversing trees or tries etc.
 * If the return value is 0, iteration will continue.  Otherwise,
 * iteration will stop.  
 *
 * The first parameter is always the object pointer in the context
 * this function is called.  For example, if it is being called in
 * the context of an avl tree, it will be the avl tree object pointer.
 * If it is being called in the context of an index object, it
 * will be the index object pointer.
 *
 * The second parameter is the object node in question.  Just like
 * the first param above, it can be an avl tree node or an index node.
 *
 * The third parameter will always be the actual user data stored
 * in that utility node passed in.
 *
 * The rest of the parameters are user passed and fed into the function.
 *
 * Not all the params will necessarily be used but this is a very generic
 * definition which covers all possible needs.
 *
 */
typedef int (*traverse_function_pointer)
    (void *utility_object, void *utility_node, void *node_data, 
     void *extra_parameter_0,
     void *extra_parameter_1,
     void *extra_parameter_2,
     void *extra_parameter_3);

#endif // 0

#ifdef __cplusplus
} // extern C
#endif 

#endif // __FUNCTION_TYPES_H__


