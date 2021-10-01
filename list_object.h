
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
** It ALWAYS is and WILL remain the sole property of Cihangir Metin Akyol.
**
** For proper indentation/viewing, regardless of which editor is being used,
** no tabs are used, ONLY spaces are used and the width of lines never
** exceed 80 characters.  This way, every text editor/terminal should
** display the code properly.  If modifying, please stick to this
** convention.
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
** This is a very simple linked list which can best be used as a fifo
** or a lifo.  What makes it extremely fast is the fact that you can
** add a data node only either to its head or its tail, which makes it
** extremely suitable to be used as either a fifo or a lifo.
**
** If you want to use it as a fifo, always add to its tail.
** If you want to use it as a lifo, always add to its head.  Then when you
** pop each element, you will always pop the nodes in the correct order.
** Popping the element will actually remove it from the list.
**
** Insertions & deletions are extremely fast, since the list always
** maintains a 'tail' pointer which always points to an 'empty' node
** which can be manipulated.
**
** The list also can be searched and random nodes can be deleted.
** Searches are linear but deletions are extremely fast.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

/*
 * singly linked list manager of arbitrary objects (void*)
 */

#ifndef __LIST_OBJECT_H__
#define __LIST_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"

typedef struct list_node_s list_node_t;
typedef struct list_obj_s list_obj_t;

/*
 * end of list is typically signified with a node which has
 * both 'data' and 'next' to be NULL.
 */
struct list_node_s {

    /* The list this node belongs to */
    list_obj_t *list;

    /* user data */
    void *data;

    /* next node */
    list_node_t *next;
};

typedef struct list_obj_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    /* where the list elements will come from */
    chunk_manager_t cmgr;

    /* how many elements in the list */
    int n;

    /* start of list, and the end node of list */
    list_node_t *head, *tail;

    /* to search a certain object in the list */
    object_comparer cmp;

    statistics_block_t stats;

} list_obj_t;

/*
 * creates a new list node with the fields correctly filled in.
 * The node will be returned in 'ret'.  Function value is 0 if
 * all went well, or a non zero error code otherwise.  If any
 * failure occurs, 'ret' will also be set to NULL.
 */
static int
list_obj_create_node (list_obj_t *list, void *data,
    list_node_t **ret)
{
    list_node_t *node;

    node = chunk_manager_alloc(&(list->cmgr));
    *ret = node;
    if (NULL == node) return ENOMEM;
    node->list = list;
    node->data = data;
    node->next = NULL;

    return 0;
}

static int
list_obj_create_tail_node (list_obj_t *list,
    list_node_t **ret)
{
    return
        list_obj_create_node(list, NULL, ret);
}

extern int
list_obj_init (list_obj_t *list,
    boolean make_it_thread_safe,
    object_comparer cmpf,
    mem_monitor_t *parent_mem_monitor)
{
    int err, number_of_initial_nodes;

    MEM_MONITOR_SETUP(list);
    LOCK_SETUP(list);

    list->n = 0;
    list->cmp = cmpf;

    /* create the initial fast cache of nodes */
    err = chunk_manager_init(&(list->cmgr), FALSE, sizeof(list_node_t),
            64, 256, &number_of_initial_nodes, parent_mem_monitor);
    if (err) goto bail;

    /* now append the last tail node to the list */
    err = list_obj_create_tail_node(list, &(list->tail));
    if (err) goto bail;
    
bail:
    OBJ_WRITE_UNLOCK(list);

    return err;
}

static list_node_t*
thread_unsafe_list_obj_search (list_obj_t *list, void *searched)
{
    int n;
    list_node_t *node;

    n = 0;
    node = list->head;
    while (n < list->n) {

        /* identical pointers; considered found */
        if (node->data == searched)
            return node;

        /* if compare function is specified, then compare */
        if (list->cmp) {
            if (list->cmp(node->data, searched) == 0) {
                return node;
            }
        }

        node = node->next;
        n++;
    }

    /* not found */
    return NULL;
}

static int
thread_unsafe_list_obj_insert_head (list_obj_t *list, void *data)
{
    int err;
    list_node_t *node;

    err = list_obj_create_node(list, data, &node);
    if (!err) {
        node->next = list->head;
        list->head = node;
        list->n++;
    }

    return err;
}

/*
 * same as above but insert to tail of list
 */
static int
thread_unsafe_list_obj_insert_tail (list_obj_t *list, void *data)
{
    int err;
    list_node_t *new_tail;

    /* if we cannot create a new tail, forget it */
    err = list_obj_create_tail_node(list, &new_tail);
    if (err) return err;

    /* copy the new info onto old tail and make the tail the new tail */
    list->tail->data = data;
    list->tail->next = new_tail;
    list->tail = new_tail;
    (list->n)++;

    return 0;
}

static int
thread_unsafe_list_obj_remove (list_obj_t *list, list_node_t *node)
{
}

/*
 * get the head node and pop it out of the list
 */
static int
thread_unsafe_list_obj_node_pop (list_obj_t *list, list_node_t **node)
{
    if (list->n > 0) {
        *node = list->head;
        list->head = list->head->next;
        (list->n)--;
        return 0;
    }
    *node = NULL;
    return ENODATA;
}

/*
 * same as 'thread_unsafe_list_obj_node_pop' but this one actually
 * returns the user data pointer (instead of the list node).
 */
static void*
thread_unsafe_list_obj_data_pop (list_obj_t *list)
{
    list_node_t *node;

    if (thread_unsafe_list_obj_node_pop(list, &node))
        return NULL;
    return
        node->data;
}

#ifdef __cplusplus
} // extern C
#endif

A


#endif // __LIST_OBJECT_H__

