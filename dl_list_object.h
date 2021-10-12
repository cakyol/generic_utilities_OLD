
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
** Doubly linked list container object
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

#include "common.h"
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
    STATISTICS_VARIABLES;

    dl_list_element_t *head, *tail;
    int should_not_be_modified;
    int n;

} dl_list_t;

/*
 * initialize a doubly linked list
 */
extern int
dl_list_init (dl_list_t *list,
    boolean make_it_thread_safe,
    boolean statistics_wanted,
    mem_monitor_t *parent_mem_monitor);

/*
 * add an object to the beginning of the list
 */
extern int 
dl_list_prepend_object (dl_list_t *list, void *object);

/*
 * add an object to the end of the list
 */
extern int
dl_list_append_object (dl_list_t *list, void *object);

/*
 * Traverse the list one element at a time, starting at the head
 * till the end or until the user supplied function returns a
 * non zero (indicating an error).  What this means is that
 * if the user finds what is being looked for, a non zero should
 * be returned, terminating the traversal.
 *
 * The positional parameters passed to the traversal function are:
 * - The list object
 * - the list element,
 * - the user object stored at that element
 * - the rest of the parameters p[0-3]
 */
extern void
dl_list_traverse (dl_list_t *list,
    traverse_function_pointer tfn,
    void *p0, void *p1, void *p2, void *p3);

extern void
dl_list_destroy (dl_list_t *list);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __DL_LIST_OBJECT_H__

