
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol (gee.akyol@gmail.com, gee_akyol@yahoo.com)
** Copyright: Cihangir Metin Akyol, March 2016, 2017
**
** This code is developed by and belongs to Cihangir Metin Akyol.  
** It is NOT owned by any company or consortium.  It is the sole
** property and work of one individual.
**
** It can be used by ANYONE or ANY company for ANY purpose as long 
** as NO ownership and/or patent claims are made to it by such persons 
** or companies.
**
** It ALWAYS is and WILL remain the property of Cihangir Metin Akyol.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#include "linkedlist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

static inline linkedlist_node_t *
new_linkedlist_node (linkedlist_t *listp, void *user_data)
{
    linkedlist_node_t *node = MEM_MONITOR_ALLOC(listp, sizeof(linkedlist_node_t));

    if (node) {
	node->next = NULL;
	node->user_data = user_data;
    }
    return node;
}

/*
 * add in ascending order based on the comparison function provided
 */
static int
thread_unsafe_linkedlist_add (linkedlist_t *listp,
	void *user_data)
{
    int result;
    linkedlist_node_t *cur, *prev;
    linkedlist_node_t *node = new_linkedlist_node(listp, user_data);

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
        node->next = listp->head;
        listp->head = node;
    }
    listp->n++;
    return 0;
}

static int
thread_unsafe_linkedlist_search (linkedlist_t *listp,
	void *searched_data, 
	void **data_found,
        linkedlist_node_t **node_found)
{
    int result;
    linkedlist_node_t *temp = listp->head;

    while (not_endof_linkedlist(temp)) {

        result = listp->cmpf(temp->user_data, searched_data);

        /* found exact match */
	if (result == 0) {
	    *data_found = temp->user_data;
            *node_found = temp;
	    return 0;
	}

        /*
         * all the nodes past here must have values greater so
         * no point in continuing to search the rest of the list
         */
        if (result > 0) break;

        temp = temp->next;
    }

    *node_found = NULL;
    return ENODATA;
}

static int
thread_unsafe_linkedlist_add_once (linkedlist_t *listp,
	void *user_data)
{
    void *found_data;
    linkedlist_node_t *found_node;

    if (thread_unsafe_linkedlist_search(listp, user_data, 
            &found_data, &found_node) == 0) {
                /* data is already in the list */
                return 0;
    }
    return
	thread_unsafe_linkedlist_add(listp, user_data);
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
static int
thread_unsafe_linkedlist_node_delete (linkedlist_t *listp,
        linkedlist_node_t *node_tobe_deleted)
{
    void *to_free;

    /* should not delete end node, NEVER */
    if (endof_linkedlist(node_tobe_deleted))
        return EINVAL;

    to_free = node_tobe_deleted->next;
    *node_tobe_deleted = *(node_tobe_deleted->next);
    MEM_MONITOR_FREE(listp, to_free);
    listp->n--;

    return 0;
}

static int
thread_unsafe_linkedlist_delete (linkedlist_t *listp,
	void *to_be_deleted, 
	void **actual_data_deleted)
{
    linkedlist_node_t *node_tobe_deleted;

    if (thread_unsafe_linkedlist_search(listp, to_be_deleted, 
            actual_data_deleted, &node_tobe_deleted) == 0) {
                return
                    thread_unsafe_linkedlist_node_delete(listp, node_tobe_deleted);
    }
    return ENODATA;
}

/**************************** Initialize *************************************/

PUBLIC int
linkedlist_init (linkedlist_t *listp,
	int make_it_thread_safe,
	comparison_function_t cmpf,
	mem_monitor_t *parent_mem_monitor)
{
    linkedlist_node_t *node;
    int rv = 0;

    LOCK_SETUP(listp);
    MEM_MONITOR_SETUP(listp);

    /* create the permanent "end" node */
    node = (linkedlist_node_t*) MEM_MONITOR_ALLOC(listp, sizeof(linkedlist_node_t));
    if (node) {
        node->next = NULL;
        node->user_data = NULL;
        listp->head = node;
        listp->n = 0;
        listp->cmpf = cmpf;
    } else {
        rv = ENOMEM;
    }
    WRITE_UNLOCK(listp);

    return rv;
}

/**************************** Insert/add *************************************/

PUBLIC int
linkedlist_add (linkedlist_t *listp, void *user_data)
{
    int rv;

    WRITE_LOCK(listp);
    rv = thread_unsafe_linkedlist_add(listp, user_data);
    WRITE_UNLOCK(listp);
    return rv;
}

PUBLIC int
linkedlist_add_once (linkedlist_t *listp, void *user_data)
{
    int rv;

    WRITE_LOCK(listp);
    rv = thread_unsafe_linkedlist_add_once(listp, user_data);
    WRITE_UNLOCK(listp);
    return rv;
}

/**************************** Search *****************************************/

PUBLIC int
linkedlist_search (linkedlist_t *listp,
	void *searched_data,
	void **data_found)
{
    int rv;
    linkedlist_node_t *node_found;

    WRITE_LOCK(listp);
    rv = thread_unsafe_linkedlist_search(listp, searched_data, 
            data_found, &node_found);
    WRITE_UNLOCK(listp);
    return rv;
}

/**************************** Remove/delete **********************************/ 

PUBLIC int
linkedlist_delete (linkedlist_t *listp,
	void *to_be_deleted,
	void **actual_data_deleted)
{
    int rv;

    WRITE_LOCK(listp);
    rv = thread_unsafe_linkedlist_delete(listp, to_be_deleted, actual_data_deleted);
    WRITE_UNLOCK(listp);
    return rv;
}

/**************************** Traverse/iterate *******************************/ 

PUBLIC int
linkedlist_iterate (linkedlist_t *listp, traverse_function_t tfn,
        void *p0, void *p1, void *p2, void *p3)
{
    int rv = 0;
    linkedlist_node_t *iter;

    READ_LOCK(listp);
    iter = listp->head;
    while (not_endof_linkedlist(iter)) {
        rv = tfn(listp, iter, iter->user_data, p0, p1, p2, p3);
        if (rv) break;
        iter = iter->next;
    }
    READ_UNLOCK(listp);
    return rv;
}

/**************************** Destroy ****************************************/ 

PUBLIC void
linkedlist_destroy (linkedlist_t *listp)
{
    linkedlist_node_t *next, *iter;

    WRITE_LOCK(listp);
    iter = listp->head;

    /* destroy all user elements */
    while (not_endof_linkedlist(iter)) {
        next = iter->next;
        MEM_MONITOR_FREE(listp, iter);
        iter = next;
    }

    /* now destroy the end marker */
    MEM_MONITOR_FREE(listp, listp->head);

    /* clear everything */
    listp->n = 0;
    listp->head = NULL;
    listp->mem_mon_p = NULL;
    listp->cmpf = NULL;
    WRITE_UNLOCK(listp);
    LOCK_OBJ_DESTROY(listp);
}

#ifdef __cplusplus
} // extern C
#endif 





