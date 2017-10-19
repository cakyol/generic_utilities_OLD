
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

#ifndef __EVENT_TYPES_H__
#define __EVENT_TYPES_H__

/*
 * These are *ALL* the possible events which can happen
 * These definitions are typically used with the event
 * manager & object manager code.
 */

#define ATTRIBUTE_VALUE_ADDED			(1 << 0)
#define ATTRIBUTE_VALUE_DELETED			(1 << 1)

#define ATTRIBUTE_INSTANCE_ADDED		(1 << 5)
#define ATTRIBUTE_INSTANCE_DELETED		(1 << 6)

#define ATTRIBUTE_EVENTS                        (ATTRIBUTE_VALUE_ADDED | \
                                                 ATTRIBUTE_VALUE_DELETED | \
                                                 ATTRIBUTE_INSTANCE_ADDED | \
                                                 ATTRIBUTE_INSTANCE_DELETED)

#define OBJECT_CREATED				(1 << 10)
#define OBJECT_DESTROYED			(1 << 11)

#define OBJECT_EVENTS                           (OBJECT_CREATED | OBJECT_DESTROYED)

static inline int
is_an_object_event (int event)
{
    return
        (OBJECT_CREATED == event) || 
        (OBJECT_DESTROYED == event);
}

static inline int
is_an_attribute_event (int event)
{
    return
        (ATTRIBUTE_VALUE_ADDED == event) ||
        (ATTRIBUTE_VALUE_DELETED == event) ||
        (ATTRIBUTE_INSTANCE_ADDED == event) ||
        (ATTRIBUTE_INSTANCE_DELETED == event);
}

#endif // __EVENT_TYPES_H__




