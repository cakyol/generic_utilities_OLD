
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

#ifndef __SLL_OBJECT_H__
#define __SLL_OBJECT_H__

#include "utils_common.h"
#include "lock_object.h"

/*
 * Users should be aware that the list is terminated with an
 * ACTUAL node whose next pointer AND the datum pointer is NULL.
 * An 'end of list' marker node is needed to implement fast
 * deletion from the list.
 *
 * Therefore, when iterating thru a list, take into account
 * the fact that the LAST node is ONLY a marker and NOT
 * real data.
 */
typedef struct sll_node_s sll_node_t;
struct sll_node_s {

    sll_node_t *next;
    datum_t user_datum;
};

/*
 * singly linked list object.
 * comparison function is used while deleting data.
 * The data in the node is compared against the data 
 * specified in the delete function.  If they match,
 * node is taken off the list and freed up.
 */
typedef struct sll_object_s {

    LOCK_VARIABLES;
    MEM_MON_VARIABLES;

    sll_node_t *head;
    comparison_function_t cmpf;
    int n;

} sll_object_t;

/*
 * We maintain an "end node" always in the list.  This is represented
 * by setting its 'next' AND datum pointers to the value of NULL.
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

static inline boolean
sll_end_node (sll_node_t *sln)
{
    return
        (NULL == sln) || 
        ((NULL == sln->next) && (NULL == sln->user_datum.pointer));
}

static inline boolean
not_sll_end_node (sll_node_t *sln)
{
    return
        sln && (NULL != sln->next) && (NULL != sln->user_datum.pointer);
}

/*
 * initializes the linked list object
 */
extern error_t
sll_object_init (sll_object_t *sll,
	boolean make_it_thread_safe,
	comparison_function_t cmpf,
	mem_monitor_t *parent_mem_monitor);

/*
 * adds a node containing the specified datum to the list.
 * Always added, ragardless of whether the datum is already 
 * in the list or not.  So, multiple copies WILL be added
 * if not controlled.  Addition is always done to the head
 * since it is so much faster to do so.
 */
extern error_t
sll_object_add (sll_object_t *sll,
	datum_t user_datum);

/*
 * integer & pointer add versions of the above function
 */
extern error_t
sll_object_add_integer (sll_object_t *sll, int integer);

extern error_t
sll_object_add_pointer (sll_object_t *sll, void *pointer);

/*
 * same as adding datum but the datum is added only once.
 * If it is already there, it will not be added again but
 * this will not be treated as an error.  Datum will be
 * checked as being available or not by applying the user
 * supplied comparison function at initialization of the
 * list object.
 */
extern error_t
sll_object_add_once (sll_object_t *sll,
	datum_t user_datum);

/*
 * integer & pointer add versions of the above function
 */
extern error_t
sll_object_add_once_integer (sll_object_t *sll, int integer);

extern error_t
sll_object_add_once_pointer (sll_object_t *sll, void *pointer);

/*
 * searches the first occurence of the matching datum.
 * This is where the compare function is applied to find
 * out whether a match has occured.
 * Function return value will be 0 if found and 'datum_found'
 * will be set to the found datum.  If not found,
 * ENODATA will be returned and 'datum_found' will be set 
 * to NULL.
 */
extern error_t
sll_object_search (sll_object_t *sll,
	datum_t searched_datum, datum_t *datum_found);

/*
 * integer & pointer add versions of the above function
 */
extern error_t
sll_object_search_integer (sll_object_t *sll,
        int searched_integer, int *found_integer);

extern error_t
sll_object_search_pointer (sll_object_t *sll,
        void *searched_pointer, void **found_pointer);

/*
 * deletes the node in which 'to_be_deleted' matches.
 * Returns 0 and the actual datum that was deleted if it
 * was found, else ENODATA and NULL otherwise.
 */
extern error_t 
sll_object_delete (sll_object_t *sll,
	datum_t to_be_deleted, datum_t *actual_data_deleted);

/*
 * integer & pointer add versions of the above function
 */
extern error_t
sll_object_delete_integer (sll_object_t *sll,
        int int_to_be_deleted, int *actual_int_deleted);

extern error_t
sll_object_delete_pointer (sll_object_t *sll,
        void *pointer_to_be_deleted, void **actual_pointer_deleted);

/*
 * Execute the function 'tfn' with appropriate parameters
 * on all elements of the list.  Ensure that tfn does NOT
 * change anything in the list itself.
 */
extern error_t
sll_object_iterate (sll_object_t *sll, traverse_function_t tfn,
        datum_t p0, datum_t p1, datum_t p2, datum_t p3);

extern void
sll_object_destroy (sll_object_t *sll);

#endif // __SLL_OBJECT_H__






