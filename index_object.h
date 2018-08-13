
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

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

#include "mem_monitor_object.h"
#include "lock_object.h"
#include "function_types.h"

typedef struct index_obj_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;
    int cannot_be_modified;
    object_comparer cmpf;
    int initial_size;
    int maximum_size;
    int expansion_size;
    int expansion_count;
    int n;
    void **elements;

} index_obj_t;

/**************************** Initialize *************************************/

extern int 
index_obj_init (index_obj_t *idx,
        int make_it_thread_safe,
        object_comparer cmpf,
        int maximum_size,
        int expansion_size,
        mem_monitor_t *parent_mem_monitor);

/**************************** Insert *****************************************/

extern int 
index_obj_insert (index_obj_t *idx,
        void *data_to_be_inserted,
        void **data_already_present);

/**************************** Search *****************************************/

extern int 
index_obj_search (index_obj_t *idx,
        void *data_to_be_searched,
        void **data_found);

/**************************** Remove *****************************************/

extern int 
index_obj_remove (index_obj_t *idx,
        void *data_to_be_removed,
        void **data_actually_removed);

/**************************** Get all entries ********************************/

extern void **
index_obj_get_all (index_obj_t *idx, int *returned_count);

/**************************** Traverse ***************************************/

extern int
index_obj_traverse (index_obj_t *idx,
        traverse_function_pointer tfn,
        void *p0, void *p1, void *p2, void *p3);

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

#ifdef __cplusplus
} // extern C
#endif 

#endif // __INDEX_OBJECT_H__


