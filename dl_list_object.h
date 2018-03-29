
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
** Doubly linked list
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __DL_LIST_OBJECT_H__
#define __DL_LIST_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "function_types.h"
#include "mem_monitor_object.h"
#include "lock_object.h"

typedef struct dl_list_element_s dl_list_element_t;

struct dl_list_element_s {

    dl_list_element_t *next, *prev;
    void *object;
};

typedef struct dl_list_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    dl_list_element_t *head, *tail;
    object_comparer objects_match_function;
    int n;

} dl_list_t;

extern int
dl_list_init (dl_list_t *list, object_comparer
        objects_match_function);

extern int 
dl_list_prepend_object (dl_list_t *list, void *object);

extern int
dl_list_append_object (dl_list_t *list, void *object);

extern int
dl_list_find_object (dl_list_t *list, void *object, dl_list_element_t **elem);

/*
 * return the n'th user object from the list (1st entry is n=0) */
extern void*
dl_list_get_object (dl_list_t *list, int n);

extern void
dl_list_iterate (dl_list_t *list, object_comparer fnp,
    void *extra_arg, int stop_if_fails);

#define DL_LIST_FOR_ALL_ELEMENTS(list, objp) \
        for (dl_list_element_t *__n__ = (list)->head; \
             (__n__) && (objp = (__typeof__(objp))(__n__->object)); \
             __n__ = __n__->next)

/*
 * Used for self depleting lists, ie at every iteration, the
 * head element is EXPECTED to be removed by the iteration body.
 * If that is not done, infinite loops WILL result.
 *
 * MUST PERFORM YOUR OWN EXTERNAL LOCKING IF NEEDED.
 */
#define DL_LIST_REMOVE_FROM_HEAD(list, objp) \
        for (volatile dl_list_element_t *__n__ = (list)->head; \
             (__n__) && (objp = (__typeof__(objp))(__n__->object)); \
             __n__ = list->head)

extern int
dl_list_delete_object (dl_list_t *list, void *object);

extern void
dl_list_destroy (dl_list_t *list);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __DL_LIST_OBJECT_H__

