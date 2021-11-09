
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
** doubly linked list
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline list_node_t *
list_new_node (list_t *list, void *data)
{
    list_node_t *node;

    node = MEM_MONITOR_ALLOC(list, sizeof(list_node_t));
    if (node) {
        node->next = node->prev = null;
        node->data = data;
    }
    return node;
}

static inline void
thread_unsafe_list_prepend_node (list_t *list,
    list_node_t *node)
{
    if (list->n <= 0) {
        list->head = list->tail = node;
    } else {
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->n++;
}

static inline void
thread_unsafe_list_append_node (list_t *list,
    list_node_t *node)
{
    if (list->n <= 0) {
        list->head = list->tail = node;
    } else {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }
    list->n++;
}

static inline int 
thread_unsafe_list_prepend_data (list_t *list, void *data)
{
    list_node_t *node;
    
    node = list_new_node(list, data);
    if (node) {
        thread_unsafe_list_prepend_node(list, node);
        return 0;
    }
    return ENOMEM;
}

static inline int
thread_unsafe_list_append_data (list_t *list, void *data)
{
    list_node_t *node;
    
    node = list_new_node(list, data);
    if (node) {
        thread_unsafe_list_append_node(list, node);
        return 0;
    }
    return ENOMEM;
}

static inline list_node_t *
thread_unsafe_list_find_data_node (list_t *list,
    void *data)
{
    list_node_t *node;

    node = list->head;
    while (node) {
        if (list->cmp) {
            if ((list->cmp)(data, node->data) == 0) return node;
        } else {
            if (data == node->data) return node;
        }
        node = node->next;
    }
    return null;
}

static inline void
thread_unsafe_list_remove_node (list_t *list,
    list_node_t *node)
{
    if (node->next == null) {
        if (node->prev == null) {
            list->head = list->tail = null;
        } else {
            node->prev->next = null;
            list->tail = node->prev;
        }
    } else {
        if (node->prev == null) {
            list->head = node->next;
            node->next->prev = null;
        } else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
    }
    MEM_MONITOR_FREE(node);
    list->n--;
}

/************************** Public functions **************************/

PUBLIC int
list_init (list_t *list,
    boolean make_it_thread_safe,
    object_comparer cmp,
    mem_monitor_t *parent_mem_monitor)
{
    MEM_MONITOR_SETUP(list);
    LOCK_SETUP(list);
    list->head = list->tail = null;
    list->n = 0;
    list->cmp = cmp;
    OBJ_WRITE_UNLOCK(list);

    return 0;
}

PUBLIC int
list_prepend_data (list_t *list, void *data)
{
    int failed;

    OBJ_WRITE_LOCK(list);
    failed = thread_unsafe_list_prepend_data(list, data);
    OBJ_WRITE_UNLOCK(list);
    return failed;
}

PUBLIC int
list_append_data (list_t *list, void *data)
{
    int failed;

    OBJ_WRITE_LOCK(list);
    failed = thread_unsafe_list_append_data(list, data);
    OBJ_WRITE_UNLOCK(list);
    return failed;
}

PUBLIC list_node_t *
list_find_data_node (list_t *list, void *data)
{
    list_node_t *node;

    OBJ_READ_LOCK(list);
    node = thread_unsafe_list_find_data_node(list, data);
    OBJ_READ_UNLOCK(list);

    return node;
}

PUBLIC void
list_remove_node (list_t *list, list_node_t *node)
{
    OBJ_WRITE_LOCK(list);
    thread_unsafe_list_remove_node(list, node);
    OBJ_WRITE_UNLOCK(list);
}

PUBLIC int
list_remove_data (list_t *list, void *data)
{
    list_node_t *node;
    int failed;

    OBJ_WRITE_LOCK(list);
    node = thread_unsafe_list_find_data_node(list, data);
    if (node) {
        list_remove_node(list, node);
        failed = 0;
    } else {
        failed = ENODATA;
    }
    OBJ_WRITE_UNLOCK(list);

    return failed;
}

void
list_destroy (list_t *list)
{
    list_node_t *node, *next_node;

    OBJ_WRITE_LOCK(list);
    node = list->head;
    while (node) {
        next_node = node->next;
        MEM_MONITOR_FREE(node);
        node = next_node;
    }
    OBJ_WRITE_UNLOCK(list);
    memset(list, 0, sizeof(list_t));
    lock_obj_destroy(list->lock);
}

#ifdef __cplusplus
} // extern C
#endif 



