
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
** Singly linked list object, with very fast deletion provided you
** know the node that you want to delete.  You do NOT have to know
** the 'previous' node.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

#include "mem_monitor.h"
#include "lock_object.h"

typedef int (*comparison_function_t) (void *v1, void *v2);
typedef struct linkedlist_s linkedlist_t;
typedef struct linkedlist_node_s linkedlist_node_t;

/*
 * Users should be aware that the list is terminated with an
 * ACTUAL node whose next pointer AND the data pointer is NULL.
 * An 'end of list' marker node is needed to implement fast
 * deletion from the list.
 *
 * Therefore, when iterating thru a list, take into account
 * the fact that the LAST node is ONLY a marker and NOT
 * real data.
 */
struct linkedlist_node_s {

    /* what list I belong to */
    linkedlist_t *list;

    /* next node */
    linkedlist_node_t *next;

    /* user opaque data */
    void *user_data;
};

/*
 * singly linked list object.
 * comparison function is used to test ranking/ordering of list elements.
 */
struct linkedlist_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    /* start of list */
    linkedlist_node_t *head;

    /* comparison function used *ONLY* for equality */
    comparison_function_t cmp_fn;

    /* number of elements in the list */
    int n;

};

/*
 * We maintain an "end node" always in the list.  This is represented
 * by setting its 'next' AND data pointers to the value of NULL.
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

static inline int
endof_linkedlist (linkedlist_node_t *llnp)
{
    return
        (NULL == llnp) || 
        ((NULL == llnp->next) && (NULL == llnp->user_data));
}

static inline int
not_endof_linkedlist (linkedlist_node_t *llnp)
{
    return
        llnp && (NULL != llnp->next) && (NULL != llnp->user_data);
}

/*
 * initializes the linked list object
 */
extern int
linkedlist_init (linkedlist_t *listp,
        int make_it_thread_safe,
        comparison_function_t cmp_fn,
        mem_monitor_t *parent_mem_monitor);

/*
 * adds a node containing the specified data to the list.
 * ALWAYS added, regardless of whether the data is already 
 * in the list or not.  So, multiple copies WILL be added
 * if not controlled.  Addition is is done at a point in
 * the list determined by the ordering specified in the
 * comparison function.
 */
extern int
linkedlist_add (linkedlist_t *listp, void *user_data);

/*
 * same as adding data but the data is added only once.
 * If it is already there, it will not be added again and
 * 'data_found' will be updated with what is already
 * found in the list.  If data was not already in the list,
 * 'data_found' will be set to NULL.  This mechanism
 * always gives the user the knowledge of whether the
 * data being inserted was in the list in the first place.
 * An error (non zero) is returned only when memory
 * allocation fails.  It is NOT considered an error if
 * the data already exists in the list.
 */
extern int
linkedlist_add_once (linkedlist_t *listp, void *user_data, 
        void **data_found);

/*
 * Just blindly adds to the head of the list regardless
 * of any ordering.  This can be used in lists where the
 * ordering may not matter.  But once the list is added
 * to this way, it will no longer be ordered and searches
 * will fail unless user specifically writes his own search
 * function.
 */
extern int
linkedlist_add_to_head (linkedlist_t *listp, void *user_data);

/*
 * searches the first occurence of the matching data.
 * This is where the compare function is applied to find
 * out whether a match has occured.
 * Function return value will be 0 if found and 'data_found'
 * will be set to the found data.  If not found,
 * non 0 will be returned and 'data_found' will be set 
 * to NULL.
 */
extern int
linkedlist_search (linkedlist_t *listp, void *searched_data, 
        void **data_found);

/*
 * Removes the entry in the list matching user data specified
 * by 'to_be_deleted'.  If data was in the list, it will be
 * removed, 0 will be returned and 'data_deleted' will be set to
 * what was found in the list.  Otherwise, non 0 will be returned
 * and 'data_deleted' will be set to NULL.
 */
extern int 
linkedlist_delete (linkedlist_t *listp, void *to_be_deleted,
        void **data_deleted);

/*
 * Frees up the actual linked list node in the list.  0 will be
 * returned if succeeded.  It will fail if 'node_to_be_deleted'
 * does not belong to the list or it happens to be the list
 * end marker node.
 */
extern int
linkedlist_delete_node (linkedlist_t *listp, 
        linkedlist_node_t *node_to_be_deleted);

/*
 * frees up all the nodes of the list and cleans it out.
 * The list must be re-initialized properly if it needs to be
 * re-used.  Note that only the list elements are removed.
 * The actual user data pointers stored in those nodes are
 * not touched in any way.
 */
extern void
linkedlist_destroy (linkedlist_t *listp);

/*
 * Some convenient macros to iterate thru the list one node at a time.
 */

#define FOR_ALL_LINKEDLIST_ELEMENTS(listp, objp) \
        for (linkedlist_node_t *__n__ = (listp)->head; \
             not_endof_linkedlist(__n__) && \
             (objp = (__typeof__(objp))(__n__->user_data)); \
             __n__ = __n__->next)

/*
 * Used for self depleting lists, ie at every iteration, the
 * head element is EXPECTED to be removed by the iteration body.
 * If that is not done, infinite loops WILL result.
 */
#define WHILE_LINKEDLIST_NOT_EMPTY(listp, objp) \
        for (linkedlist_node_t *__n__ = (listp)->head; \
             not_endof_linkedlist(__n__) && \
             (objp = (__typeof__(objp))(__n__->user_data)); \
             __n__ = listp->head)

#ifdef __cplusplus
} // extern C
#endif 

#endif // __LINKEDLIST_H__






