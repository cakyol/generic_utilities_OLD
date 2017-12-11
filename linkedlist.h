
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

#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include "lock_object.h"
#include "mem_monitor.h"
#include "function_types.h"

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
typedef struct linkedlist_node_s linkedlist_node_t;
struct linkedlist_node_s {

    linkedlist_node_t *next;
    void *user_data;
};

/*
 * singly linked list object.
 * comparison function is used while deleting data.
 * The data in the node is compared against the data 
 * specified in the delete function.  If they match,
 * node is taken off the list and freed up.
 */
typedef struct linkedlist_s {

    LOCK_VARIABLES;
    MEM_MON_VARIABLES;

    linkedlist_node_t *head;
    comparison_function_t cmpf;
    int n;

} linkedlist_t;

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
endof_linkedlist (linkedlist_node_t *sln)
{
    return
        (NULL == sln) || 
        ((NULL == sln->next) && (NULL == sln->user_data));
}

static inline int
not_endof_linkedlist (linkedlist_node_t *sln)
{
    return
        sln && (NULL != sln->next) && (NULL != sln->user_data);
}

/*
 * initializes the linked list object
 */
extern int
linkedlist_init (linkedlist_t *listp,
	int make_it_thread_safe,
	comparison_function_t cmpf,
	mem_monitor_t *parent_mem_monitor);

/*
 * adds a node containing the specified data to the list.
 * Always added, ragardless of whether the data is already 
 * in the list or not.  So, multiple copies WILL be added
 * if not controlled.  Addition is always done to the head
 * since it is so much faster to do so.
 */
extern int
linkedlist_add (linkedlist_t *listp,
	void *user_data);

/*
 * same as adding data but the data is added only once.
 * If it is already there, it will not be added again but
 * this will not be treated as an error.  Datum will be
 * checked as being available or not by applying the user
 * supplied comparison function at initialization of the
 * list object.
 */
extern int
linkedlist_add_once (linkedlist_t *listp,
	void *user_data);

/*
 * searches the first occurence of the matching data.
 * This is where the compare function is applied to find
 * out whether a match has occured.
 * Function return value will be 0 if found and 'data_found'
 * will be set to the found data.  If not found,
 * ENODATA will be returned and 'data_found' will be set 
 * to NULL.
 */
extern int
linkedlist_search (linkedlist_t *listp,
	void *searched_data, void **data_found);

/*
 * deletes the node in which 'to_be_deleted' matches.
 * Returns 0 and the actual data that was deleted if it
 * was found, else ENODATA and NULL otherwise.
 */
extern int 
linkedlist_delete (linkedlist_t *listp,
	void *to_be_deleted, void **actual_data_deleted);

/*
 * Execute the function 'tfn' with appropriate parameters
 * on all elements of the list.  Ensure that tfn does NOT
 * change anything in the list itself.
 */
extern int
linkedlist_iterate (linkedlist_t *listp, traverse_function_t tfn,
        void *p0, void *p1, void *p2, void *p3);

extern void
linkedlist_destroy (linkedlist_t *listp);

/*
 * Some convenient macros to iterate thru the list one node at a time.
 */

#define FOR_ALL_LINKEDLIST_ELEMENTS(list, obj_ptr) \
        for (linkedlist_node_t *__n__ = list->head; \
             not_linkedlist_end_node(__n__) && \
             (obj_ptr = (__typeof__(obj_ptr))(__n__->user_data)); \
             __n__ = __n__->next)

/*
 * Used for self depleting lists, ie at every iteration, the
 * head element is EXPECTED to be removed by the iteration body.
 * If that is not done, infinite loops may result.
 */
#define WHILE_LINKEDLIST_NOT_EMPTY(list, obj_ptr) \
        for (linkedlist_node_t *__n__ = list->head; \
             not_linkedlist_end_node(__n__) && \
             (obj_ptr = (__typeof__(obj_ptr))(__n__->user_data)); \
             __n__ = list->head)

#ifdef __cplusplus
} // extern C
#endif 

#endif // __LINKEDLIST_H__






