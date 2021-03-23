
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

#include "object_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

debug_module_block_t om_debug = { 

    .level = ERROR_DEBUG_LEVEL,
    .module_name = "OBJECT_MANAGER",
    .drf = NULL

};

/******************************************************************************
 *
 * general support functions
 *
 */

static int
compare_objects (void *o1, void *o2)
{
    int res;

    res = ((object_t*) o1)->object_type - ((object_t*) o2)->object_type;
    if (res) return res;
    return 
        ((object_t*) o1)->object_instance - ((object_t*) o2)->object_instance;
}

static int
compare_attributes (void *aip1, void *aip2)
{
    return
        ((attribute_t*) aip1)->attribute_id -
        ((attribute_t*) aip2)->attribute_id;
}

static inline object_t*
get_object_pointer (object_manager_t *omp,
        int object_type, int object_instance)
{
    object_t searched;
    void *found;

    searched.object_type = object_type;
    searched.object_instance = object_instance;
    if (0 == avl_tree_search(&omp->om_objects, &searched, &found)) {
        return found;
    }
    return NULL;
}

static inline object_t*
get_parent_pointer (object_t *obj)
{
    if (obj->parent.is_pointer) {
        return
            obj->parent.u.object_ptr;
    }
        
    return
        get_object_pointer(obj->omp,
            obj->parent.u.object_id.object_type,
            obj->parent.u.object_id.object_instance);
}

static inline attribute_t*
get_attribute_pointer (object_t *obj, int attribute_id)
{
    void *found;
    attribute_t searched;

    searched.attribute_id = attribute_id;
    if (index_obj_search(&obj->attributes, &searched, &found) == 0) {
        return found;
    }

    return NULL;
}

/******************************************************************************
 *
 * attribute value related functions
 *
 */

#define ATTRIBUTE_VALUE_IS_SIMPLE_FLAG          (0x1)
#define ATTRIBUTE_VALUE_IS_COMPLEX_FLAG         (0x2)

/*
 * conveniences to access the attribute value union
 */
#define ATTRIBUTE_VALUE_IS_SIMPLE(avp) \
    (avp && (avp->attribute_flags & ATTRIBUTE_VALUE_IS_SIMPLE_FLAG))

#define ATTRIBUTE_VALUE_IS_COMPLEX(avp) \
    (avp && (avp->attribute_flags & ATTRIBUTE_VALUE_IS_COMPLEX_FLAG))

#define SIMPLE_ATTRIBUTE_VALUE(avp) \
    (avp->attribute_value_u.simple_attribute_value)

#define COMPLEX_ATTRIBUTE_VALUE_LENGTH(avp) \
    (avp->attribute_value_u.complex_attribute_value.length)

#define COMPLEX_ATTRIBUTE_VALUE_PTR(avp) \
    (avp->attribute_value_u.complex_attribute_value.data)

/*
 * Is 'value' the same as the value in 'avp' ?
 */
static boolean
same_attribute_simple_values (attribute_value_t *avp,
    long long int value)
{
    assert(avp->ref_count > 0);
    if (ATTRIBUTE_VALUE_IS_SIMPLE(avp)) {
        return (SIMPLE_ATTRIBUTE_VALUE(avp) == value);
    }
    return false;
}

/*
 * Is byte stream 'data' of length 'len' bytes
 * the same as the value in 'avp' ?
 */
static boolean
same_attribute_complex_values (attribute_value_t *avp,
    byte *data, int len)
{
    assert(avp->ref_count > 0);
    if (ATTRIBUTE_VALUE_IS_COMPLEX(avp) && 
        (COMPLEX_ATTRIBUTE_VALUE_LENGTH(avp) == len)) {
            return
                (memcmp(data, COMPLEX_ATTRIBUTE_VALUE_PTR(avp), len) == 0);
    }
    return false;
}

/*
 * In attribute instance 'aip' find the value which 
 * matches the simple attribute value of 'value'.
 * Return a pointer to it if found.
 */
static attribute_value_t *
attribute_simple_value_find (attribute_t *aip,
    long long int value, attribute_value_t **prev)
{
    attribute_value_t *avp = aip->avp_list;

    safe_pointer_set(prev, NULL);
    while (avp) {
        if (same_attribute_simple_values(avp, value)) return avp;
        safe_pointer_set(prev, avp);
        avp = avp->next_avp;
    }
    return NULL;
}

/*
 * In attribute instance 'aip' find the value which 
 * matches the complex attribute value of 'data' and 'len'.
 * Return a pointer to it if found.
 */
static attribute_value_t *
attribute_complex_value_find (attribute_t *aip,
    byte *data, int len, attribute_value_t **prev)
{
    attribute_value_t *avp = aip->avp_list;

    safe_pointer_set(prev, NULL);
    while (avp) {
        if (same_attribute_complex_values(avp, data, len)) return avp;
        safe_pointer_set(prev, avp);
        avp = avp->next_avp;
    }
    return NULL;
}

/*
 * obliterate & free all storages of all the attribute values
 * belonging to attribute instance 'aip'.
 */
static void
attribute_values_remove_all (attribute_t *aip)
{
    attribute_value_t *deleter;

    while (aip->avp_list) {
        deleter = aip->avp_list;
        aip->avp_list = aip->avp_list->next_avp;
        aip->n -= deleter->ref_count;
        if (ATTRIBUTE_VALUE_IS_COMPLEX(deleter)) {
            mem_monitor_free(COMPLEX_ATTRIBUTE_VALUE_PTR(deleter));
        }
        mem_monitor_free(deleter);
    }
    assert(aip->n == 0);
    assert(aip->avp_list == NULL);
}

/*
 * add an attribute value to an attribute id.
 * This function can add both a simple or a complex
 * attribute value, depending on 'complex'.  The values
 * will be taken from different parameters depending on
 * 'complex'.
 *
 * Note that since the same exact value (simple or complex)
 * can be added multiple times to an attribute id, a parameter
 * named 'howmany_extras' is added.
 *
 * It does exactly what it says.  It adds EXTRA count of
 * values.  If it is 0, the attribute value will be added
 * only once.  0 specifies a one time unique value.  If you
 * call the function with the same value and 0 extras,
 * there will always be ONE copy of the value.  If you want
 * the same value added 10 times, you can call the function
 * either 10 times with 'howmany_extras' as one, OR call
 * it once with 'howmany_extras' as 9.
 */
static int
attribute_value_add_engine (attribute_t *aip,
    boolean complex,
    long long int value,        /* used when 'complex' is false */
    byte *data, int len,        /* used when 'complex' is true */
    int howmany_extras)
{
    attribute_value_t *avp;
    byte *d;

    if (complex) {
        avp = attribute_complex_value_find(aip, data, len, NULL);
    } else {
        avp = attribute_simple_value_find(aip, value, NULL);
    }
    if (avp) {
        avp->ref_count += howmany_extras;
        aip->n += howmany_extras;
        return 0;
    }

    avp = MEM_MONITOR_ALLOC(aip->object->omp, sizeof(attribute_value_t));
    if (NULL == avp) return ENOMEM;
    if (complex) {
        d = MEM_MONITOR_ALLOC(aip->object->omp, len);
        if (NULL == d) {
            mem_monitor_free(avp);
            return ENOMEM;
        }
        avp->attribute_flags = ATTRIBUTE_VALUE_IS_COMPLEX_FLAG;
        COMPLEX_ATTRIBUTE_VALUE_LENGTH(avp) = len;
        COMPLEX_ATTRIBUTE_VALUE_PTR(avp) = d;
        memcpy(d, data, len);
    } else {
        avp->attribute_flags = ATTRIBUTE_VALUE_IS_SIMPLE_FLAG;
        SIMPLE_ATTRIBUTE_VALUE(avp) = value;
    }
    
    avp->ref_count = 1 + howmany_extras;
    avp->next_avp = aip->avp_list;
    aip->avp_list = avp;
    aip->n += avp->ref_count;

    return 0;
}

static inline int
attribute_simple_value_add (attribute_t *aip,
    long long int value, int howmany_extras)
{
    return
        attribute_value_add_engine(aip, false,
            value, NULL, 0, howmany_extras);
}

/*
 * the 'set' function is different.  It deletes all other values
 * and just sets the value to the new one.
 */
static inline int
attribute_simple_value_set (attribute_t *aip,
    long long int value, int ref_count)
{
    attribute_values_remove_all(aip);
    return
        attribute_simple_value_add(aip, value, (ref_count-1));
}

static inline int
attribute_complex_value_add (attribute_t *aip,
    byte *data, int len, int howmany_extras)
{
    return
        attribute_value_add_engine(aip, true,
            0, data, len, howmany_extras);
}

static inline boolean
attribute_complex_value_set (attribute_t *aip,
    byte *data, int len, int ref_count)
{
    attribute_values_remove_all(aip);
    return
        attribute_complex_value_add(aip, data, len, (ref_count-1));
}

/*
 * You can delete multiple occurences of the same attribute
 * value.  This is specified in 'howmany'.
 * If that value is greater than the total number of values,
 * then all those values will be deleted.
 */
static int
attribute_value_remove_engine (attribute_t *aip,
    boolean complex,
    long long int value,        /* used when 'complex' is false */
    byte *data, int len,        /* used when 'complex' is true */
    int howmany)
{
    attribute_value_t *avp, *prev;

    if (complex) {
        avp = attribute_complex_value_find(aip, data, len, &prev);
    } else {
        avp = attribute_simple_value_find(aip, value, &prev);
    }
    if (NULL == avp) return ENOENT;
    if (howmany > avp->ref_count) howmany = avp->ref_count;
    avp->ref_count -= howmany;
    aip->n -= howmany;
    assert(aip->n >= 0);
    if (avp->ref_count <= 0) {
        if (NULL == prev) {
            assert(aip->avp_list == avp);
            aip->avp_list = aip->avp_list->next_avp;
        } else {
            assert(prev->next_avp == avp);
            prev->next_avp = avp->next_avp;
        }
        if (complex) mem_monitor_free(COMPLEX_ATTRIBUTE_VALUE_PTR(avp));
        mem_monitor_free(avp);
    }
    return 0;
}

static inline int
attribute_simple_value_remove (attribute_t *aip,
    long long int value, int howmany)
{
    return
        attribute_value_remove_engine(aip, false,
            value, NULL, 0, howmany);
}

static inline int
attribute_complex_value_remove (attribute_t *aip,
    byte *data, int len, int howmany)
{
    return
        attribute_value_remove_engine(aip, true,
            0, data, len, howmany);
}

/*
 * Get all the values associated with an attribute id.
 *
 * Return all the attribute value pointers to the requester as pointers
 * in the supplied array of attribute value pointers.  The 'limit' is
 * the size of the array passed in by the caller and only up to that
 * many attribute values will be returned.  'limit' is also used to return
 * how many attribute values have been returned.
 */
static void
attribute_values_get_all (attribute_t *aip,
    attribute_value_t *avp_list [], int *limit)
{
    attribute_value_t *avp = aip->avp_list;
    int i, n = *limit;

    /* add them to the array one by one */
    for (i = 0; ((i < n) && avp); i++) {
        avp_list[i] = avp;
        avp = avp->next_avp;
    }
    *limit = i;
}

/******************************************************************************/

static void
attribute_remove (attribute_t *aip)
{
    object_t *obj = aip->object;
    void *removed_data;

    /* delete all its values first */
    attribute_values_remove_all(aip);

    /* remove the attribute from its object */
    index_obj_remove(&obj->attributes, aip, &removed_data);
    assert(aip == removed_data);

    mem_monitor_free(aip);
}

static void
attributes_remove_all (object_t *obj)
{
    int i;

    /* delete each attribute one by one */
    for (i = 0; i < obj->attributes.n; i++) {
        attribute_remove(obj->attributes.elements[i]);
    }
    assert(0 == obj->attributes.n);
    index_obj_reset(&obj->attributes);
}

static inline void
object_indexes_init (object_t *obj)
{
    mem_monitor_t *memp = obj->omp->mem_mon_p;

    assert(0 == index_obj_init(&obj->children, 
                    false, compare_objects,
                    8, 8, memp));
    assert(0 == index_obj_init(&obj->attributes,
                    false, compare_attributes,
                    8, 8, memp));
}

/*
 * get object type and object instance values of an object.
 */
static void
get_ot_and_oi (object_representation_t *reprp,
        int *otp, int *oip)
{
    *otp = *oip = -1;
    if (NULL == reprp) return;
    if (reprp->is_pointer) {
        *otp = reprp->u.object_ptr->object_type;
        *oip = reprp->u.object_ptr->object_instance;
    } else {
        *otp = reprp->u.object_id.object_type;
        *oip = reprp->u.object_id.object_instance;
    }
}

/*
 * This function traverses all children of 'root' recursively all
 * the way to the last children.  Note however that it does not use
 * actual recursion to do it, since in very deep trees, it would run
 * out of stack.  It does it in an iterative way.  It therefore does
 * not need any extra memory.
 *
 * Note that the traversal of root is EXCLUDED.  Only its children
 * are traversed (not the mother).
 *
 * Note that the 'current' value of the indexes must be all at 0 when
 * starting the traversal and must end with value 0 when the traversal
 * ends, so that they are ready for the next traversal.  Therefore, the
 * traversal continues till the very end, even though user function
 * may no longer be called due to an error it may have returned.
 *
 * Function return value is the first ever error value returned by the
 * user function while traversing the tree.
 */
static int
object_children_traverse (object_manager_t *omp, object_t *root,
        traverse_function_pointer tfn,
        void *p0, void *p1, void *p2, void *p3, void *p4)
{
    index_obj_t *idxp;
    object_t *obj;
    int failed = 0;

    if (NULL == root) return 0;

    /*
     * make sure that the user supplied traversal function
     * cannot change anything in this object manager.
     */
    omp->should_not_be_modified = true;

    obj = root;
    idxp = &obj->children;
    while (true) {

        /* we apply the user function the very first time we enter it */
        if ((0 == idxp->current) && (0 == failed) && (obj != root)) {
            failed = tfn(omp, obj, p0, p1, p2, p3, p4);
        }

        /* if it has any remaining children go down to it */
        if (idxp->current < idxp->n) {
            obj = idxp->elements[idxp->current++];
            idxp = &obj->children;

            /* shld we do this here on first entry into the object */
            idxp->current = 0;

        /* all children of this object traversed, go back to the parent */
        } else {
            idxp->current = 0;
            obj = get_parent_pointer(obj);
            idxp = &obj->children;

            /* back at root and processed every child */
            if ((obj == root) && (idxp->current >= idxp->n)) {
                idxp->current = 0;
                break;
            }
        }
    }

    /* ok traversal ended */
    omp->should_not_be_modified = false;

    return failed;
}

/*
 * This function uses the same algorithm for traversing as above but 
 * for a very specific purpose of collecting all the children of an
 * object BUT EXCLUDING the root object itself.
 *
 * It returns all the child object pointers in a malloc'ed area to the
 * user, with the number returned in 'count'.  It expands itself using
 * realloc.  If realloc fails, then it will only return a subset of 
 * the children, as much as malloc'ed area allows.  Even if realloc
 * fails however, 'total' will always return how many total children
 * are below the object.  Basically, even if memory allocation fails,
 * the entire children will be traversed & counted, but cannot obviously 
 * all be passed up in a children pointer array.  Besides, since we MUST
 * clear the 'current' values on each children index object, we MUST
 * let the traversal finish completely, regardless of whether we were
 * able to store child object pointers along the way or not.
 *
 * 'enuf_memory' is set to false when malloc/realloc fails for the very
 * first time and after that, no attempt is made again to acquire any
 * memory any more, since once failed, it is very unlikely that immediately
 * consecutive requests will succeed.  First failure therefore is the
 * only & final failure.
 *
 * The caller is responsible for freeing this storage area returned
 * (Note that NULL may be returned, if also the initial malloc fails).
 */
static object_t **
object_children_get (object_manager_t *omp, object_t *root,
        int *count, int *total)
{
    index_obj_t *idxp;
    object_t *obj, **storage, **new;
    int limit = 256;
    bool enuf_memory = true;

    *count = *total = 0;
    if (NULL == root) return NULL;
    obj = root;
    idxp = &obj->children;
    storage = (object_t**) (malloc(limit * sizeof(object_t*)));

    /*
     * we must traverse till the end, to find 'total' even
     * if the memory allocation may have failed.
     */
    if (NULL == storage) {
        ERROR(&om_debug, "malloc of %d pointers space failed\n", limit);
        enuf_memory = false;
    }

    while (true) {

        /* we want the children of the root, but EXCLUDING the root */
        if ((0 == idxp->current) && (obj != root)) {

            /* array limit reached, try & expand */
            if ((*count >= limit) && enuf_memory) {
                limit += 256;
                new = (object_t**)
                    realloc(storage, limit * sizeof(object_t*));
                if (new) {
                    storage = new;
                } else {
                    ERROR(&om_debug, "realloc of %d pointers space failed\n",
                        limit);
                    enuf_memory = false;
                }
            }

            /* as long as we have storage, record it and increment */
            if (enuf_memory) {
                storage[(*count)++] = obj;
            }

            /* this is always incremented, counting total children */
            (*total)++;
        }

        /* traverse down */
        if (idxp->current < idxp->n) {
            obj = idxp->elements[idxp->current++];
            idxp = &obj->children;

            /* shld we do this here ? */
            idxp->current = 0;

        /* all children of this object traversed, go back to the parent */
        } else {
            idxp->current = 0;
            obj = get_parent_pointer(obj);
            idxp = &obj->children;
            if ((obj == root) && (idxp->current >= idxp->n)) {
                idxp->current = 0;
                break;
            }
        }
    }

    return storage;
}

/*
 * The 'leave_parent_consistent' is used for something very subtle.
 *
 * when we remove an object, we must leave the parent's
 * children in a consistent state.... except when we know
 * we are removing a whole bunch of sub children in a
 * big swoop. Since these objects will all be removed,
 * including the parent, we do not have to do the extra and 
 * useless effort of leaving their child/parent relationship 
 * consistent since we know the parent will also be eventually 
 * destroyed.
 *
 * Therefore ONLY the top object has to be kept consistent but 
 * all the children can simply be destroyed quickly.
 *
 * This variable controls that.
 */
static void 
om_object_remove_engine (object_t *obj,
        boolean leave_parent_consistent,
        boolean remove_all_children)
{
    void *removed_obj;
    object_t **all_the_children;
    object_manager_t *omp = obj->omp;
    object_t *parent;
    int child_count, total_children, i;

    /* take object out of the parent's children index if needed */
    if (leave_parent_consistent) {
        parent = get_parent_pointer(obj);
        if (parent) {
            index_obj_remove(&parent->children, obj, &removed_obj);
        }
    }

    /* get rid of all its attribute storage */
    attributes_remove_all(obj);
    
    /* get rid of all children traversing iteratively */
    if (remove_all_children) {

        /* collect all children from every sub level into an array */
        all_the_children = object_children_get(omp, obj,
                &child_count, &total_children);
        if (child_count != total_children) {
            ERROR(&om_debug, "could only get %d children of %d "
                " for object (%d, %d)\n",
                child_count, total_children,
                obj->object_type, obj->object_instance);
        }

        /* simple delete each one from the array */
        if (child_count && all_the_children) {
            for (i = 0; i < child_count; i++) {
                om_object_remove_engine(all_the_children[i], false, false);
            }
            free(all_the_children);
        }
    }

    /* take object out of the main object index */
    assert(0 == avl_tree_remove(&omp->om_objects, obj, &removed_obj));
    assert(removed_obj == obj);

    /* free up its index objects */
    index_obj_destroy(&obj->children, NULL, NULL);
    index_obj_destroy(&obj->attributes, NULL, NULL);

    /* and blow it away */
    mem_monitor_free(obj);
}

static object_t *
om_object_create_engine (object_manager_t *omp,
        int parent_object_type, int parent_object_instance,
        int object_type, int object_instance)
{
    object_t *obj, *parent;
    object_t *exists;
    int pot, poi;

    TRACE(&om_debug, "creating (%d, %d) with parents (%d, %d)\n",
        object_type, object_instance,
        parent_object_type, parent_object_instance);

    /* create & search simultaneously */
    obj = MEM_MONITOR_ALLOC(omp, sizeof(object_t));
    if (NULL == obj) {
        WARN(&om_debug, "MEM_MONITOR_ALLOC failed for %d bytes\n",
            sizeof(object_t));
        return NULL;
    }
    obj->object_type = object_type;
    obj->object_instance = object_instance;

    /* if the object already exists simply return that */
    assert(0 == avl_tree_insert(&omp->om_objects, obj, (void**) &exists));
    if (exists) {
        get_ot_and_oi(&exists->parent, &pot, &poi);
        TRACE(&om_debug,
            "object (%d, %d) already exists with parent (%d, %d)\n",
            object_type, object_instance,
            pot, poi);
        if ((pot != parent_object_type) || (poi != parent_object_instance)) {
            ERROR(&om_debug,
                "object (%d, %d) already has parent (%d, %d), "
                "but requested parent (%d, %d) is different. "
                "Object not created.\n",
                object_type, object_instance,
                pot, poi,
                parent_object_type, parent_object_instance);
            MEM_MONITOR_FREE(obj);
            return NULL;
        }
        MEM_MONITOR_FREE(obj);
        return exists;
    }

    /* ok, it does not already exist, fill the rest */
    obj->omp = omp;
    parent = get_object_pointer(omp,
                parent_object_type, parent_object_instance);
    if (parent) {
        obj->parent.is_pointer = true;
        obj->parent.u.object_ptr = parent;
        assert(0 == index_obj_insert(&parent->children, obj,
                        (void**) &exists));
    } else {
        obj->parent.is_pointer = false;
        obj->parent.u.object_id.object_type = parent_object_type;
        obj->parent.u.object_id.object_instance = parent_object_instance;
    }

    /* initialize the children and attribute indexes */
    object_indexes_init(obj);

    TRACE(&om_debug, "object (%d, %d) created with parent (%d, %d)\n",
        object_type, object_instance,
        parent_object_type, parent_object_instance);

    /* done */
    return obj;
}

static attribute_t *
obj_attribute_add (object_t *obj, int attribute_id)
{
    attribute_t *aip;
    void *found;

    /* create the new attribute  */
    aip = MEM_MONITOR_ZALLOC(obj->omp, sizeof(attribute_t));
    if (NULL == aip) return NULL;

    /* if already there, just free up the new one & return */
    aip->attribute_id = attribute_id;
    if (index_obj_insert(&obj->attributes, aip, &found) == 0) {
        if (found) {
            mem_monitor_free(aip);
            return found;
        }
    }

    /* initialize rest of it */
    aip->object = obj;
    aip->n = 0;
    aip->avp_list = NULL;

    /* done */
    return aip;
}

static int
obj_attribute_simple_value_add (object_t *obj, int attribute_id, 
    long long int value, int howmany_extras)
{
    attribute_t *aip;

    aip = get_attribute_pointer(obj, attribute_id);
    if (NULL == aip) {
        aip = obj_attribute_add(obj, attribute_id);
        if (NULL == aip) return ENOMEM;
    }
    return
        attribute_simple_value_add(aip, value, howmany_extras);
}

static int
obj_attribute_simple_value_set (object_t *obj, int attribute_id, 
    long long int value, int ref_count)
{
    attribute_t *aip;

    aip = get_attribute_pointer(obj, attribute_id);
    if (NULL == aip) {
        aip = obj_attribute_add(obj, attribute_id);
        if (NULL == aip) return ENOMEM;
    }
    return
        attribute_simple_value_set(aip, value, ref_count);
}

static int
obj_attribute_simple_value_remove (object_t *obj, int attribute_id, 
    long long int value, int howmany)
{
    attribute_t *aip;

    aip = get_attribute_pointer(obj, attribute_id);
    if (NULL == aip) {
        return ENOENT;
    }
    return 
        attribute_simple_value_remove(aip, value, howmany);
}

static int
obj_attribute_complex_value_add (object_t *obj, int attribute_id, 
    byte *data, int len, int howmany_extras)
{
    attribute_t *aip;

    aip = get_attribute_pointer(obj, attribute_id);
    if (NULL == aip) {
        aip = obj_attribute_add(obj, attribute_id);
        if (NULL == aip) return ENOMEM;
    }
    return
        attribute_complex_value_add(aip, data, len, howmany_extras);
}

static int
obj_attribute_complex_value_set (object_t *obj, int attribute_id, 
    byte *data, int len, int ref_count)
{
    attribute_t *aip;

    aip = get_attribute_pointer(obj, attribute_id);
    if (NULL == aip) {
        aip = obj_attribute_add(obj, attribute_id);
        if (NULL == aip) return ENOMEM;
    }
    return
        attribute_complex_value_set(aip, data, len, ref_count);
}

static int
obj_attribute_complex_value_remove (object_t *obj, int attribute_id, 
    byte *data, int len, int howmany)
{
    attribute_t *aip;

    aip = get_attribute_pointer(obj, attribute_id);
    if (NULL == aip) {
        return ENOENT;
    }
    return
        attribute_complex_value_remove(aip, data, len, howmany);
}

static int
obj_attribute_remove (object_t *obj, int attribute_id)
{
    attribute_t *aip;

    aip = get_attribute_pointer(obj, attribute_id);
    if (aip) {
        attribute_remove(aip);
        return 0;
    }
    return ENODATA;
}

/*************** Public functions *********************************************/

PUBLIC int
om_init (object_manager_t *omp,
        boolean make_it_thread_safe,
        int manager_id,
        mem_monitor_t *parent_mem_monitor)
{
    memset(omp, 0, sizeof(object_manager_t));
    MEM_MONITOR_SETUP(omp);
    LOCK_SETUP(omp);
    omp->manager_id = manager_id;
    omp->should_not_be_modified = false;
    assert(0 == avl_tree_init(&omp->om_objects, false, compare_objects,
        omp->mem_mon_p));
    OBJ_WRITE_UNLOCK(omp);
    return 0;
}

PUBLIC int
om_object_create (object_manager_t *omp,
        int parent_object_type, int parent_object_instance,
        int object_type, int object_instance)
{
    int failed;
    object_t *obj;

    OBJ_WRITE_LOCK(omp);
    obj = om_object_create_engine(omp,
                parent_object_type, parent_object_instance,
                object_type, object_instance);
    failed = (obj ? 0 : EFAULT);
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC boolean
om_object_exists (object_manager_t *omp,
        int object_type, int object_instance)
{
    boolean exists;

    OBJ_READ_LOCK(omp);
    exists = (NULL != get_object_pointer(omp, object_type, object_instance));
    OBJ_READ_UNLOCK(omp);
    return exists;
}

PUBLIC int
om_attribute_add (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id)
{
    int failed;
    object_t *obj;
    attribute_t *aip;

    OBJ_WRITE_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        aip = obj_attribute_add(obj, attribute_id);
        failed = (aip ? 0 : ENOMEM);
    }
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_attribute_simple_value_add (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        long long int value, int howmany_extras)
{
    int failed;
    object_t *obj;

    OBJ_WRITE_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = obj_attribute_simple_value_add(obj,
                        attribute_id, value, howmany_extras);
    }
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_attribute_simple_value_set (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        long long int value, int ref_count)
{
    int failed;
    object_t *obj;

    OBJ_WRITE_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = obj_attribute_simple_value_set(obj,
                        attribute_id, value, ref_count);
    }
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_attribute_simple_value_remove (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        long long int value, int howmany)
{
    int failed;
    object_t *obj;

    OBJ_WRITE_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = obj_attribute_simple_value_remove(obj,
                        attribute_id, value, howmany);
    }
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_attribute_complex_value_add (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        byte *data, int len, int howmany_extras)
{
    int failed;
    object_t *obj;

    OBJ_WRITE_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = obj_attribute_complex_value_add(obj, attribute_id,
                        data, len, howmany_extras);
    }
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_attribute_complex_value_set (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        byte *data, int len, int ref_count)
{
    int failed;
    object_t *obj;

    OBJ_WRITE_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = obj_attribute_complex_value_set(obj, attribute_id,
                    data, len, ref_count);
    }
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_attribute_complex_value_remove (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        byte *data, int len, int howmany)
{
    int failed;
    object_t *obj;

    OBJ_WRITE_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = obj_attribute_complex_value_remove(obj, attribute_id,
                        data, len, howmany);
    }
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_attribute_values_get (object_manager_t *omp, 
    int object_type, int object_instance,
    int attribute_id,
    attribute_value_t *avp_list [], int *limit)
{
    object_t *obj;
    attribute_t *aip;
    int rc = 0;
    int n = *limit;

    *limit = 0;
    OBJ_READ_LOCK(omp);

    /* get object */
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        rc = ENODATA;
        goto bail;
    }

    /* get attribute */
    aip = get_attribute_pointer(obj, attribute_id);
    if (NULL == aip) {
        rc =  ENODATA;
        goto bail;
    }

    attribute_values_get_all(aip, avp_list, &n);
    *limit = n;

bail: 

    OBJ_READ_UNLOCK(omp);
    
    return rc;
}

PUBLIC int
om_attribute_remove (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id)
{
    int failed;
    object_t *obj;

    OBJ_WRITE_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = obj_attribute_remove(obj, attribute_id);
    }
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_object_remove (object_manager_t *omp,
        int object_type, int object_instance)
{
    int failed;
    object_t *obj;

    OBJ_WRITE_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = 0;
        om_object_remove_engine(obj, true, true);
    }
    OBJ_WRITE_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_parent_get (object_manager_t *omp,
        int object_type, int object_instance,
        int *parent_object_type, int* parent_object_instance)
{
    object_t *obj;
    int failed = 0;

    OBJ_READ_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) failed = ENODATA;
    get_ot_and_oi(&obj->parent, parent_object_type, parent_object_instance);
    OBJ_READ_UNLOCK(omp);
    return failed;
}

PUBLIC int
om_traverse (object_manager_t *omp,
        int object_type, int object_instance,
        traverse_function_pointer tfn,
        void *p0, void *p1, void *p2, void *p3, void *p4)
{
    int failed = 0;
    object_t *obj;

    OBJ_READ_LOCK(omp);
    obj = get_object_pointer(omp, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = object_children_traverse (omp, obj, tfn,
                    p0, p1, p2, p3, p4);
    }
    OBJ_READ_UNLOCK(omp);
    return failed;
}

PUBLIC void
om_destroy (object_manager_t *omp)
{
    object_t *root = NULL;

    OBJ_WRITE_LOCK(omp);
    if (omp->om_objects.root_node != NULL) {
        root = (object_t*) omp->om_objects.root_node->user_data;
        om_object_remove_engine(root, false, false);
    }
    OBJ_WRITE_UNLOCK(omp);
    LOCK_OBJ_DESTROY(omp);
}

/******************************************************************************
 *
 * Reading and writing the object manager from/to a file for permanency.
 */

/*
 * acronyms used in the object manager file
 */
static char *object_acronym = "OBJ";
static char *attribute_id_acronym = "AID";
static char *attribute_complex_value_acronym = "CAV";
static char *attribute_simple_value_acronym = "SAV";

/************* Writing the object manager to a file functions *************/

static void
om_write_one_attribute_simple_value (FILE *fp, attribute_value_t *avp)
{
    fprintf(fp, "\n    %s %d %lld",
        attribute_simple_value_acronym,
        avp->ref_count,
        SIMPLE_ATTRIBUTE_VALUE(avp));

}

static void
om_write_one_attribute_complex_value (FILE *fp, attribute_value_t *avp)
{
    byte *bptr;
    int i, len;

    len = COMPLEX_ATTRIBUTE_VALUE_LENGTH(avp);
    fprintf(fp, "\n    %s %d %d ",
        attribute_complex_value_acronym, avp->ref_count, len);
    bptr = COMPLEX_ATTRIBUTE_VALUE_PTR(avp);
    for (i = 0; i < len; i++) {
        fprintf(fp, "%d ", *bptr);
        bptr++;
    }
}

static void
om_write_one_attribute_value (FILE *fp, attribute_value_t *avp)
{
    assert(avp->ref_count > 0);
    if (ATTRIBUTE_VALUE_IS_SIMPLE(avp)) {
        om_write_one_attribute_simple_value(fp, avp);
    } else if (ATTRIBUTE_VALUE_IS_COMPLEX(avp)) {
        om_write_one_attribute_complex_value(fp, avp);
    } else {
        FATAL_ERROR(&om_debug, "unknown attribute type 0x%x\n",
            avp->attribute_flags);
    }
}

static void
om_write_one_attribute (FILE *fp, void *vattr)
{
    attribute_t *attr = (attribute_t*) vattr;
    attribute_value_t *avp;

    fprintf(fp, "\n  %s %d ", attribute_id_acronym, attr->attribute_id);
    avp = attr->avp_list;
    while (avp) {
        om_write_one_attribute_value(fp, avp);
        avp = avp->next_avp;
    }
}

static int
om_write_one_object_tfn (void *utility_object, void *utility_node,
        void *v_object, void *v_FILE, 
        void *p1, void *p2, void *p3)
{
    object_t *obj = (object_t*) v_object;
    FILE *fp = v_FILE;
    int i;
    int pt, pi;

    get_ot_and_oi(&obj->parent, &pt, &pi);
    fprintf(fp, "\n%s %d %d %d %d",
        object_acronym, 
        pt, pi,
        obj->object_type, obj->object_instance);
    for (i = 0; i < obj->attributes.n; i++) {
        om_write_one_attribute(fp, obj->attributes.elements[i]);
    }

    /* must return 0 to continue traversing */
    return 0;
}

/*
 * Write the object manager out to disk.
 *
 * THIS IS *** VERY *** IMPORTANT SO UNDERSTAND IT WELL.
 *
 * We have to ensure that the object manager is written out in a way such
 * that when we recreate it reading back from the file, the parent 
 * of an object to be created must already have been written out 
 * earlier such that when we create an object, its parent
 * already exists and we can associate the two objects.  Now, to be
 * able to do that, we have to start recursively writing from root
 * object down, first writing out the object and its children later.
 * This way since children are written out later, the parent object has
 * already been created.  So, writing the object manager to a file involves
 * writing the current object out, followed recusively writing out
 * all its children.
 *
 * This is fine and dandy with ONE HUGE PROBLEM.  For very deep object
 * managers, we BADLY run out of recursion stack no matter how big a
 * stack is allocated.  So, recursion is useless in this case.
 *
 * Here is the alternative.  The 'om_objects' in the object manager holds 
 * ALL the objects but in random order (NOT neatly parent followed 
 * by children as we want).  But since our tree traversal uses morris
 * traversals, it does not use any extra stack or queue or any kind
 * of memory.  This makes it PERFECT for extremely large object managers
 * since we never run out of stack space.  But now, we introduce the
 * problem where we may have to create an object without having yet 
 * created its parent.  So, how do we solve this problem ? This actually
 * is not as difficult but it just needs a second pass over all the 
 * objects.
 *
 * In the first pass, we create the objects and if we happen to have 
 * their parent already available we can do the association immediately.
 * However, there will be many objects in which the parents are NOT
 * yet created.  In those cases, we store the parent object identifier
 * (in the form of type, instance) and move to the next object.
 *
 * In the second pass, since we have created ALL the objects, we simply
 * scan thru all the objects whose parents have not yet been resolved 
 * and associate them.  When the association is complete all objects
 * will have their parents represented as 'pointer' values, EXCEPT
 * for the very first object created, since it will not have a parent.
 * This is the root of the avl tree.
 *
 */

PUBLIC int
om_write (object_manager_t *omp)
{
    FILE *fp;
    char om_name [TYPICAL_NAME_SIZE];
    char backup_om_name [TYPICAL_NAME_SIZE];
    char backup_om_tmp [TYPICAL_NAME_SIZE];
    void *unused = NULL;    // shut the compiler up

    OBJ_READ_LOCK(omp);

    sprintf(om_name, "om_%d", omp->manager_id);
    sprintf(backup_om_name, "%s_BACKUP", om_name);
    sprintf(backup_om_tmp, "%s_tmp", backup_om_name);

    /* does not matter if these fail */
    unlink(backup_om_tmp);
    rename(backup_om_name, backup_om_tmp);
    rename(om_name, backup_om_name);

    fp = fopen(om_name, "w");
    if (NULL == fp) {
        OBJ_READ_UNLOCK(omp);
        return -1;
    }
    avl_tree_traverse(&omp->om_objects, NULL, om_write_one_object_tfn,
        fp, unused, unused, unused);

    /* close up the file */
    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);

#if 0 // error
    unlink(om_name);
    rename(backup_om_name, om_name);
    unlink(backup_om_tmp);
#endif
    OBJ_READ_UNLOCK(omp);

    return 0;
}

/*************** Reading back from a file functions ******************/

static int
load_object (object_manager_t *omp, FILE *fp,
        object_t **objp)
{
    int count;
    int parent_type, parent_instance, object_type, object_instance;

    count = fscanf(fp, "%d %d %d %d",
                &parent_type, &parent_instance, 
                &object_type, &object_instance);
    if (4 != count) return -1;
    *objp = om_object_create_engine(omp,
                parent_type, parent_instance,
                object_type, object_instance);
    return *objp ? 0 : -1;
}

static int
load_attribute_id (object_manager_t *omp, FILE *fp,
    object_t *obj, attribute_t **aipp)
{
    int aid;

    /* we should NOT have a NULL object at this point */
    if (NULL == obj) return -1;

    if (fscanf(fp, "%d", &aid) != 1)
        return -1;
    *aipp = obj_attribute_add(obj, aid);
    if (NULL == *aipp)
        return -1;

    return 0;
}

static int
load_attribute_simple_value (object_manager_t *omp, FILE *fp,
    attribute_t *aip)
{
    long long int value;
    int ref_count;

    /* we should NOT have a NULL attribute id at this point */
    if (NULL == aip) return -1;

    if (fscanf(fp, " %d %lld", &ref_count, &value) != 2) return -1;
    return
        attribute_simple_value_add(aip, value, ref_count-1);

    return 0;
}

static int
load_attribute_complex_value (object_manager_t *omp, FILE *fp,
    attribute_t *aip)
{
    byte *value;
    int i, len, ref_count;
    int err;

    /* we should NOT have a NULL attribute id at this point */
    if (NULL == aip) return -1;

    /* read ref count & length */
    if (fscanf(fp, "%d %d ", &ref_count, &len) != 2) return -1;

    /* allocate temp space */
    value = (byte*) malloc(len + 1);
    if (NULL == value) return -1;

    /* read each data byte in */
    for (i = 0; i < len; i++) {
        if (fscanf(fp, "%d", (int*) (&value[i])) != 1) {
            free(value);
            return -1;
        }
    }

    /* add complex value to the attribute */
    err = attribute_complex_value_add(aip, value, len, ref_count-1);

    /* free up temp storage */
    free(value);

    /* done */
    return err;
}

/*
 * This function is called on every object and resolves its parent
 * pointer if it is not already in the form of a pointer.
 */
static int
resolve_parent_tfn (void *utility_object, void *utility_node,
    void *user_data, void *v_omp,
    void *v_1, void *v_2, void *v_3)
{
    object_manager_t *omp;
    object_t *obj, *parent;

    omp = (object_manager_t*) v_omp;
    obj = (object_t*) user_data;
    if (!(obj->parent.is_pointer)) {
        parent = get_object_pointer(omp,
                    obj->parent.u.object_id.object_type,
                    obj->parent.u.object_id.object_instance);
        if (parent) {
            obj->parent.is_pointer = true;
            obj->parent.u.object_ptr = parent;
        }
    }
    return 0;
}

static int
om_resolve_all_parents (object_manager_t *omp)
{
    void *unused = NULL;

    avl_tree_traverse(&omp->om_objects, NULL, resolve_parent_tfn, omp, 
        unused, unused, unused);
    return 0;
}

PUBLIC int
om_read (int manager_id, object_manager_t *omp)
{
    char om_name [TYPICAL_NAME_SIZE];
    FILE *fp;
    int failed, count;
    object_t *obj, *parent;
    attribute_t *aip;
    char string [TYPICAL_NAME_SIZE];

    sprintf(om_name, "om_%d", manager_id);
    fp = fopen(om_name, "r");
    if (NULL == fp) return -1;

    //om_destroy(omp);
    if (om_init(omp, true, manager_id, NULL) != 0) {
        return -1;
    }

    obj = parent = NULL;
    aip = NULL;
    failed = 0;

    while ((count = fscanf(fp, "%s", string)) != EOF) {

        /* skip empty lines */
        if (count != 1) continue;

        if (strcmp(string, object_acronym) == 0) {
            if (load_object(omp, fp, &obj) != 0) {
                failed = -1;
                break;
            }
        } else if (strcmp(string, attribute_id_acronym) == 0) {
            if (load_attribute_id(omp, fp, obj, &aip) != 0) {
                failed = -1;
                break;
            }
        } else if (strcmp(string, attribute_simple_value_acronym) == 0) {
            if (load_attribute_simple_value(omp, fp, aip) != 0) {
                failed = -1;
                break;
            }
        } else if (strcmp(string, attribute_complex_value_acronym) == 0) {
            if (load_attribute_complex_value(omp, fp, aip) != 0) {
                failed = -1;
                break;
            }
        }
    }
    fclose(fp);

    /*
     * now perform a second pass over the object manager 
     * to resolve all un-resolved parent pointers
     */
    if (0 == failed) {
        return
            om_resolve_all_parents(omp);
    }

    return failed;
}

#ifdef __cplusplus
} // extern C
#endif 



