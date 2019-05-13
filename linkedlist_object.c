
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
#include "linkedlist_object.h"

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
 *
 * NOTE THAT THE USE OF THIS FUNCTION WILL COMPLETELY
 * DESTROY THE OBJECT ORDERING IN THE LIST WHICH WILL
 * CAUSE ALL SUBSEQUENT INSERT, SEARCH AND DELETE
 * OPERATIONS TO GO BERSERK.
 */
int
thread_unsafe_linkedlist_add_to_head (linkedlist_t *listp, void *user_data,
        linkedlist_node_t **node_added)
{
    linkedlist_node_t *node;

    node = linkedlist_new_node(listp, user_data);
    if (NULL == node) {
        safe_pointer_set(node_added, NULL);
        return ENOMEM;
    }

    /* insert to head */
    if (listp->head) node->next = listp->head;
    listp->head = node;
    listp->n++;

    /* return node to user */
    safe_pointer_set(node_added, node);

    /* done */
    return 0;
}

/*
 * add the new node/data based on ordering defined by 
 * the user specified comparison function.
 */
static int
thread_unsafe_linkedlist_add (linkedlist_t *listp, void *user_data,
        linkedlist_node_t **node_added)
{
    int result;
    linkedlist_node_t *cur, *prev;
    linkedlist_node_t *node;
    
    node = linkedlist_new_node(listp, user_data);
    if (NULL == node) {
        safe_pointer_set(node_added, NULL);
        return ENOMEM;
    }
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
    safe_pointer_set(node_added, node);

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

    prev = NULL;
    cur = listp->head;
    while (not_endof_linkedlist(cur)) {

        result = listp->cmpf(cur->user_data, searched_data);

        /* an exact match */
        if (0 == result) {
            safe_pointer_set(data_found, cur->user_data);
            safe_pointer_set(node_found, cur);
            safe_pointer_set(node_before, prev);
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
    safe_pointer_set(data_found, NULL);
    safe_pointer_set(node_found, NULL);
    safe_pointer_set(node_before, prev);
    return ENODATA;
}

/*
 * This function will ensure that only one of the desired
 * entry gets inserted into the list.  0 will be returned
 * if successfull.  EEXIST will be returned if the exact same
 * entry is already in the list.  ENOMEM will be returned
 * if a new node cannot be allocated due to memory exhaustion.
 */
static int
thread_unsafe_linkedlist_add_once (linkedlist_t *listp, void *user_data, 
        void **data_found, linkedlist_node_t **node_found)
{
    int failed;
    linkedlist_node_t *node_added, *prev;

    failed =
        thread_unsafe_linkedlist_search(listp, user_data,
                data_found, node_found, &prev);

    /* found, already in list */
    if (0 == failed) return EEXIST;
    
    /* create the new node */
    node_added = linkedlist_new_node(listp, user_data);
    if (NULL == node_added) return ENOMEM;

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
thread_unsafe_linkedlist_delete (linkedlist_t *listp,
        void *data_to_be_deleted, void **data_deleted)
{
    int failed;
    linkedlist_node_t *nd;

    failed = thread_unsafe_linkedlist_search
                (listp, data_to_be_deleted, data_deleted, &nd, NULL);
    if ((0 == failed) && nd) {
        return
            thread_unsafe_linkedlist_node_delete(listp, nd);
    }
    return failed;
}

/**************************** Initialize *************************************/

PUBLIC int
linkedlist_init (linkedlist_t *listp,
        int make_it_thread_safe,
        object_comparer cmpf,
        mem_monitor_t *parent_mem_monitor)
{
    int failed = ENOMEM;
    linkedlist_node_t *last_node;

    /* MUST have a compare function since list is ordered */
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
        failed = 0;
    }
    WRITE_UNLOCK(listp);
    return failed;
}

/**************************** Insert/add *************************************/

PUBLIC int
linkedlist_add (linkedlist_t *listp, void *user_data)
{
    int failed;

    WRITE_LOCK(listp);
    failed = thread_unsafe_linkedlist_add(listp, user_data, NULL);
    WRITE_UNLOCK(listp);
    return failed;
}

PUBLIC int
linkedlist_add_once (linkedlist_t *listp, void *user_data, 
        void **data_found)
{
    int failed;

    WRITE_LOCK(listp);
    failed =
        thread_unsafe_linkedlist_add_once(listp, user_data, data_found, NULL);
    WRITE_UNLOCK(listp);
    return failed;
}

/**************************** Search *****************************************/

PUBLIC int
linkedlist_search (linkedlist_t *listp, void *searched_data,
        void **data_found)
{
    int failed;

    READ_LOCK(listp);
    failed = thread_unsafe_linkedlist_search
                (listp, searched_data, data_found, NULL, NULL);
    READ_UNLOCK(listp);
    return failed;
}

/**************************** Remove/delete **********************************/ 

PUBLIC int
linkedlist_delete (linkedlist_t *listp, void *data_to_be_deleted,
        void **data_deleted)
{
    int failed;

    WRITE_LOCK(listp);
    failed = thread_unsafe_linkedlist_delete(listp, data_to_be_deleted,
                data_deleted);
    WRITE_UNLOCK(listp);
    return failed;
}

PUBLIC int
linkedlist_delete_node (linkedlist_t *listp,
        linkedlist_node_t *node_to_be_deleted)
{
    int failed;

    WRITE_LOCK(listp);
    failed = thread_unsafe_linkedlist_node_delete(listp, node_to_be_deleted);
    WRITE_UNLOCK(listp);
    return failed;
}

/**************************** Destroy ****************************************/ 

PUBLIC void
linkedlist_destroy (linkedlist_t *listp,
        data_delete_callback_t ddcbf, void *ddcbf_arg)
{
    volatile void *user_data;

    WRITE_LOCK(listp);
    while (not_endof_linkedlist(listp->head)) {
        user_data = listp->head->user_data;
        thread_unsafe_linkedlist_node_delete(listp, listp->head);
        if (ddcbf) ddcbf((void*) user_data, ddcbf_arg);
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




