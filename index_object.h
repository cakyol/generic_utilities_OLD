
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

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"

typedef struct index_obj_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;
    boolean should_not_be_modified;
    object_comparer cmpf;
    int maximum_size;
    int expansion_size;
    int n;
    void **elements;

    /* used for traversing */
    int current;

    statistics_block_t stats;

} index_obj_t;

/****************************** Initialize ***********************************
 *
 * This does not need much explanation, except maybe just the 'expansion_size'
 * field.  When an index object is initialized, its size is set to accept
 * 'maximum_size' data pointers.  If more than that number of entries are
 * needed, then the object self expands by 'expansion_size'.  If this value
 * is specified as 0, then expansion will not be allowed and the insertion
 * will fail.
 *
 * Function return value is errno or 0.
 */
extern int 
index_obj_init (index_obj_t *idx,
        boolean make_it_thread_safe,
        object_comparer cmpf,
        int maximum_size,
        int expansion_size,
        mem_monitor_t *parent_mem_monitor);

/******************************** Insert *************************************
 *
 * Inserts 'data' into its appropriate place in the index.  If the data
 * was already in the index, also returns whatever was there in 'present_data'.
 * 'present_data' can be NULL if not needed.  If data was already there,
 * it will NOT be overwritten.
 *
 * Function return value is errno or 0.
 */
extern int 
index_obj_insert (index_obj_t *idx,
        void *data,
        void **present_data);

/******************************** Search **************************************
 *
 * Search the data specified by 'data'.  Whatever is found, will be returned in
 * 'found'.  This can be set to NULL if not needed.
 *
 * Function return value is errno or 0.
 */
extern int 
index_obj_search (index_obj_t *idx,
        void *data,
        void **found);

/********************************* Remove *************************************
 *
 * remove the data specified by 'data'.  What is removed is placed into
 * 'data_removed'.  This can be NULL if not needed.
 *
 * Function return value is errno or 0.
 */
extern int 
index_obj_remove (index_obj_t *idx,
        void *data,
        void **data_removed);

/**************************** Get all entries ********************************
 *
 * Get a snapshot of all the data pointers in the object.  The user
 * passes in the array which needs to be populated, and the size of the
 * array in *count.  Upon return count will contain how many items have
 * been written into the array.  If there is more items in the object
 * than what the array can accommodate, the function will stop populating
 * at the specified array size.
 */
extern void
index_obj_get_all (index_obj_t *idx,
        void *user_pointers [], int *count);

/******************************* Traverse ************************************
 *
 * Apply a user specified function 'tfn' to all the entries in the index
 * object.  The iteration will continue as long as 'tfn' returns a non zero
 * value (errno) or until all data is traversed.
 *
 * Function return value is the first non zero value 'tfn' returns.
 */
extern int
index_obj_traverse (index_obj_t *idx,
        traverse_function_pointer tfn,
        void *p0, void *p1, void *p2, void *p3);

/******************************* Trim size ************************************
 *
 * This function will trim the unused memory and return it back to
 * the system.  If the object expanded greatly and then shrunk
 * considerably ('n' is much less than 'maximum_size'), that means index is
 * holding on to a lot of unused memory.  User can make the decision
 * as to when to call this function.  With the current values, if the
 * object is holding on to more memory than 4 more available slots for 
 * user data, it will be trimmed.
 *
 * For example, if the object has grown considerably during the execution
 * to say a max size of 200, but it is holding only say 60 user data pointers,
 * since 200 > 60 + 4, its max size will be trimmed to 64.
 *
 * Function return value is errno or 0.
 */
extern int
index_obj_trim (index_obj_t *idx);

/********************************* Reset **************************************
 *
 * This call resets the object back to as if it was completely empty with
 * only about 4 entries available as default.
 */
extern void
index_obj_reset (index_obj_t *idx);

/******************************** Destroy *************************************
 *
 * This calls destroys this index object and makes it un-usable.
 * Note that it destroys only the references to the objects, NOT
 * the object itself, since that is the caller's responsibility.
 * If the user wants to perform any operation on the actual user data
 * itself, it passes in a function pointer, (destruction handling
 * function pointer), which will be called for every item removed
 * off the object.  The parameters passed to that function will be
 * the user data pointer, and an extra arg the user may wish to
 * provide ('extra_arg').
 *
 * Note that the destruction handler function must not in any way
 * change anything on the index object itself.
 */
extern void
index_obj_destroy (index_obj_t *idx,
        destruction_handler_t dh_fptr, void *extra_arg);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __INDEX_OBJECT_H__


