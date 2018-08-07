
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

#ifndef __OBJECT_TYPES_H__
#define __OBJECT_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * All objects are uniquely identified by a tuple of object type
 * and an object instance number. Zeros are reserved for the
 * root of the system and should never be used for any user defined
 * objects.  User objects should always have type & instance values 
 * of greater than 0.
 *
 * Note that these values also mean 'all' types and instances for 
 * certain contexts.
 */
#define ROOT_OBJECT_TYPE                0
#define ALL_OBJECT_TYPES                ROOT_OBJECT_TYPE
#define ROOT_OBJECT_INSTANCE            0
#define ALL_OBJECT_INSTANCES            ROOT_OBJECT_INSTANCE

#ifdef __cplusplus
} // extern C
#endif 

#endif // __OBJECT_TYPES_H__


