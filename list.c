
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

#define RETURN_ERROR_IF_LIST_IS_FULL(list) \
    do { \
        if ((list->n_max > 0) && (list->n >= list->n_max)) { \
            insertion_failed(list); \
            return ENOSPC; \
        } \
    } while (0)

static inline int 
thread_unsafe_list_prepend_data (list_t *list, void *data)
{
    list_node_t *node;

    RETURN_ERROR_IF_LIST_IS_FULL(list);

    node = list_new_node(list, data);
    if (node) {
        thread_unsafe_list_prepend_node(list, node);
        insertion_succeeded(list);
        return 0;
    }
    insertion_failed(list);
    return ENOMEM;
}

static inline int
thread_unsafe_list_append_data (list_t *list, void *data)
{
    list_node_t *node;

    RETURN_ERROR_IF_LIST_IS_FULL(list);

    node = list_new_node(list, data);
    if (node) {
        thread_unsafe_list_append_node(list, node);
        insertion_succeeded(list);
        return 0;
    }
    insertion_failed(list);
    return ENOMEM;
}

static inline int
thread_unsafe_list_insert_data_after_node (list_t *list,
    list_node_t *node, void *data)
{
    list_node_t *new_node;

    if (null == node) return EINVAL;
    RETURN_ERROR_IF_LIST_IS_FULL(list);
    new_node = list_new_node(list, data);
    if (null == new_node) return ENOMEM;

    /* append to the end of the list */
    if (null == node->next) {
        thread_unsafe_list_append_node(list, new_node);
        return 0;
    }

    node->next->prev = new_node;
    new_node->next = node->next;
    new_node->prev = node;
    node->next = new_node;

    list->n++;
    insertion_succeeded(list);

    return 0;
}

static inline int
thread_unsafe_list_insert_data_before_node (list_t *list,
    list_node_t *node, void *data)
{
    list_node_t *new_node;

    if (null == node) return EINVAL;
    RETURN_ERROR_IF_LIST_IS_FULL(list);
    new_node = list_new_node(list, data);
    if (null == new_node) return ENOMEM;

    /* append to the head of the list */
    if (node->prev == null) {
        thread_unsafe_list_prepend_node(list, new_node);
        return 0;
    }

    new_node->next = node;
    node->prev->next = new_node;
    new_node->prev = node->prev;
    node->prev = new_node;

    list->n++;
    insertion_succeeded(list);

    return 0;
}

/*
 * Note that object comparer function is always non null,
 * guaranteed to be so from the list initialization function.
 * Therefore we can safely access it without worrying it may
 * be null.
 */
static inline list_node_t *
thread_unsafe_list_find_data_node (list_t *list,
    void *data)
{
    list_node_t *node;

    node = list->head;
    while (node) {
        if ((list->cmp)(data, node->data) == 0) {
            search_succeeded(list);
            return node;
        }
        node = node->next;
    }
    search_failed(list);
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
    deletion_succeeded(list);
}

/*
 * return 0 if the same pointer
 */
static int
default_comparer (void *obj1, void *obj2)
{
    return obj1 - obj2;
}

/************************** Public functions **************************/

/*
 * If a comparer function is not specified, install a default
 * one which simply compares the data pointers themselves.
 */
PUBLIC int
list_init (list_t *list,
    bool make_it_thread_safe,
    bool statistics_wanted,
    int n_max,
    object_comparer cmp,
    mem_monitor_t *parent_mem_monitor)
{
    MEM_MONITOR_SETUP(list);
    LOCK_SETUP(list);
    STATISTICS_SETUP(list);

    list->head = list->tail = null;
    list->n = 0;
    list->cmp = cmp ? cmp : default_comparer;
    list->n_max = (n_max > 0) ? n_max : 0;
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

PUBLIC int
list_insert_data_after_node (list_t *list,
    list_node_t *node, void *data)
{
    int failed;

    OBJ_WRITE_LOCK(list);
    failed = thread_unsafe_list_insert_data_after_node(list, node, data);
    OBJ_WRITE_UNLOCK(list);
    return failed;
}

PUBLIC int
list_insert_data_before_node (list_t *list,
    list_node_t *node, void *data)
{
    int failed;

    OBJ_WRITE_LOCK(list);
    failed = thread_unsafe_list_insert_data_before_node(list, node, data);
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




