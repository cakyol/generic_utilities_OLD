
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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "list_object.h"

/*
 * creates a new list node with the fields correctly filled in.
 * The node will be returned in 'ret'.  Can fail only if no
 * memory is left.
 */
static inline list_node_t *
list_object_create_node (list_object_t *list, void *data)
{
    list_node_t *node;

    node = (list_node_t*) MEM_MONITOR_ALLOC(list, sizeof(list_node_t));
    if (node) {
        node->list = list;
        node->data = data;
        node->next = NULL;
    }
    return node;
}

static list_node_t *
thread_unsafe_list_object_search (list_object_t *list,
    void *searched)
{
    int n;
    list_node_t *node;

    n = 0;
    node = list->head;
    if (list->cmp) {
        while (n < list->n) {
            if (list->cmp(node->data, searched) == 0)
                return node;
            n++;
            node = node->next;
        }
    } else {
        while (n < list->n) {
            if (node->data == searched)
                return node;
            n++;
            node = node->next;
        }
    }
    return null;
}

/*
 * Insert to head of list, used for a lifo implementation.
 */
static int
thread_unsafe_list_object_insert_head (list_object_t *list,
    void *data, list_node_t **inserted_node)
{
    int err;
    list_node_t *node;

    /* no more space in list */
    if (list->n_max && (list->n >= list->n_max))
        return ENOSPC;

    node = list_object_create_node(list, data);
    if (node) {
        node->next = list->head;
        list->head = node;
        safe_pointer_set(inserted_node, node);
        list->n++;
        err = 0;
    } else {
        safe_pointer_set(inserted_node, null);
        err = ENOMEM;
    }
    return err;
}

/*
 * Same as above but insert to tail of list.
 * Used for a fifo implementation.
 */
static int
thread_unsafe_list_object_insert_tail (list_object_t *list,
    void *data, list_node_t **inserted_node)
{
    list_node_t *new_tail;

    safe_pointer_set(inserted_node, null);

    /* no more space in list */
    if (list->n_max && (list->n >= list->n_max))
        return ENOSPC;

    /* if we cannot allocate a new tail, we cannot proceed */
    new_tail = list_object_create_node(list, NULL);
    if (NULL == new_tail) return ENOMEM;

    /* copy the new info onto old tail and make new tail the list tail */
    list->tail->data = data;
    list->tail->next = new_tail;
    safe_pointer_set(inserted_node, list->tail);
    list->tail = new_tail;
    (list->n)++;

    return 0;
}

static int
thread_unsafe_list_object_remove_node (list_object_t *list,
    list_node_t *node)
{
    list_node_t *next;

    /* should never remove the end of list marker */
    if (node == list->tail) return EFAULT;

    /* copy the next node over to this one */
    next = node->next;
    node->data = next->data;
    node->next = next->next;
    (list->n)--;

    /* if next was the tail, this one now becomes the new tail */
    if (next == list->tail) list->tail = node;

    /* return the next node back to storage */
    MEM_MONITOR_FREE(next);

    return 0;
}

static int
thread_unsafe_list_object_remove_data (list_object_t *list, void *data)
{
    list_node_t *node;

    node = thread_unsafe_list_object_search(list, data);
    if (NULL == node) return ENODATA;
    
    return
        thread_unsafe_list_object_remove_node(list, node);
}

/*
 * get the head node and pop it out of the list
 */
static list_node_t *
thread_unsafe_list_object_pop_node (list_object_t *list)
{
    list_node_t *node;

    if (list->n > 0) {
        node = list->head;
        list->head = list->head->next;
        (list->n)--;
        return node;
    }
    return NULL;
}

/***************************** 80 column separator ****************************/

PUBLIC int
list_object_init (list_object_t *list,
    boolean make_it_thread_safe,
    object_comparer cmpf,
    int n_max,
    mem_monitor_t *parent_mem_monitor)
{
    int err = 0;

    MEM_MONITOR_SETUP(list);
    LOCK_SETUP(list);

    list->n = 0;
    list->cmp = cmpf;

    /* set list content limit */
    list->n_max = (n_max <= 0) ? 0 : n_max;

    /* now append the last tail node to the list */
    list->tail = list_object_create_node(list, NULL);
    if (NULL == list->tail) {
        err = ENOMEM;
    }

    OBJ_WRITE_UNLOCK(list);

    return err;
}

PUBLIC int
list_object_insert_head (list_object_t *list,
    void *data, list_node_t **inserted_node)
{
    int rc;

    OBJ_WRITE_LOCK(list);
    rc = thread_unsafe_list_object_insert_head(list, data, inserted_node);
    OBJ_WRITE_UNLOCK(list);

    return rc;
}

PUBLIC int
list_object_insert_tail (list_object_t *list,
    void *data, list_node_t **inserted_node)
{
    int rc;

    OBJ_WRITE_LOCK(list);
    rc = thread_unsafe_list_object_insert_tail(list, data, inserted_node);
    OBJ_WRITE_UNLOCK(list);

    return rc;
}

PUBLIC list_node_t *
list_object_search (list_object_t *list, void *searched)
{
    list_node_t *node;

    OBJ_READ_LOCK(list);
    node = thread_unsafe_list_object_search(list, searched);
    OBJ_READ_UNLOCK(list);

    return node;
}

PUBLIC boolean
list_object_contains (list_object_t *list,
    void *searched, list_node_t **found)
{
    list_node_t *node;

    OBJ_READ_LOCK(list);
    node = thread_unsafe_list_object_search(list, searched);
    safe_pointer_set(found, node);
    OBJ_READ_UNLOCK(list);

    return (node != NULL);
}


PUBLIC int
list_object_remove_data (list_object_t *list, void *data)
{
    int rc;

    OBJ_WRITE_LOCK(list);
    rc = thread_unsafe_list_object_remove_data(list, data);
    OBJ_WRITE_UNLOCK(list);

    return rc;
}

PUBLIC int
list_object_remove_node (list_node_t *node)
{
    int rc;

    OBJ_WRITE_LOCK(node->list);
    rc = thread_unsafe_list_object_remove_node(node->list, node);
    OBJ_WRITE_UNLOCK(node->list);

    return rc;
}

PUBLIC void *
list_object_pop_data (list_object_t *list)
{
    list_node_t *node;
    void *data;

    OBJ_WRITE_LOCK(list);
    node = thread_unsafe_list_object_pop_node(list);
    data = node ? node->data : NULL;
    OBJ_WRITE_UNLOCK(list);

    return data;
}

PUBLIC void
list_object_destroy (list_object_t *list)
{
    list_node_t *node, *next;

    /* make sure that the end of list marker is also removed */

    OBJ_WRITE_LOCK(list);
    node = list->head;
    while (node) {
        next = node->next;
        MEM_MONITOR_FREE(node);
        node = next;
    }
    OBJ_WRITE_UNLOCK(list);
    LOCK_OBJ_DESTROY(list);
    memset(list, 0, sizeof(list_object_t));
}

#ifdef __cplusplus
} // extern C
#endif

