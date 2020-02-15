
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

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

/*
 * Handles overlapping copies
 */
static inline void
copy_index_elements (void **src, void **dst, int count)
{
    if (dst < src)
        while (count-- > 0) *dst++ = *src++;
    else {
        src += count;
        dst += count;
        while (count-- > 0) *(--dst) = *(--src);
    }
}

static int
index_resize (index_obj_t *idx, int new_size)
{
    void **new_elements;

    new_elements = MEM_MONITOR_ALLOC(idx, new_size * sizeof(void*));
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
        void *searched_data, 
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
    *insertion_point = diff > 0 ? (mid + 1) : mid;
    return -1;
}

static int
thread_unsafe_index_obj_insert (index_obj_t *idx,
        void *data,
        void **present_data)
{
    int insertion_point = 0;    /* shut the -Werror up */
    int size, i;
    void **source;

    /* assume no entry */
    safe_pointer_set(present_data, NULL);

    /* being traversed, cannot be changed */
    if (idx->should_not_be_modified) {
        insertion_failed(idx);
        return EBUSY;
    }

    /*
    ** see if element is already there and if not,
    ** note the insertion point in "insertion_point".
    */
    i = index_find_position(idx, data, &insertion_point);

    /* key/data already in index */
    if (i >= 0) {
        safe_pointer_set(present_data, idx->elements[i]);
        insertion_duplicated(idx);
        return 0;
    }

    /* if index is full, attempt to expand by specified expansion_size */
    if (idx->n >= idx->maximum_size) {

        /* cannot expand, not allowed */
        if (idx->expansion_size <= 0) {
            insertion_failed(idx);
            return ENOSPC;
        }

        /* tried to expand but failed */
        if (index_resize(idx, idx->maximum_size + idx->expansion_size)) {
            insertion_failed(idx);
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

    insertion_succeeded(idx);

    return 0;
}

static int 
thread_unsafe_index_obj_search (index_obj_t *idx,
        void *search_key,
        void **found)
{
    int i, dummy;

    safe_pointer_set(found, NULL);

    i = index_find_position(idx, search_key, &dummy);

    /* not found */
    if (i < 0) {
        search_failed(idx);
        return ENODATA;
    }

    safe_pointer_set(found, idx->elements[i]);
    search_succeeded(idx);
    return 0;
}

static int
thread_unsafe_index_obj_remove (index_obj_t *idx,
        void *data_to_be_removed,
        void **actual_data_removed)
{
    int i, size, dummy;

    safe_pointer_set(actual_data_removed, NULL);

    if (idx->should_not_be_modified) return EBUSY;

    /* first see if it is there */
    i = index_find_position(idx, data_to_be_removed, &dummy);

    /* not in table */
    if (i < 0) {
        deletion_failed(idx);
        return ENODATA;
    }

    safe_pointer_set(actual_data_removed, idx->elements[i]);
    idx->n--;

    /* pull the elements AFTER "index" to the left by one */
    if ((size = idx->n - i) > 0) {
        void **source = &idx->elements[i+1];
        copy_index_elements(source, (source - 1), size);
    }

    deletion_succeeded(idx);

    return 0;
}

/**************************** Initialize *************************************/

PUBLIC int
index_obj_init (index_obj_t *idx,
        int make_it_thread_safe,
        object_comparer cmpf,
        int maximum_size,
        int expansion_size,
        mem_monitor_t *parent_mem_monitor)
{
    int failed = 0;

    if ((maximum_size <= 1) || (expansion_size < 0)) {
        return EINVAL;
    }

    MEM_MONITOR_SETUP(idx);
    LOCK_SETUP(idx);
    idx->should_not_be_modified = 0;
    idx->initial_size = idx->maximum_size = maximum_size;
    idx->expansion_size = expansion_size;
    idx->expansion_count = 0;
    idx->cmpf = cmpf;
    idx->n = 0;
    idx->elements = MEM_MONITOR_ALLOC(idx, sizeof(void*) * maximum_size);
    if (NULL == idx->elements) {
        failed = EINVAL;
    }
    WRITE_UNLOCK(idx);
    return failed;
}

/**************************** Insert *****************************************/

PUBLIC int
index_obj_insert (index_obj_t *idx,
        void *data_to_be_inserted,
        void **present_data)
{
    int failed;

    WRITE_LOCK(idx);
    failed = thread_unsafe_index_obj_insert(idx, 
            data_to_be_inserted, present_data);
    WRITE_UNLOCK(idx);
    return failed;
}

/**************************** Search *****************************************/

PUBLIC int
index_obj_search (index_obj_t *idx,
        void *data_to_be_searched,
        void **present_data)
{
    int failed;

    READ_LOCK(idx);
    failed = thread_unsafe_index_obj_search(idx, 
            data_to_be_searched, present_data);
    READ_UNLOCK(idx);
    return failed;
}

/**************************** Remove *****************************************/

PUBLIC int
index_obj_remove (index_obj_t *idx,
        void *data_to_be_removed,
        void **data_actually_removed)
{
    int failed;
    
    WRITE_LOCK(idx);
    failed = thread_unsafe_index_obj_remove(idx,
                data_to_be_removed, data_actually_removed);
    WRITE_UNLOCK(idx);
    return failed;
}

/**************************** Get all entries ********************************/

PUBLIC void **
index_obj_get_all (index_obj_t *idx, int *returned_count)
{
    int i;
    void **storage_area;

    READ_LOCK(idx);
    storage_area = MEM_MONITOR_ALLOC(idx, (idx->n + 1) * sizeof(void*));
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

/**************************** Traverse ***************************************/

PUBLIC int
index_obj_traverse (index_obj_t *idx,
        traverse_function_pointer tfn,
        void *p0, void *p1, void *p2, void *p3)
{
    int i;
    int failed = 0;

    if (idx->should_not_be_modified) return EBUSY;
    idx->should_not_be_modified = 1;

    READ_LOCK(idx);
    for (i = 0; i < idx->n; i++) {
        if ((tfn)((void*) idx, &(idx->elements[i]), idx->elements[i],
            p0, p1, p2, p3) != 0) {
                failed = EFAULT;
                break;
        }
    }
    READ_UNLOCK(idx);

    idx->should_not_be_modified = 0;

    return failed;
}

/**************************** Destroy ****************************************/

/*
 * This function first iterates thru all its elements and calls
 * the user specified data destruction callback function for each
 * non NULL entry in the index.  It then completely frees up the
 * storage associated with the index.
 */
PUBLIC void
index_obj_destroy (index_obj_t *idx,
        destruction_handler_t dh_fptr, void *extra_arg)
{
    int i;

    WRITE_LOCK(idx);
    if (idx->elements) {
        if (dh_fptr) {
            for (i = 0; i < idx->n; i++) dh_fptr(idx->elements[i], extra_arg);
        }
        MEM_MONITOR_FREE(idx, idx->elements);
    }
    WRITE_UNLOCK(idx);
    LOCK_OBJ_DESTROY(idx);
    memset(idx, 0, sizeof(index_obj_t));
}

/**************************** Other ******************************************/

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

#ifdef __cplusplus
} // extern C
#endif 




