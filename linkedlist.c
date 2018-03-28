
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

#include <stdlib.h>
#include <assert.h>
#include "linkedlist.h"

#define PUBLIC

#ifdef __cplusplus
extern "C" {
#endif

static linkedlist_node_t *
linkedlist_new_node (linkedlist_t *listp, void *user_data)
{
    linkedlist_node_t *n;

    n = (linkedlist_node_t*) 
	MEM_MONITOR_ALLOC(listp, sizeof(linkedlist_node_t));
    if (n) {
	n->list = listp;
	n->user_data = user_data;
	n->next = NULL;
    }
    return n;
}

/*
 * DONT MAKE THIS STATIC, it is used in scheduler.c
 *
 * data is always unconditionally added to the head of
 * the list, no duplicate checking is done.
 */
int
thread_unsafe_linkedlist_add_to_head (linkedlist_t *listp, void *user_data)
{
    linkedlist_node_t *node;

    node = linkedlist_new_node(listp, user_data);
    if (NULL == node) return ENOMEM;

    /* insert to head */
    if (listp->head) node->next = listp->head;
    listp->head = node;
    listp->n++;

    /* done */
    return 0;
}

/*
 * add the new node/data based on ordering defined by 
 * the user specified comparison function
 */
static int
thread_unsafe_linkedlist_add (linkedlist_t *listp, void *user_data,
        linkedlist_node_t **node_added)
{
    int result;
    linkedlist_node_t *cur, *prev;
    linkedlist_node_t *node;
    
    node = linkedlist_new_node(listp, user_data);
    if (NULL == node) return ENOMEM;

    cur = listp->head;
    prev = NULL;
    while (not_endof_linkedlist(cur)) {
        result = listp->cmpf(cur->user_data, user_data);
        if (result >= 0) break;
        prev = cur;
        cur = cur->next;
    }
    if (prev) {
        node->next = prev->next;
        prev->next = node;
    } else {
        /* first element in the list */
        node->next = listp->head;
        listp->head = node;
    }
    listp->n++;
    *node_added = node;
    return 0;
}

static int
thread_unsafe_linkedlist_search (linkedlist_t *listp, void *searched_data,
        void **data_found,
        linkedlist_node_t **node_found,
        linkedlist_node_t **node_before)
{
    int result;
    linkedlist_node_t *cur, *prev;

    cur = listp->head;
    *node_before = prev = NULL;
    while (not_endof_linkedlist(cur)) {

        result = listp->cmpf(cur->user_data, searched_data);

        /* an exact match */
        if (0 == result) {
            *data_found = cur->user_data;
            *node_found = cur;
            return 0;
        }

        /*
         * all the nodes past here must have values greater so
         * no point in continuing to search the rest of the list
         */
        if (result > 0) break;
        prev = cur;
        cur = cur->next;
    }

    /* not found */
    *node_before = prev;
    *data_found = NULL;
    *node_found = NULL;
    return ENODATA;
}

/*
 * This will succeed if no malloc errors occur but the
 * 'data_found' and 'node_found' may or may not be NULL,
 * depending on whether the data to be added was already
 * in the list or not.
 */
static int
thread_unsafe_linkedlist_add_once (linkedlist_t *listp, void *user_data, 
        void **data_found,
        linkedlist_node_t **node_found)
{
    int rv;
    linkedlist_node_t *node_added = NULL;
    linkedlist_node_t *prev;

    rv = thread_unsafe_linkedlist_search
            (listp, user_data, data_found, node_found, &prev);

    /* found, already in list */
    if (0 == rv) return 0;
    
    /* create and populate the new node */
    node_added = (linkedlist_node_t*)
                    MEM_MONITOR_ALLOC(listp, sizeof(linkedlist_node_t));
    if (NULL == node_added) return ENOMEM;
    node_added->list = listp;
    node_added->user_data = user_data;

    /* insert new node into its proper position */
    if (prev) {
        node_added->next = prev->next;
        prev->next = node_added;
    } else {
        node_added->next = listp->head;
        listp->head = node_added;
    }

    /* one more node */
    listp->n++;

    return 0;
}

/*
 * This is very clever, I got this off the internet.
 * Not sure about the authors name but I am quoting
 * it just to acknowledge his/her contribution.
 * The site from which the idea is taken is:
 * http://www.geeksforgeeks.org/in-a-linked-list-given-only-a-\
 *     pointer-to-a-node-to-be-deleted-in-a-singly-linked-list-\
 *     how-do-you-delete-it/
 *
 * Since we always have an "end of list" marker node 
 * (which is never NULL), this algorithm works even for 
 * deleting the last node in the list.
 */
int
thread_unsafe_linkedlist_node_delete (linkedlist_t *listp,
        linkedlist_node_t *node_tobe_deleted)
{
    void *to_free;

    /* should not delete end node, NEVER */
    if (endof_linkedlist(node_tobe_deleted))
        return EINVAL;

    /* if node is not part of list, no go */
    if (listp != node_tobe_deleted->list)
        return EINVAL;

    to_free = node_tobe_deleted->next;
    *node_tobe_deleted = *(node_tobe_deleted->next);
    MEM_MONITOR_FREE(listp, to_free);
    listp->n--;

    return 0;
}

static int
thread_unsafe_linkedlist_delete (linkedlist_t *listp, void *data_to_be_deleted, 
        void **data_deleted, linkedlist_node_t **node_deleted)
{
    int rv;
    linkedlist_node_t *prev;

    rv = thread_unsafe_linkedlist_search
            (listp, data_to_be_deleted, data_deleted, node_deleted, &prev);
    if ((0 == rv) && node_deleted) {
        return
            thread_unsafe_linkedlist_node_delete(listp, *node_deleted);
    }
    return ENODATA;
}

/**************************** Initialize *************************************/

PUBLIC int
linkedlist_init (linkedlist_t *listp,
        int make_it_thread_safe,
        object_comparer cmpf,
        mem_monitor_t *parent_mem_monitor)
{
    int rv = ENOMEM;
    linkedlist_node_t *last_node;

    if (NULL == cmpf) return EINVAL;

    MEM_MONITOR_SETUP(listp);
    LOCK_SETUP(listp);

    last_node = (linkedlist_node_t*) 
                    MEM_MONITOR_ALLOC(listp, sizeof(linkedlist_node_t));
    if (last_node) {
        last_node->list = listp;
        last_node->next = NULL;
        last_node->user_data = NULL;
        listp->head = last_node;
        listp->n = 0;
        listp->cmpf = cmpf;
        rv = 0;
    }
    WRITE_UNLOCK(listp);
    return rv;
}

/**************************** Insert/add *************************************/

PUBLIC int
linkedlist_add (linkedlist_t *listp, void *user_data)
{
    int rv;
    linkedlist_node_t *node_added;

    WRITE_LOCK(listp);
    rv = thread_unsafe_linkedlist_add(listp, user_data, &node_added);
    WRITE_UNLOCK(listp);
    return rv;
}

PUBLIC int
linkedlist_add_once (linkedlist_t *listp, void *user_data, 
        void **data_found)
{
    int rv;
    linkedlist_node_t *node_found;

    WRITE_LOCK(listp);
    rv = thread_unsafe_linkedlist_add_once
            (listp, user_data, data_found, &node_found);
    WRITE_UNLOCK(listp);
    return rv;
}

PUBLIC int
linkedlist_add_to_head (linkedlist_t *listp, void *user_data)
{
    int rv;

    WRITE_LOCK(listp);
    rv = thread_unsafe_linkedlist_add_to_head(listp, user_data);
    WRITE_UNLOCK(listp);
    return rv;
}

/**************************** Search *****************************************/

PUBLIC int
linkedlist_search (linkedlist_t *listp, void *searched_data,
        void **data_found)
{
    int rv;
    linkedlist_node_t *node_found, *prev;

    READ_LOCK(listp);
    rv = thread_unsafe_linkedlist_search
            (listp, searched_data, data_found, &node_found, &prev);
    READ_UNLOCK(listp);
    return rv;
}

/**************************** Remove/delete **********************************/ 

PUBLIC int
linkedlist_delete (linkedlist_t *listp, void *data_to_be_deleted,
        void **data_deleted)
{
    int rv;
    linkedlist_node_t *node_deleted;

    WRITE_LOCK(listp);
    rv = thread_unsafe_linkedlist_delete(listp, data_to_be_deleted,
                data_deleted, &node_deleted);
    WRITE_UNLOCK(listp);
    return rv;
}

PUBLIC int
linkedlist_delete_node (linkedlist_t *listp,
        linkedlist_node_t *node_to_be_deleted)
{
    int rv;

    WRITE_LOCK(listp);
    rv = thread_unsafe_linkedlist_node_delete(listp, node_to_be_deleted);
    WRITE_UNLOCK(listp);
    return rv;
}

/**************************** Destroy ****************************************/ 

PUBLIC void
linkedlist_destroy (linkedlist_t *listp)
{
    WRITE_LOCK(listp);
    while (not_endof_linkedlist(listp->head)) {
        thread_unsafe_linkedlist_node_delete(listp, listp->head);
    }

    /* free up the end of list marker */
    MEM_MONITOR_FREE(listp, listp->head);
    listp->head = NULL;

    /* check sanity */
    assert(0 == listp->n);

    WRITE_UNLOCK(listp);
}

#ifdef __cplusplus
} // extern C
#endif 




