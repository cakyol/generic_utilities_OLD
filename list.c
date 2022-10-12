
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

/******************************************************************************
 *
 * Node operations.
 *
 * These operations SHOULD NOT fail, so make sure you feed the
 * correct params into them.  The params should be checked for
 * validity BEFORE the node operations are called.
 */

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

static inline void
thread_unsafe_list_insert_node_after_node (list_t *list,
    list_node_t *node, list_node_t *new_node)
{
    /* append to the end of the list */
    if (null == node->next) {
        thread_unsafe_list_append_node(list, new_node);
        return;
    }

    node->next->prev = new_node;
    new_node->next = node->next;
    new_node->prev = node;
    node->next = new_node;
    list->n++;
}

static inline void
thread_unsafe_list_insert_node_before_node (list_t *list,
    list_node_t *node, list_node_t *new_node)
{
    /* append to the head of the list */
    if (node->prev == null) {
        thread_unsafe_list_prepend_node(list, new_node);
        return;
    }

    new_node->next = node;
    node->prev->next = new_node;
    new_node->prev = node->prev;
    node->prev = new_node;
    list->n++;
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

/******************************************************************************
 *
 * Data operations.
 *
 * These MAY fail, so make sure all are checked BEFORE calling
 * the node operations defined above this line.
 */

#define RETURN_ERROR_IF_LIST_IS_FULL(list) \
    do { \
        if ((list->n_max > 0) && (list->n >= list->n_max)) { \
            return ENOSPC; \
        } \
    } while (0)

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

static inline int 
thread_unsafe_list_prepend_data (list_t *list, void *data)
{
    list_node_t *node;

    RETURN_ERROR_IF_LIST_IS_FULL(list);

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

    RETURN_ERROR_IF_LIST_IS_FULL(list);

    node = list_new_node(list, data);
    if (node) {
        thread_unsafe_list_append_node(list, node);
        return 0;
    }
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
    thread_unsafe_list_insert_node_after_node(list, node, new_node);
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
    thread_unsafe_list_insert_node_before_node(list, node, new_node);
    return 0;
}

static inline list_node_t *
thread_unsafe_list_find_data_node (list_t *list,
    void *data)
{
    list_node_t *node;

    node = list->head;
    while (node) {
        if (data == node->data) return node;
        node = node->next;
    }
    return null;
}

/************************** Public functions **************************/

PUBLIC int
list_init (list_t *list,
    bool make_it_thread_safe,
    bool enable_statistics,
    int n_max,
    mem_monitor_t *parent_mem_monitor)
{
    MEM_MONITOR_SETUP(list);
    LOCK_SETUP(list);
    STATISTICS_SETUP(list);

    list->head = list->tail = null;
    list->n = 0;
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
    insertion_stats_update(list, failed);
    OBJ_WRITE_UNLOCK(list);

    return failed;
}

PUBLIC int
list_append_data (list_t *list, void *data)
{
    int failed;

    OBJ_WRITE_LOCK(list);
    failed = thread_unsafe_list_append_data(list, data);
    insertion_stats_update(list, failed);
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
    insertion_stats_update(list, failed);
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
    insertion_stats_update(list, failed);
    OBJ_WRITE_UNLOCK(list);

    return failed;
}

PUBLIC list_node_t *
list_find_data_node (list_t *list, void *data)
{
    list_node_t *node;

    OBJ_READ_LOCK(list);
    node = thread_unsafe_list_find_data_node(list, data);
    search_stats_update(list, (null == node));
    OBJ_READ_UNLOCK(list);

    return node;
}

PUBLIC int
list_remove_node (list_t *list, list_node_t *node)
{
    int failed;

    OBJ_WRITE_LOCK(list);
    if (node) {
        thread_unsafe_list_remove_node(list, node);
        failed = 0;
    } else {
        failed = EINVAL;
    }
    deletion_stats_update(list, failed);
    OBJ_WRITE_UNLOCK(list);

    return failed;
}

PUBLIC int
list_remove_data (list_t *list, void *data)
{
    list_node_t *node;
    int failed;

    OBJ_WRITE_LOCK(list);
    node = thread_unsafe_list_find_data_node(list, data);
    if (node) {
        thread_unsafe_list_remove_node(list, node);
        failed = 0;
    } else {
        failed = ENODATA;
    }
    deletion_stats_update(list, failed);
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




