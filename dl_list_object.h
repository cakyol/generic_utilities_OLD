
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

typedef struct dl_list_node_s dl_list_node_t;

struct dl_list_node_s {

    dl_list_node_t *next, *prev;
    void *data;
};

typedef struct dl_list_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    dl_list_node_t *head, *tail;

    /*
     * used to order the list and/or search the list
     * for a specific data which matches.  Supplied
     * by the user at init time.  Can be null.
     */
    object_comparer cmp;

    /* how many nodes are in the list currently */
    int n;

} dl_list_t;

/*
 * initialize the list.  Return value is 0 for success
 * or a non zero errno.
 */
extern int
dl_list_init (dl_list_t *list,
    boolean make_it_thread_safe,
    object_comparer cmp,
    mem_monitor_t *parent_mem_monitor);

/*
 * Add user data to the beginning of the list.
 * Return value is 0 for success or a non zero
 * errno value.
 */
extern int 
dl_list_prepend_data (dl_list_t *list, void *data);

/*
 * Add user data to the end of the list.
 * Return value is 0 for success or a non zero
 * errno value.
 */
extern int
dl_list_append_data (dl_list_t *list, void *data);

/*
 * Finds the data stored in the list and if found, returns
 * the node in which it is srtored.  If not found, it returns
 * null.
 */
extern dl_list_node_t *
dl_list_find_data_node (dl_list_t *list, void *data);

/*
 * Delete a node in the list, used when you
 * already know the node to be deleted.
 */
extern void
dl_list_delete_node (dl_list_t *list, dl_list_node_t *node);

/*
 * Delete the user data from the list.  If a comparison function
 * to compare user data pointers was defined at the init time of the
 * list, then that is used to find the node/data which is then
 * deleted.  If the function was not specified at the initialization
 * time, then just a simple pointer comparison is done.
 * Return value is 0 for success or a non zero
 * errno value.
 */
extern int
dl_list_delete_data (dl_list_t *list, void *data);

/*
 * Destruction is complete, list cannot be used until
 * it is re-initialized again properly.
 */
extern void
dl_list_destroy (dl_list_t *list);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __DL_LIST_OBJECT_H__

