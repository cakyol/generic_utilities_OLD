
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol (gee.akyol@gmail.com, gee_akyol@yahoo.com)
** Copyright: Cihangir Metin Akyol, March 2016
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

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** @@@@@ INDEX OBJECT
**
** Generic insert/search/delete index.  Binary search based
** and hence extremely fast search.  However, slower in
** insertion & deletions.  But uses very little memory, only
** the size of a pointer per each element in the object.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __INDEX_OBJECT_H__
#define __INDEX_OBJECT_H__

#include "utils_common.h"
#include "lock_object.h"

typedef struct index_obj_s {

    LOCK_VARIABLES
    mem_monitor_t mem_mon, *mem_mon_p;
    comparison_function_t cmpf;
    int initial_size;
    int maximum_size;
    int expansion_size;
    int expansion_count;
    int n;
    datum_t *elements;

} index_obj_t;

/**************************** Initialize *************************************/

extern error_t 
index_obj_init (index_obj_t *idx,
	boolean make_it_thread_safe,
	comparison_function_t cmpf,
	int maximum_size,
	int expansion_size,
        mem_monitor_t *parent_mem_monitor);

/**************************** Insert *****************************************/

extern error_t 
index_obj_insert (index_obj_t *idx,
	datum_t datum_to_be_inserted,
	datum_t *datum_already_present);

extern error_t
index_obj_insert_integer (index_obj_t *idx,
        int integer_to_be_inserted, 
        int *integer_already_present);

extern error_t
index_obj_insert_pointer (index_obj_t *idx,
        void *pointer_to_be_inserted, 
        void **pointer_already_present);

/**************************** Search *****************************************/

extern error_t 
index_obj_search (index_obj_t *idx,
	datum_t datum_to_be_searched,
	datum_t *datum_found);

extern error_t
index_obj_search_integer (index_obj_t *idx,
        int integer_to_be_searched,
        int *integer_found);

extern error_t
index_obj_search_pointer (index_obj_t *idx,
        void *pointer_to_be_searched,
        void **pointer_found);

/**************************** Remove *****************************************/

extern error_t 
index_obj_remove (index_obj_t *idx,
	datum_t datum_to_be_removed,
	datum_t *datum_actually_removed);

extern error_t
index_obj_remove_integer (index_obj_t *idx,
        int integer_to_be_removed,
        int *integer_actually_removed);

extern error_t
index_obj_remove_pointer (index_obj_t *idx,
        void *pointer_to_be_removed,
        void **pointer_actually_removed);

/**************************** Get all entries ********************************/

extern datum_t *
index_obj_get_all (index_obj_t *idx, int *returned_count);

extern int *
index_obj_get_all_integers (index_obj_t *idx, int *returned_count);

extern void **
index_obj_get_all_pointers (index_obj_t *idx, int *returned_count);

/**************************** Traverse ***************************************/

extern error_t
index_obj_traverse (index_obj_t *idx,
	traverse_function_t tfn,
	datum_t p0, datum_t p1, datum_t p2, datum_t p3);

/**************************** Other ******************************************/

/*
 * This function will trim the unused memory and return it back to
 * the system.  If the object expanded greatly and then shrunk
 * considerably ('n' is much less than 'maximum_size'), that means index is
 * holding on to a lot of unused memory.  User can make the decision
 * as to when to call this function.  Typically if 'n' is less than
 * half the size of 'maximum_size', it may be a good time to call this function.
 */
extern int
index_obj_trim (index_obj_t *idx);

/**************************** Destroy ****************************************/

extern void
index_obj_destroy (index_obj_t *idx);

#endif // __INDEX_OBJECT_H__


