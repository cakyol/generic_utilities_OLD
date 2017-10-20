
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

#include "index_object.h"

/*
 * Handles overlapping copies
 */
static inline void
copy_index_elements (datum_t *src, datum_t *dst, int count)
{
    if (dst < src)
	while (count-- > 0) *dst++ = *src++;
    else {
	src += count;
	dst += count;
	while (count-- > 0) *(--dst) = *(--src);
    }
}

static error_t
index_resize (index_obj_t *idx, int new_size)
{
    datum_t *new_elements;

    new_elements = MEM_MONITOR_ALLOC(idx, new_size * sizeof(datum_t));
    if (NULL == new_elements) return ENOMEM;
    copy_index_elements(idx->elements, new_elements, idx->n);
    MEM_MONITOR_FREE(idx, idx->elements);
    idx->elements = new_elements;
    idx->maximum_size = new_size;
    return 0;
}

/*
** This is the heart of the object: binary search
*/
static int 
index_find_position (index_obj_t *idx,
	datum_t searched_data, 
	int *insertion_point)
{
    register int mid, diff, lo, hi;

    lo = mid = diff = 0;
    hi = idx->n - 1;

    /* binary search */
    while (lo <= hi) {
	mid = (hi+lo) >> 1;
	diff = (idx->cmpf)(searched_data, idx->elements[mid]);
	if (diff > 0) {
	    lo = mid + 1;
	} else if (diff < 0) {
	    hi = mid - 1;
	} else {
	    return mid;
	}
    }

    /*
    ** not found, but record where the element should be 
    ** inserted, in case it was required to be put into 
    ** the array.
    */
    SAFE_POINTER_SET(insertion_point, (diff > 0 ? (mid + 1) : mid));
    return -1;
}

static error_t
thread_unsafe_index_obj_insert (index_obj_t *idx,
	datum_t data,
        datum_t *exists)
{
    int insertion_point = 0;	/* shut the -Werror up */
    int size, i;
    datum_t *source;

    /*
    ** see if element is already there and if not,
    ** note the insertion point in "insertion_point".
    */
    i = index_find_position(idx, data, &insertion_point);

    /* key/data already in index */
    if (i >= 0) {
        SAFE_POINTER_SET(exists, idx->elements[i]);
        return 0;
    }

    /* 
    ** if we are here, key/data pair is NOT in the index
    ** and we are attempting to insert it for the first time
    */
    SAFE_NULLIFY_DATUMP(exists);

    /* if index is full, attempt to expand by specified expansion_size */
    if (idx->n >= idx->maximum_size) {

	/* cannot expand, not allowed */
	if (idx->expansion_size <= 0) {
	    return ENOSPC;
	}

	/* tried to expand but failed */
	if (FAILED(index_resize(idx, idx->maximum_size + idx->expansion_size))) {
	    return ENOMEM;
	}

	/* record the expansion */
	idx->expansion_count++;
    }

    /*
    ** shift all of the pointers after 
    ** "insertion_point" right by one 
    */
    source = &(idx->elements[insertion_point]);
    if ((size = idx->n - insertion_point) > 0)
	copy_index_elements(source, (source+1), size);
    
    /* fill in the new node values */
    idx->elements[insertion_point] = data;

    /* increment element count */
    idx->n++;

    return 0;
}

static error_t 
thread_unsafe_index_obj_search (index_obj_t *idx,
	datum_t search_key,
	datum_t *found)
{
    int i;

    i = index_find_position(idx, search_key, NULL);

    /* not found */
    if (i < 0) {
        SAFE_NULLIFY_DATUMP(found);
	return ENODATA;
    }

    SAFE_POINTER_SET(found, idx->elements[i]);
    return 0;
}

static error_t
thread_unsafe_index_obj_remove (index_obj_t *idx,
	datum_t data_to_be_removed,
	datum_t *actual_data_removed)
{
    int i, size;

    /* first see if it is there */
    i = index_find_position(idx, data_to_be_removed, NULL);

    /* not in table */
    if (i < 0) {
        SAFE_NULLIFY_DATUMP(actual_data_removed);
	return ENODATA;
    }

    SAFE_POINTER_SET(actual_data_removed, idx->elements[i]);
    idx->n--;

    /* pull the elements AFTER "index" to the left by one */
    if ((size = idx->n - i) > 0) {
	datum_t *source = &idx->elements[i+1];
	copy_index_elements(source, (source - 1), size);
    }

    return 0;
}

/***************************** public functions *****************************/

PUBLIC error_t
index_obj_init (index_obj_t *idx,
	boolean make_it_lockable,
	comparison_function_t cmpf,
	int maximum_size,
	int expansion_size,
        mem_monitor_t *parent_mem_monitor)
{
    error_t rv = 0;

    if ((maximum_size <= 1) || (expansion_size < 0)) {
	return EINVAL;
    }

    LOCK_SETUP(idx);
    idx->initial_size = idx->maximum_size = maximum_size;
    idx->expansion_size = expansion_size;
    idx->expansion_count = 0;
    idx->cmpf = cmpf;
    idx->n = 0;

    MEM_MONITOR_SETUP(idx);
    idx->elements = MEM_MONITOR_ALLOC(idx, sizeof(datum_t) * maximum_size);
    if (NULL == idx->elements) {
	rv = EINVAL;
    }
    WRITE_UNLOCK(idx);
    return rv;
}

PUBLIC error_t
index_obj_insert (index_obj_t *idx,
	datum_t data,
	datum_t *exists)
{
    error_t rv;

    WRITE_LOCK(idx, NULL);
    rv = thread_unsafe_index_obj_insert(idx, data, exists);
    WRITE_UNLOCK(idx);
    return rv;
}

PUBLIC error_t
index_obj_search (index_obj_t *idx,
	datum_t search_key,
	datum_t *found)
{
    error_t rv;

    READ_LOCK(idx);
    rv = thread_unsafe_index_obj_search(idx, search_key, found);
    READ_UNLOCK(idx);
    return rv;
}

PUBLIC error_t
index_obj_remove (index_obj_t *idx,
	datum_t data_to_be_removed,
	datum_t *actual_data_removed)
{
    error_t rv;
    
    WRITE_LOCK(idx, NULL);
    rv = thread_unsafe_index_obj_remove(idx,
		data_to_be_removed, actual_data_removed);
    WRITE_UNLOCK(idx);
    return rv;
}

PUBLIC error_t
index_obj_traverse (index_obj_t *idx, traverse_function_t tfn,
    datum_t p0, datum_t p1, datum_t p2, datum_t p3)
{
    int i;
    error_t rv = 0;

    READ_LOCK(idx);
    for (i = 0; i < idx->n; i++) {
	if ((tfn)((void*) idx, &(idx->elements[i]), idx->elements[i],
	    p0, p1, p2, p3) != ok) {
		rv = EFAULT;
		break;
	}
    }
    READ_UNLOCK(idx);
    return rv;
}

PUBLIC datum_t *
index_obj_get_all (index_obj_t *idx, int *returned_count)
{
    int i;
    datum_t *storage_area;

    READ_LOCK(idx);
    storage_area = MEM_MONITOR_ALLOC(idx, (idx->n + 1) * sizeof(datum_t));
    if (NULL == storage_area) {
	*returned_count = 0;
	READ_UNLOCK(idx);
	return NULL;
    }
    for (i = 0; i < idx->n; i++) storage_area[i] = idx->elements[i];
    *returned_count = i;
    READ_UNLOCK(idx);
    return storage_area;
}

PUBLIC uint64
index_obj_memory_usage (index_obj_t *idx, double *mega_bytes)
{
    uint64 size = sizeof(index_obj_t) + idx->mem_mon_p->bytes_used;

    SAFE_POINTER_SET(mega_bytes, ((double) size / (double) MEGA));
    return size;
}

PUBLIC int
index_obj_trim (index_obj_t *idx)
{
    int trimmed = 0;
    int old_size = idx->maximum_size;

    /* trim only if size is >= twice the number of elements */
    if (idx->maximum_size >= (2 * idx->n)) {
	if (index_resize(idx, (idx->n + 1)) == 0) {
	    trimmed = old_size - (idx->n + 1);
	}
    }
    return trimmed;
}

PUBLIC void
index_obj_destroy (index_obj_t *idx)
{
    if (idx->elements) {
	free(idx->elements);
    }
    LOCK_OBJ_DESTROY(idx);
    memset(idx, 0, sizeof(index_obj_t));
}


