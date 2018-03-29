
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
** @@@@@ DYNAMIC ARRAY OBJECT
**
** This is an array object which can be accessed by ANY index.  As user
** inserts data into various locations in the array, the array adjusts
** to the specified indexes and enlarges itself to accommodate the entire
** span of indexes.  The only limit is the memory limit imposed by malloc.
**
** For example, if user inserts a data at index 5000, and then inserts
** another data at index -234, the array adjusts itself to expand to
** 5234 entries automatically.  Obviously if the index span gets larger
** and larger, more & more memory will be used.
**
** This data structure is intended to be used with a smallish set of
** integers which can span the indexes.  If for example a user wants
** to store within indexes bound between 1500 - 1650, this would be a
** perfect structure and it would only use 150 entries.
**
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __DYNAMIC_ARRAY_OBJECT_H__
#define __DYNAMIC_ARRAY_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <assert.h>

#include "mem_monitor_object.h"
#include "lock_object.h"

typedef struct dynamic_array_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    int size;
    int n;
    int lowest, highest;
    unsigned long long int inserts, deletes;
    void **elements;

} dynamic_array_t;

extern int
dynamic_array_init (dynamic_array_t *datp,
	int make_it_thread_safe,
	int initial_size,
        mem_monitor_t *parent_mem_monitor);

extern int 
dynamic_array_insert (dynamic_array_t *datp,
	int index, 
	void *value);

/*
 * if an entry is requested outside the array bounds, it is an error.
 * Similarly, if an entry is un-set, it is also considered an error
 * even if it resides within the valid array boundaries
 */
extern int
dynamic_array_get (dynamic_array_t *datp,
	int index, 
	void **returned_value);

/*
 * cannot remove an entry which does not lie within boundaries
 * or is an unused slot
 */
extern int
dynamic_array_remove (dynamic_array_t *datp,
	int index,
	void **removed);

extern void **
dynamic_array_get_all (dynamic_array_t *datp, int *count);

extern void
dynamic_array_destroy (dynamic_array_t *datp);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __DYNAMIC_ARRAY_OBJECT_H__

