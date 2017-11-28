
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

#include "sll_object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

static inline sll_node_t *
new_sll_node (sll_object_t *sll, void *user_data)
{
    sll_node_t *node = MEM_MONITOR_ALLOC(sll, sizeof(sll_node_t));

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
thread_unsafe_sll_object_add (sll_object_t *sll,
	void *user_data)
{
    int result;
    sll_node_t *cur, *prev;
    sll_node_t *node = new_sll_node(sll, user_data);

    if (NULL == node) return ENOMEM;
    cur = sll->head;
    prev = NULL;
    while (not_sll_end_node(cur)) {
        result = sll->cmpf(cur->user_data, user_data);
        if (result >= 0) break;
        prev = cur;
        cur = cur->next;
    }
    if (prev) {
        node->next = prev->next;
        prev->next = node;
    } else {
        node->next = sll->head;
        sll->head = node;
    }
    sll->n++;
    return 0;
}

static int
thread_unsafe_sll_object_search (sll_object_t *sll,
	void *searched_data, 
	void **data_found,
        sll_node_t **node_found)
{
    int result;
    sll_node_t *temp = sll->head;

    while (not_sll_end_node(temp)) {

        result = sll->cmpf(temp->user_data, searched_data);

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
thread_unsafe_sll_object_add_once (sll_object_t *sll,
	void *user_data)
{
    void *found_data;
    sll_node_t *found_node;

    if (thread_unsafe_sll_object_search(sll, user_data, 
            &found_data, &found_node) == 0) {
                /* data is already in the list */
                return 0;
    }
    return
	thread_unsafe_sll_object_add(sll, user_data);
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
thread_unsafe_sll_node_delete (sll_object_t *sll,
        sll_node_t *node_tobe_deleted)
{
    void *to_free;

    /* should not delete end node, NEVER */
    if (sll_end_node(node_tobe_deleted))
        return EINVAL;

    to_free = node_tobe_deleted->next;
    *node_tobe_deleted = *(node_tobe_deleted->next);
    MEM_MONITOR_FREE(sll, to_free);
    sll->n--;

    return 0;
}

static int
thread_unsafe_sll_object_delete (sll_object_t *sll,
	void *to_be_deleted, 
	void **actual_data_deleted)
{
    sll_node_t *node_tobe_deleted;

    if (thread_unsafe_sll_object_search(sll, to_be_deleted, 
            actual_data_deleted, &node_tobe_deleted) == 0) {
                return
                    thread_unsafe_sll_node_delete(sll, node_tobe_deleted);
    }
    return ENODATA;
}

/**************************** Initialize *************************************/

PUBLIC int
sll_object_init (sll_object_t *sll,
	int make_it_thread_safe,
	comparison_function_t cmpf,
	mem_monitor_t *parent_mem_monitor)
{
    sll_node_t *node;
    int rv = 0;

    LOCK_SETUP(sll);
    MEM_MONITOR_SETUP(sll);

    /* create the permanent "end" node */
    node = (sll_node_t*) MEM_MONITOR_ALLOC(sll, sizeof(sll_node_t));
    if (node) {
        node->next = NULL;
        node->user_data = NULL;
        sll->head = node;
        sll->n = 0;
        sll->cmpf = cmpf;
    } else {
        rv = ENOMEM;
    }
    WRITE_UNLOCK(sll);

    return rv;
}

/**************************** Insert/add *************************************/

PUBLIC int
sll_object_add (sll_object_t *sll, void *user_data)
{
    int rv;

    WRITE_LOCK(sll);
    rv = thread_unsafe_sll_object_add(sll, user_data);
    WRITE_UNLOCK(sll);
    return rv;
}

PUBLIC int
sll_object_add_once (sll_object_t *sll, void *user_data)
{
    int rv;

    WRITE_LOCK(sll);
    rv = thread_unsafe_sll_object_add_once(sll, user_data);
    WRITE_UNLOCK(sll);
    return rv;
}

/**************************** Search *****************************************/

PUBLIC int
sll_object_search (sll_object_t *sll,
	void *searched_data,
	void **data_found)
{
    int rv;
    sll_node_t *node_found;

    WRITE_LOCK(sll);
    rv = thread_unsafe_sll_object_search(sll, searched_data, 
            data_found, &node_found);
    WRITE_UNLOCK(sll);
    return rv;
}

/**************************** Remove/delete **********************************/ 

PUBLIC int
sll_object_delete (sll_object_t *sll,
	void *to_be_deleted,
	void **actual_data_deleted)
{
    int rv;

    WRITE_LOCK(sll);
    rv = thread_unsafe_sll_object_delete(sll, to_be_deleted, actual_data_deleted);
    WRITE_UNLOCK(sll);
    return rv;
}

/**************************** Traverse/iterate *******************************/ 

PUBLIC int
sll_object_iterate (sll_object_t *sll, traverse_function_t tfn,
        void *p0, void *p1, void *p2, void *p3)
{
    int rv = 0;
    sll_node_t *iter;

    READ_LOCK(sll);
    iter = sll->head;
    while (not_sll_end_node(iter)) {
        rv = tfn(sll, iter, iter->user_data, p0, p1, p2, p3);
        if (rv) break;
        iter = iter->next;
    }
    READ_UNLOCK(sll);
    return rv;
}

/**************************** Destroy ****************************************/ 

PUBLIC void
sll_object_destroy (sll_object_t *sll)
{
    sll_node_t *next, *iter;

    WRITE_LOCK(sll);
    iter = sll->head;
    while (not_sll_end_node(iter)) {
        next = iter->next;
        MEM_MONITOR_FREE(sll, iter);
        iter = next;
    }
    MEM_MONITOR_FREE(sll, sll->head);
    sll->n = 0;
    sll->head = NULL;
    sll->mem_mon_p = NULL;
    sll->cmpf = NULL;
    WRITE_UNLOCK(sll);
    LOCK_OBJ_DESTROY(sll);
}

#ifdef __cplusplus
} // extern C
#endif 





