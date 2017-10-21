
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

#include "dynamic_array.h"

static error_t
dynamic_array_expand (dynamic_array_t *datp, int index)
{
    int i, j, new_size;
    boolean reverse_copy;
    datum_t *new_elements;

    if (index < datp->lowest) {
        new_size = datp->highest - index + 1;
        datp->lowest = index;
        reverse_copy = true;
    } else if (index > datp->highest) {
        new_size = index - datp->lowest + 1;
        datp->highest = index;
        reverse_copy = false;
    } else {
        /* we should NEVER be here */
        assert(0);
    }

    new_elements = 
	(datum_t*) MEM_MONITOR_ALLOC(datp, new_size * sizeof(datum_t));
    if (NULL == new_elements)
	return ENOMEM;
    for (i = 0; i < new_size; i++) NULLIFY_DATUM(new_elements[i]);

    /* copy existing data over */
    if (reverse_copy) {
	for (i = datp->size-1, j = new_size-1; i >= 0; i--, j--)
	    new_elements[j] = datp->elements[i];
    } else {
	for (i = 0; i < datp->size; i++)
	    new_elements[i] = datp->elements[i];
    }
    
    /* adjust the rest */
    datp->size = new_size;
    MEM_MONITOR_FREE(datp, datp->elements);
    datp->elements = new_elements;

    return 0;
}

/*
 * Shrink array so that it is limited by the lowest & highest index boundaries
 * Basically, this operation trims all unused entries from both ends of the
 * dynamic array and frees up that memory shrinking the array.
 */
static void
dynamic_array_shrink (dynamic_array_t *datp)
{
    datum_t *new_elements;
    int i, j, start_unused_entries, end_unused_entries;
    int new_size;

    /* see how many unused consecutive entries at the beginning */
    start_unused_entries = 0;
    for (i = 0; i < datp->size; i++) {
        if (DATUM_IS_NULL(datp->elements[i]))
            start_unused_entries++;
        else
            break;
    }

    /* now find how many unused consecutive entries at the end */
    end_unused_entries = 0;
    for (i = datp->size - 1; i >= 0; i--) {
	if (DATUM_IS_NULL(datp->elements[i]))
            end_unused_entries++;
        else
            break;
    }

    /* is it worth it ? */
    if ((start_unused_entries + end_unused_entries) < (datp->size / 2))
        return;

    /* 
     * if we are here, it has been determined that 
     * it is worth freeing up some memory, so do it now
     */

    /* allocate the new smaller array */
    new_size = datp->size - start_unused_entries - end_unused_entries;
    new_elements = (datum_t*)
	MEM_MONITOR_ALLOC(datp, new_size * sizeof(datum_t));
    if (NULL == new_elements) return;

    /* copy the valid data from the old into the new array */
    for (i = start_unused_entries, j = 0; 
         i < datp->size - end_unused_entries; 
         i++, j++) {
	    new_elements[j] = datp->elements[i];
    }

    /* free the old storage, attach new smaller one */
    datp->size = new_size;
    MEM_MONITOR_FREE(datp, datp->elements);
    datp->elements = new_elements;

    /* adjust all new boundaries now */
    datp->lowest += start_unused_entries;
    datp->highest -= end_unused_entries;
}

/*
 * Note that this is an intensive operation so be careful how often
 * the shrinking attempt should be invoked.  It may affect performance
 * considerably.
 */
static inline void
dynamic_array_attempt_to_shrink_memory (dynamic_array_t *datp)
{
    /*
     * If the array size has grown greater than 1k *AND*
     * if there has been as many deletions as at least half of insertions,
     * there may be unused memory which may be returned to the system.
     */
    if ((datp->size > 1024) && (datp->deletes > datp->inserts/2)) {
        dynamic_array_shrink(datp);
        datp->inserts -= datp->deletes;
        datp->deletes = 0;
    }
}

static inline boolean
valid_index (dynamic_array_t *datp, int index)
{
    return
        (index >= datp->lowest) &&
        (index <= datp->highest);
}

/***********************************************************************/

PUBLIC error_t
dynamic_array_init (dynamic_array_t *datp,
	boolean make_it_lockable,
	int initial_size,
        mem_monitor_t *parent_mem_monitor)
{
    int i;

    /*
     * array must be able to have a middle, left & right, 
     * so it requires a minimum of 3 elements
     */
    if (initial_size < 3) {
	return EINVAL;
    }
    LOCK_SETUP(datp);
    MEM_MONITOR_SETUP(datp);
    datp->elements = 
	MEM_MONITOR_ALLOC(datp, (initial_size * sizeof(datum_t*)));
    if (NULL == datp->elements)
	return ENOMEM;
    for (i = 0; i < initial_size; i++) NULLIFY_DATUM(datp->elements[i]);
    datp->size = initial_size;
    datp->n = 0;
    datp->inserts = datp->deletes = 0;
    WRITE_UNLOCK(datp);

    return 0;
}

static error_t 
thread_unsafe_dynamic_array_insert (dynamic_array_t *datp,
	int index,
	datum_t data)
{
    /* the first entry sets midpoint and all boundaries */
    if (datp->n <= 0) {
        datp->lowest = index - datp->size/2;
        datp->highest = datp->lowest + datp->size - 1;
	datp->elements[index - datp->lowest] = data;
	datp->n = 1;
        datp->inserts++;
	return 0;
    }

    /* if the index falls inside the boundaries, put it there */
    if (valid_index(datp, index)) {
	datp->elements[index - datp->lowest] = data;
	datp->n++;
        datp->inserts++;
	return 0;
    }

    /* if we are here, we must be out of bounds, so expand */
    if (FAILED(dynamic_array_expand(datp, index))) {
	return ENOMEM;
    }

    // this should succeed coz now everything is re-adjusted
    return
	thread_unsafe_dynamic_array_insert(datp, index, data);
}

PUBLIC error_t
dynamic_array_insert (dynamic_array_t *datp,
	int index,
	datum_t data)
{
    error_t rv;

    WRITE_LOCK(datp, NULL);
    rv = thread_unsafe_dynamic_array_insert(datp, index, data);
    WRITE_UNLOCK(datp);
    return rv;
}

/*
 * if an entry is requested outside the array bounds, it is an error.
 */
static error_t
thread_unsafe_dynamic_array_get (dynamic_array_t *datp,
	int index, 
	datum_t *returned)
{
    datum_t found;

    SAFE_NULLIFY_DATUMP(returned);
    if (valid_index(datp, index)) {
	found = datp->elements[index - datp->lowest];
	if (DATUM_IS_NULL(found)) {
            return ENODATA;
        }
        SAFE_DATUMP_SET(returned, found);
        return 0;
    }
    return ENODATA;
}

PUBLIC error_t
dynamic_array_get (dynamic_array_t *datp,
	int index,
	datum_t *returned)
{
    error_t rv;

    READ_LOCK(datp);
    rv = thread_unsafe_dynamic_array_get(datp, index, returned);
    READ_UNLOCK(datp);
    return rv;
}

/*
 * cannot remove an entry which does not lie within boundaries
 */
static error_t
thread_unsafe_dynamic_array_remove (dynamic_array_t *datp,
	int index,
	datum_t *removed)
{
    datum_t found;

    SAFE_NULLIFY_DATUMP(removed);
    if (valid_index(datp, index)) {
	found = datp->elements[index - datp->lowest];
	if (DATUM_IS_NULL(found)) {
	    return ENODATA;
	}
	SAFE_DATUMP_SET(removed, found);
	NULLIFY_DATUM(datp->elements[index - datp->lowest]);
	datp->n--;
	datp->deletes++;
	dynamic_array_attempt_to_shrink_memory(datp);
	return 0;
    }
    return EINVAL;
}

PUBLIC error_t
dynamic_array_remove (dynamic_array_t *datp,
	int index,
	datum_t *removed)
{
    error_t rv;

    WRITE_LOCK(datp, NULL);
    rv = thread_unsafe_dynamic_array_remove(datp, index, removed);
    WRITE_UNLOCK(datp);
    return rv;
}

PUBLIC datum_t *
dynamic_array_get_all (dynamic_array_t *datp, int *count)
{
    int i, valid;
    datum_t *assembled;

    *count = 0;
    assembled = (datum_t*) malloc(datp->n * sizeof(datum_t));
    if (NULL == assembled) return NULL;

    for (i = 0, valid = 0; i < datp->size; i++) {
	if (DATUM_NOT_NULL(datp->elements[i])) {
	    assembled[valid++] = datp->elements[i];
	    if (valid >= datp->n) break;
	}
    }
    *count = valid;
    return assembled;
}

PUBLIC void 
dynamic_array_destroy (dynamic_array_t *datp)
{
    if (datp->elements) {
	MEM_MONITOR_FREE(datp, datp->elements);
    }
    LOCK_OBJ_DESTROY(datp);
    memset(datp, 0, sizeof(dynamic_array_t));
}



