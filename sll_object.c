
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

/*
 * We maintain an "end node" always in the list.  This is represented
 * by setting its 'next' pointer to the value of 0x1.  We cannot use 
 * the datum value of NULL (or 0) as an end node indicator since users MAY
 * want to use those values as ligitimate data to be stored in the list.
 * Alternatively, If I introduce another field into the structure, it will 
 * bump up its size by at least 4 bytes wasting too much memory.
 * Therefore I am using the kludge mentioned above which works very well.
 *
 * The reason we need an "end node" representation is so that we
 * can delete a node VERY quickly.  What we do to delete a node
 * is actually copy the contents of the NEXT node onto THIS one and delete
 * the NEXT node.  This will simulate the effect of having deleted THIS
 * node, which is exactly what is wanted.  And since we always have the 
 * "next" node pointer available even in the LAST valid node, this
 * is very easy.  However, this would fail if the last node was being
 * deleted since there would be nothing past the last node.  To alleviate
 * this we ALWAYS maintain an "end node".  The "end node" holds NO data
 * and is ONLY a marker.
 */

/*
 * Pointer value that eventually should point to a value of 0x1.
 * We cannot use integer arithmetic on pointers and since
 * I dont want to use "intptr_t", I use this instead with 
 * a value of "1" in it, to represent an end node.
 */
static char *pointer_to_one;

static inline boolean
sll_end_node (sll_node_t *sln)
{
    return
        (NULL == sln) || 
        (NULL == sln->next) ||
        ((sll_node_t*) pointer_to_one == sln->next);
}

static inline boolean
not_sll_end_node (sll_node_t *sln)
{
    return
        sln && (sln->next != (sll_node_t*) pointer_to_one);
}

PUBLIC error_t
sll_object_init (sll_object_t *sll,
	boolean make_it_thread_safe,
	comparison_function_t cmpf,
	mem_monitor_t *parent_mem_monitor)
{
    sll_node_t *node;
    error_t rv = 0;

    /* make the pointer value 0x1 */
    pointer_to_one = NULL;
    pointer_to_one++;
        
    CREATE_AND_WRITE_LOCK(sll);
    MEM_MONITOR_SETUP(sll);

    /* create the permanent "end" node */
    node = (sll_node_t*) MEM_ALLOC(sll, sizeof(sll_node_t));
    if (node) {
        node->next = (sll_node_t*) pointer_to_one;
        node->user_datum.pointer = NULL;
        sll->head = node;
        sll->n = 0;
        sll->cmpf = cmpf;
    } else {
        rv = ENOMEM;
    }
    WRITE_UNLOCK_CONDITIONAL(sll);

    return rv;
}

static inline sll_node_t *
new_sll_node (sll_object_t *sll, datum_t user_datum)
{
    sll_node_t *node = MEM_ALLOC(sll, sizeof(sll_node_t));

    if (node) {
	node->next = NULL;
	node->user_datum = user_datum;
    }
    return node;
}

/*
 * always adds to head, it is soo much faster to do that
 */
static error_t
thread_unsafe_sll_object_add (sll_object_t *sll,
	datum_t user_datum)
{
    sll_node_t *node = new_sll_node(sll, user_datum);

    if (node) {
	node->next = sll->head;
	sll->head = node;
        sll->n++;
	return 0;
    }

    /*
     * only reason this could have failed is if 
     * 'new_sll_node' failed due to malloc failure
     */
    return ENOMEM;
}

static error_t
thread_unsafe_sll_object_search (sll_object_t *sll,
	datum_t searched_datum, 
	datum_t *datum_found,
        sll_node_t **node_found)
{
    sll_node_t *temp = sll->head;

    while (not_sll_end_node(temp)) {

	if (sll->cmpf(temp->user_datum, searched_datum) == 0) {
	    *datum_found = temp->user_datum;
            *node_found = temp;
	    return 0;
	}

        /*
         * We CAN use the pointer here directly since we know this
         * will not be the end node, since we checked it above
         */
        temp = temp->next;
    }

    *node_found = NULL;
    return ENODATA;
}

static error_t
thread_unsafe_sll_object_add_once (sll_object_t *sll,
	datum_t user_datum)
{
    datum_t found_datum;
    sll_node_t *found_node;

    if (thread_unsafe_sll_object_search(sll, user_datum, 
            &found_datum, &found_node) == 0) {
                /* datum is already in the list */
                return 0;
    }
    return
	thread_unsafe_sll_object_add(sll, user_datum);
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
static error_t
thread_unsafe_sll_node_delete (sll_object_t *sll,
        sll_node_t *node_tobe_deleted)
{
    void *to_free;

    /* should not delete end node, NEVER */
    if (sll_end_node(node_tobe_deleted))
        return EINVAL;

    to_free = node_tobe_deleted->next;
    *node_tobe_deleted = *(node_tobe_deleted->next);
    MEM_FREE(sll, to_free);
    sll->n--;

    return 0;
}

static error_t
thread_unsafe_sll_object_delete (sll_object_t *sll,
	datum_t to_be_deleted, 
	datum_t *actual_data_deleted)
{
    sll_node_t *node_tobe_deleted;

    if (thread_unsafe_sll_object_search(sll, to_be_deleted, 
            actual_data_deleted, &node_tobe_deleted) == 0) {
                return
                    thread_unsafe_sll_node_delete(sll, node_tobe_deleted);
    }
    return ENODATA;
}

/******************************************************************************/

PUBLIC error_t
sll_object_add (sll_object_t *sll, datum_t user_datum)
{
    error_t rv;

    WRITE_LOCK_CONDITIONAL(sll, NULL);
    rv = thread_unsafe_sll_object_add(sll, user_datum);
    WRITE_UNLOCK_CONDITIONAL(sll);
    return rv;
}

PUBLIC error_t
sll_object_add_once (sll_object_t *sll, datum_t user_datum)
{
    error_t rv;

    WRITE_LOCK_CONDITIONAL(sll, NULL);
    rv = thread_unsafe_sll_object_add_once(sll, user_datum);
    WRITE_UNLOCK_CONDITIONAL(sll);
    return rv;
}

PUBLIC error_t
sll_object_search (sll_object_t *sll,
	datum_t searched_datum,
	datum_t *datum_found)
{
    error_t rv;
    sll_node_t *node_found;

    WRITE_LOCK_CONDITIONAL(sll, NULL);
    rv = thread_unsafe_sll_object_search(sll, searched_datum, 
            datum_found, &node_found);
    WRITE_UNLOCK_CONDITIONAL(sll);
    return rv;
}

PUBLIC error_t
sll_object_delete (sll_object_t *sll,
	datum_t to_be_deleted,
	datum_t *actual_data_deleted)
{
    error_t rv;

    WRITE_LOCK_CONDITIONAL(sll, NULL);
    rv = thread_unsafe_sll_object_delete(sll, to_be_deleted, actual_data_deleted);
    WRITE_UNLOCK_CONDITIONAL(sll);
    return rv;
}

PUBLIC error_t
sll_object_iterate (sll_object_t *sll, traverse_function_t tfn,
        datum_t p0, datum_t p1, datum_t p2, datum_t p3)
{
    error_t rv;
    sll_node_t *iter = sll->head;

    while (not_sll_end_node(iter)) {
        rv = tfn(sll, iter, iter->user_datum, p0, p1, p2, p3);
        if (rv) return rv;
        iter = iter->next;
    }
    return 0;
}

PUBLIC void
sll_object_destroy (sll_object_t *sll)
{
    sll_node_t *next, *iter;

    WRITE_LOCK_CONDITIONAL(sll, NULL);
    iter = sll->head;
    while (not_sll_end_node(iter)) {
        next = iter->next;
        MEM_FREE(sll, iter);
        iter = next;
    }
    MEM_FREE(sll, sll->head);
    sll->n = 0;
    sll->head = NULL;
    sll->mem_mon_p = NULL;
    sll->cmpf = NULL;
    WRITE_UNLOCK_CONDITIONAL(sll);
    LOCK_OBJ_DESTROY(sll);
}





