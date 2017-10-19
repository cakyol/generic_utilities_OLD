
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
 * Users should NEVER traverse thru the nodes by themselves but ONLY
 * thru the APIs provided.  This is becoz the 'next' pointer does NOT
 * point to the next node as implied.  Internal implementation has
 * some tricks which prohibits use of the 'next' pointer directly.
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

    LOCK_VARIABLES
    mem_monitor_t mem_mon, *mem_mon_p;
    sll_node_t *head;
    comparison_function_t cmpf;
    int n;

} sll_object_t;

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
 * same ass adding datum but the datum is added only once.
 * If it is already there, it will not be added again but
 * this will not be treated as an error.
 */
extern error_t
sll_object_add_once (sll_object_t *sll,
	datum_t user_datum);

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
 * deletes the node in which 'to_be_deleted' matches.
 * Returns 0 and the actual datum that was deleted if it
 * was found, else ENODATA and NULL otherwise.
 */
extern error_t 
sll_object_delete (sll_object_t *sll,
	datum_t to_be_deleted, datum_t *actual_data_deleted);

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






