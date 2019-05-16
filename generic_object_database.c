
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

#include "generic_object_database.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

/******************************************************************************
 *
 * global variables which control whether an avl tree or the index
 * object should be used for some heavily used data structures.
 * Using avl trees makes all insertions/deletions much faster but
 * uses much more memory.  If your database is relatively static after
 * it has been created (ie, it is mostly used for lookups rather than
 * continually being inserted into and deleted from), then this value
 * can be set to '0'.  However if it is very dynamic, then set this
 * to '1' if to get the best speed performance (but maximum memory
 * consumption).  Also, if memory consefailedation is the MOST important
 * consideration, then always set the avl usage variables to 0.
 */
static int use_avl_tree_for_database_object_index = 1;
static int use_avl_tree_for_object_children = 1;
#ifndef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES
static int use_avl_tree_for_attribute_ids = 1;
#endif

/*
 * forward declarations
 */

static int
database_announce_event (object_database_t *obj_db,
    int event,
    object_t *obj, object_t *obj_related, 
    int attribute_id, attribute_value_t *related_attribute_value);

/******************************************************************************
 *
 * general & common support functions, used by anyone
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

static inline int
object_is_root (object_t *obj)
{
    return
        (obj->object_type == ROOT_OBJECT_TYPE);
}

static object_t *
get_object_pointer (object_database_t *obj_db,
        int object_type, int object_instance)
{
    object_t searched;
    void *found;

    if (object_type == ROOT_OBJECT_TYPE) {
        return
            &obj_db->root_object;
    }

    searched.object_type = object_type;
    searched.object_instance = object_instance;
    if (0 == table_search(&obj_db->object_index, &searched, &found)) {
        return found;
    }
    return NULL;
}

static object_t *
get_parent_pointer (object_t *obj)
{
    if (obj->parent.is_pointer) {
        return
            obj->parent.u.object_ptr;
    }
        
    return
        get_object_pointer(obj->obj_db,
            obj->parent.u.object_id.object_type,
            obj->parent.u.object_id.object_instance);
}

static attribute_instance_t *
get_attribute_instance_pointer (object_t *obj, int attribute_id)
{
    void *found;

#ifdef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES
    if (dynamic_array_get(&obj->attributes, attribute_id, &found) == 0) {
        return found;
    }
#else
    attribute_instance_t searched;

    searched.attribute_id = attribute_id;
    if (table_search(&obj->attributes, &searched, &found) == 0) {
        return found;
    }
#endif
    return NULL;
}

/*
 * It is very important to understand this function.
 * It works to cut down redundant event announcements
 * during a deletion event.  In a deletion event,
 * only the topmost delete needs to be announced.
 * sub event deletions need not be sent and must be suppressed
 * to cut unnecessary chatter between the databases.
 * The purpose of this function is to set the blocked event
 * bits.  Once those are set, those bits MUST stick and
 * perhaps be added with more bits as calls get deeper
 * within the stack.  This is why "|=" is used to accumulate
 * more events to block.  This has the effect of making sure that
 * if an event was already blocked before this call, it STAYS
 * blocked, regardless of what this bit says.  If however,
 * it was not blocked before, it still stays unblocked.
 */
static int
allow_event_if_not_already_blocked (object_database_t *obj_db, 
        int allowed_events)
{
    int orig = obj_db->blocked_events;

    obj_db->blocked_events |= (~allowed_events);
    return orig;
}

static void
restore_events (object_database_t *obj_db, int events)
{
    obj_db->blocked_events = events;
}

/*
 * This function collects all FIRST level children of an object
 * whose object types match a specified type.
 * It collects them into the collector array up to the specified
 * limit.
 */
static int
get_matching_children_tfn (void *utility_object, void *utility_node,
    void *user_data, void *v_matching_object_type, 
    void *v_collector, void *v_index, void *v_limit)
{
    object_t *obj = user_data;
    int matching_object_type = pointer2integer(v_matching_object_type);
    object_representation_t *collector; 
    object_representation_t *rep;
    int *index;

    /* end of iteration */
    if (NULL == obj) return 0;

    /* if object type does not match, skip it */
    if ((matching_object_type != ALL_OBJECT_TYPES) &&
        (matching_object_type != obj->object_type))
            return 0;

    index = (int*) v_index;
    if (*index >= pointer2integer(v_limit)) return ENOSPC;
    
    collector = (object_representation_t*) v_collector;
    rep = &collector[*index];
    rep->is_pointer = 0;
    rep->u.object_id.object_type = obj->object_type;
    rep->u.object_id.object_instance = obj->object_instance;
    (*index)++;

    return 0;
}

/*
 * This is similar to above but it collects ALL the matching objects below
 * the specified object, including children, grand children and ALL.
 * Entire set of descendants.
 *
 * This is used internally, so the collector array stores the
 * object pointers.
 */
static int
get_matching_descendants_tfn (void *utility_object, void *utility_node,
    void *user_data, void *v_matching_object_type, 
    void *v_collector, void *v_index, void *v_limit)
{
    object_t *obj = user_data;
    int matching_object_type = pointer2integer(v_matching_object_type);
    object_representation_t *collector; 
    int *index;

    /* end of iteration */
    if (NULL == obj) return 0;

    /* if object type does not match, skip it */
    if ((matching_object_type != ALL_OBJECT_TYPES) &&
        (matching_object_type != obj->object_type))
            return 0;

    index = (int*) v_index;
    if (*index >= pointer2integer(v_limit)) return ENOSPC;
    
    collector = (object_representation_t*) v_collector;
    collector[*index].is_pointer = 1;
    collector[*index].u.object_ptr = obj;
    (*index)++;

    /* now get all the descendants too */
    table_traverse(&obj->children, get_matching_descendants_tfn,
            v_matching_object_type, v_collector, v_index, v_limit);

    return 0;
}

/******************************************************************************
 *
 * attribute related functions
 *
 */

#ifndef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES

/*
 * if dynamic arrays are not used for storing attribute ids, the table
 * object is used.  This function is therefore needed as the comparison
 * function to support that object
 */
static int
compare_attribute_ids (void *att1, void *att2)
{
    return
        ((attribute_instance_t*) att1.pointer)->attribute_id -
        ((attribute_instance_t*) att2.pointer)->attribute_id;
}

#endif 

static attribute_value_t *
create_simple_attribute_value (mem_monitor_t *memp, long long int value)
{
    attribute_value_t *avtp;

    if (memp) {
        avtp = mem_monitor_allocate(memp, sizeof(attribute_value_t));
    } else {
        avtp = malloc(sizeof(attribute_value_t));
    }
    if (NULL != avtp) {
        avtp->next_attribute_value = NULL;
        avtp->attribute_value_data = value;
        avtp->attribute_value_length = 0;
    }
    return avtp;
}

static attribute_value_t *
clone_simple_attribute_value (attribute_value_t *avtp)
{
    return
        create_simple_attribute_value(NULL, avtp->attribute_value_data);
}

static int
same_simple_attribute_values (attribute_value_t *avtp, long long int value)
{
    return
        (avtp->attribute_value_length == 0) &&
        (avtp->attribute_value_data == value);
}

static attribute_value_t *
find_simple_attribute_value (attribute_instance_t *aitp, long long int value,
        attribute_value_t **previous_avtp)
{
    attribute_value_t *avtp;

    safe_pointer_set(previous_avtp, NULL);
    avtp = aitp->avps;
    while (avtp) {
        if (same_simple_attribute_values(avtp, value)) return avtp;
        safe_pointer_set(previous_avtp, avtp);
        avtp = avtp->next_attribute_value;
    }
    return NULL;
}

static attribute_value_t *
create_complex_attribute_value (mem_monitor_t *memp, 
        byte *complex_value_data, int complex_value_data_length)
{
    attribute_value_t *avtp;
    int size = sizeof(attribute_value_t);

    /* adjust for stream whose length > sizeof(long long int) */
    if (complex_value_data_length > (int) sizeof(long long int)) {
        size = size + complex_value_data_length - sizeof(long long int);
    }

    /* allocate enuf space */
    if (memp) {
        avtp = mem_monitor_allocate(memp, size);
    } else {
        avtp = malloc(size);
    }
    if (NULL == avtp) return NULL;

    /* set the stuff */
    avtp->next_attribute_value = NULL;
    avtp->attribute_value_length = complex_value_data_length;
    memcpy((void*) &avtp->attribute_value_data,
                complex_value_data, complex_value_data_length);

    return avtp;
}

static attribute_value_t *
clone_complex_attribute_value (attribute_value_t *avtp)
{
    return
        create_complex_attribute_value(NULL, 
            (byte*) &avtp->attribute_value_data, 
            avtp->attribute_value_length);
}

static int
same_complex_attribute_values (attribute_value_t *avtp, 
    byte *complex_value_data, int complex_value_data_length)
{
    return
        (complex_value_data_length > 0) && 
        (avtp->attribute_value_length == complex_value_data_length) &&
        (memcmp((void*) &avtp->attribute_value_data,
                    complex_value_data, complex_value_data_length) == 0);
}

static attribute_value_t *
find_complex_attribute_value (attribute_instance_t *aitp,
    byte *complex_value_data, int complex_value_data_length,
    attribute_value_t **previous_avtp)
{
    attribute_value_t *avtp;

    safe_pointer_set(previous_avtp, NULL);
    avtp = aitp->avps;
    while (avtp) {
        if (same_complex_attribute_values(avtp,
                    complex_value_data, complex_value_data_length)) return avtp;
        safe_pointer_set(previous_avtp, avtp);
        avtp = avtp->next_attribute_value;
    }
    return NULL;
}

static attribute_value_t *
clone_attribute_value (attribute_value_t *avtp)
{
    if (avtp->attribute_value_length > 0) {
        return
            clone_complex_attribute_value(avtp);
    }
    return
        clone_simple_attribute_value(avtp);
}

static void
add_attribute_value_to_list (attribute_instance_t *aitp, 
    attribute_value_t *avtp)
{
    /* always add to head, much faster */
    avtp->next_attribute_value = aitp->avps;
    aitp->avps = avtp;
    aitp->n_attribute_values++;
}

static void
remove_attribute_value_from_list (attribute_instance_t *aitp, 
    attribute_value_t *avtp, attribute_value_t *prev_avtp)
{
    if (prev_avtp) {
        prev_avtp->next_attribute_value = avtp->next_attribute_value;
    } else {
        aitp->avps = avtp->next_attribute_value;
    }
    aitp->n_attribute_values--;
}

/*
 * WILL handle NULL keep, in fact this is deliberately used in
 * some functions below so do *NOT* check for NULL keep.
 */
static void
destroy_all_attribute_values_except (attribute_instance_t *aitp,
    attribute_value_t *keep)
{
    attribute_value_t *curr, *next;
    object_t *obj;
    int events;

    if (NULL == aitp) return;
    obj = aitp->object;

    events = allow_event_if_not_already_blocked(obj->obj_db, 
            ATTRIBUTE_VALUE_DELETED);
    
    /* now delete all skipping over the kept one */
    curr = aitp->avps;
    while (curr) {
        next = curr->next_attribute_value;
        if (keep != curr) {
            database_announce_event(obj->obj_db, ATTRIBUTE_VALUE_DELETED, obj,
                NULL, aitp->attribute_id, curr);
            MEM_MONITOR_FREE(obj->obj_db, curr);
        }
        curr = next;
    }

    restore_events(obj->obj_db, events);

    /*
     * if we kept any, we can now make it the ONLY one in the list,
     * otherwise, there should ne nothing left in the list
     */
    aitp->avps = keep;
    if (keep) {
        keep->next_attribute_value = NULL;
        aitp->n_attribute_values = 1;
    } else {
        aitp->n_attribute_values = 0;
    }
}

/******************************************************************************/

static attribute_value_t *
attribute_add_simple_value_engine (attribute_instance_t *aitp,
    long long int value)
{
    attribute_value_t *avtp;
    object_t *obj;

    if (NULL == aitp) return NULL;
    obj = aitp->object;

    /* value already there ? */
    avtp = find_simple_attribute_value(aitp, value, NULL);
    if (avtp) return avtp;

    /* not there, add it */
    avtp = create_simple_attribute_value(obj->obj_db->mem_mon_p, value);
    if (avtp) {
        add_attribute_value_to_list(aitp, avtp);
        database_announce_event(obj->obj_db, ATTRIBUTE_VALUE_ADDED, obj,
            NULL, aitp->attribute_id, avtp);
    }

    return avtp;
}

static int
attribute_add_simple_value (attribute_instance_t *aitp, 
    long long int value)
{
    attribute_value_t *avtp;
        
    avtp = attribute_add_simple_value_engine(aitp, value);
    return avtp ? 0 : -1;
}

static int
attribute_set_simple_value (attribute_instance_t *aitp,
    long long int value)
{
    attribute_value_t *avtp;

    avtp = attribute_add_simple_value_engine(aitp, value);
    if (avtp) {
        destroy_all_attribute_values_except(aitp, avtp);
        return 0;
    }
    return -1;
}

static int
attribute_delete_simple_value (attribute_instance_t *aitp, 
        long long int value)
{
    attribute_value_t *avtp, *prev_avtp;
    object_t *obj;

    if (NULL == aitp) return -1;
    obj = aitp->object;
    avtp = find_simple_attribute_value(aitp, value, &prev_avtp);
    if (avtp) {
        remove_attribute_value_from_list(aitp, avtp, prev_avtp);
        database_announce_event(obj->obj_db, ATTRIBUTE_VALUE_DELETED, obj,
            NULL, aitp->attribute_id, avtp);
        MEM_MONITOR_FREE(obj->obj_db, avtp);
        return 0;
    }
    return -1;
}

/******************************************************************************/

static attribute_value_t *
attribute_add_complex_value_engine (attribute_instance_t *aitp, 
        byte *complex_value_data, int complex_value_data_length)
{
    attribute_value_t *avtp;
    object_t *obj;

    if (NULL == aitp) return NULL;
    obj = aitp->object;

    /* value already there ? */
    avtp = find_complex_attribute_value(aitp,
                complex_value_data, complex_value_data_length, NULL);
    if (avtp) return avtp;

    /* not there, add it */
    avtp = create_complex_attribute_value(obj->obj_db->mem_mon_p,
                complex_value_data, complex_value_data_length);
    if (avtp) {
        add_attribute_value_to_list(aitp, avtp);
        database_announce_event(obj->obj_db, ATTRIBUTE_VALUE_ADDED, obj,
            NULL, aitp->attribute_id, avtp);
    }
    return avtp;
}

static int
attribute_add_complex_value (attribute_instance_t *aitp,
        byte *complex_value_data, int complex_value_data_length)
{
    attribute_value_t *avtp;

    avtp = attribute_add_complex_value_engine(aitp,
                complex_value_data, complex_value_data_length);
    return avtp ? 0 : -1;
}

static int
attribute_set_complex_value (attribute_instance_t *aitp,
    byte *complex_value_data, int complex_value_data_length)
{
    attribute_value_t *avtp;

    avtp = attribute_add_complex_value_engine(aitp,
                complex_value_data, complex_value_data_length);
    if (avtp) {
        destroy_all_attribute_values_except(aitp, avtp);
        return 0;
    }
    return -1;
}

static int
attribute_delete_complex_value (attribute_instance_t *aitp, 
        byte *complex_value_data, int complex_value_data_length)
{
    attribute_value_t *avtp, *prev_avtp;
    object_t *obj;

    if (NULL == aitp) return -1;
    obj = aitp->object;
    avtp = find_complex_attribute_value(aitp,
                complex_value_data, complex_value_data_length, &prev_avtp);
    if (avtp) {
        remove_attribute_value_from_list(aitp, avtp, prev_avtp);
        database_announce_event(obj->obj_db, ATTRIBUTE_VALUE_DELETED, obj,
            NULL, aitp->attribute_id, avtp);
        MEM_MONITOR_FREE(obj->obj_db, avtp);
        return 0;
    }
    return -1;
}

static void
attribute_instance_destroy (attribute_instance_t *aitp)
{
    object_t *obj = aitp->object;
    object_database_t *obj_db = obj->obj_db;
    void *removed_data;
    int events;

    events = allow_event_if_not_already_blocked(obj_db, 
                ATTRIBUTE_INSTANCE_DELETED);

    /* delete all its values first */
    destroy_all_attribute_values_except(aitp, NULL);

    /* now delete the instance itself */
#ifdef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES
    dynamic_array_remove(&obj->attributes, aitp->attribute_id, &removed_data);
#else
    table_remove(&obj->attributes, aitp, &removed_data);
#endif
    assert(aitp == removed_data);

    database_announce_event(obj->obj_db, ATTRIBUTE_INSTANCE_DELETED, 
            obj, NULL, aitp->attribute_id, NULL);

    /* ok event is processed, now restore back */
    restore_events(obj_db, events);

    MEM_MONITOR_FREE(obj_db, aitp);
}

static void
object_indexes_init (mem_monitor_t *memp, object_t *obj)
{
    assert(0 == table_init(&obj->children, 
                    0, compare_objects, memp,
                    use_avl_tree_for_object_children));

#ifdef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES

    assert(0 == dynamic_array_init(&obj->attributes,
                    0, 4, memp));

#else /* !USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES */

    assert(0 == table_init(&obj->attributes,
                    0, compare_attribute_ids, memp,
                    use_avl_tree_for_attribute_ids));

#endif /* !USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES */
}

static int
object_attributes_delete_all (object_t *obj)
{
    int count, i;
    attribute_instance_t **all_attributes;

    all_attributes = (attribute_instance_t**) 
#ifdef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES
        dynamic_array_get_all(&obj->attributes, &count);
#else
        table_get_all(&obj->attributes, &count);
#endif
    if (all_attributes && count) {
        for (i = 0; i < count; i++) {
            attribute_instance_destroy(all_attributes[i]);
        }
        free(all_attributes);
    }
    return count;
}

/*
 * The 'leave_parent_consistent' is used for something very subtle.
 *
 * when we destroy an object, we must leave the parent's
 * children in a consistent state.... except when we know
 * we are destroying a whole bunch of sub children in a
 * big swoop. Since these objects will all be destroyed,
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
object_destroy_engine (object_t *obj, int leave_parent_consistent)
{
    void *removed_obj;
    void **all_its_children;
    object_database_t *obj_db = obj->obj_db;
    object_t *parent;
    int child_count, i;
    int events;

    /*
     * stop all lower level AND child object destruction events
     * from being advertised.  That would cause too much noise.
     * Only 'this object' destroyed event is sufficient.
     */ 
    events = allow_event_if_not_already_blocked(obj_db, 0);

    parent = get_parent_pointer(obj);

    /*
     * make sure it is taken out of the parent's children list
     * but only if full & consistent cleanup is required.
     */ 
    if (leave_parent_consistent && parent) {
        table_remove(&parent->children, obj, &removed_obj);
    }

    /*
     * destroy all its children recursively.  Since *THIS* object is
     * also being destroyed, all its children can be destroyed without
     * having to keep their parent/child relationship consistent,
     * hence passing '0' to the function below.
     */ 
    all_its_children = table_get_all(&obj->children, &child_count);
    if (child_count && all_its_children) {
        for (i = 0; i < child_count; i++) {
            object_destroy_engine((object_t*) all_its_children[i], 0);
        }
        free(all_its_children);
    }

    /* free up all its attribute storage */
    object_attributes_delete_all(obj);
    
    /*
     * if this was the root object, it got cleaned; report
     * the event and nothing more to do.  Root object is
     * static and should never be deleted.
     */ 
    if (object_is_root(obj)) {
        restore_events(obj_db, events);
        database_announce_event(obj_db, OBJECT_DESTROYED, obj, NULL, 0, NULL);
        return;
    }

    /* take object out of the main object index */
    assert(0 == table_remove(&obj_db->object_index, obj, &removed_obj));
    assert(removed_obj == obj);

    /* free up its index objects */
    table_destroy(&obj->children);
#ifdef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES
    dynamic_array_destroy(&obj->attributes);
#else
    table_destroy(&obj->attributes);
#endif

    /* finally notify its destruction */
    restore_events(obj_db, events);
    database_announce_event(obj_db, OBJECT_DESTROYED, obj, NULL, 0, NULL);

    /* and blow it away */
    MEM_MONITOR_FREE(obj_db, obj);
}

static object_t *
object_create_engine (object_database_t *obj_db,
        int parent_must_exist,
        int parent_object_type, int parent_object_instance,
        int child_object_type, int child_object_instance)
{
    object_t *obj, *parent;
    void *exists;

    /* 
     * Obtain parent pointer.
     *
     * Note that it IS possible when loading from the database,
     * that the parent object has not yet been created.  In that
     * case, simply store the type & instance of the parent rather
     * than a direct pointer to it.  This will get resolved later.
     * This is controlled by 'parent_must_exist'.
     */
    parent = get_object_pointer(obj_db,
                    parent_object_type, parent_object_instance);
    if (parent_must_exist && (NULL == parent)) {
        return NULL;
    }

    /* allocate the object & fill in some basics */
    obj = MEM_MONITOR_ALLOC(obj_db, sizeof(object_t));
    assert(obj != NULL);
    obj->obj_db = obj_db;
    if (parent) {
        obj->parent.is_pointer = 1;
        obj->parent.u.object_ptr = parent;
    } else {
        obj->parent.is_pointer = 0;
        obj->parent.u.object_id.object_type = parent_object_type;
        obj->parent.u.object_id.object_instance = parent_object_instance;
    }
    obj->object_type = child_object_type;
    obj->object_instance = child_object_instance;

    /*
     * if the object already exists simply return that.
     */
    assert(0 == table_insert(&obj_db->object_index, obj, &exists));
    if (exists) {

        /* it already exists, we dont need the new one we just created */
        MEM_MONITOR_FREE(obj_db, obj);

        return exists;
    }

    /* make sure this object is added as a child of the parent */
    if (parent) {
        assert(0 == table_insert(&parent->children, obj, &exists));
    }

    /* initialize the children and attribute indexes */
    object_indexes_init(obj_db->mem_mon_p, obj);

    /*
     * send out the creation event but only if the parent exists.
     * If it does not, it means we are just loading from the 
     * database and some parents have not yet been resolved.
     * In this case, wait now since the 2nd pass of the database
     * parent resolving phase will generate the events as the
     * parent resolving completes.
     */
    if (parent) {
        database_announce_event(obj_db, OBJECT_CREATED, obj, parent, 0, NULL);
    }

    /* done */
    return obj;
}

static attribute_instance_t *
attribute_instance_add (object_t *obj, int attribute_id)
{
    attribute_instance_t *aitp;
    void *found;

#ifdef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES

    /* if already there, just return the existing one */
    if (dynamic_array_get(&obj->attributes, attribute_id, &found) == 0) {
        return found;
    }
    
    /* create the new attribute */
    aitp = MEM_MONITOR_ALLOC(obj->obj_db, sizeof(attribute_instance_t));
    if (NULL == aitp) return NULL;

    /* add it to object */
    aitp->attribute_id = attribute_id;
    if (dynamic_array_insert(&obj->attributes, attribute_id, aitp) != 0) {
        MEM_MONITOR_FREE(obj->obj_db, aitp);
        return NULL;
    }

#else

    /* create the new attribute  */
    aitp = MEM_MONITOR_ALLOC(obj->obj_db, sizeof(attribute_instance_t));
    if (NULL == aitp) return NULL;

    /* if already there, just free up the new one & return */
    aitp->attribute_id = attribute_id;
    if (table_insert(&obj->attributes, aitp, &found) == 0) {
        if (found) {
            MEM_MONITOR_FREE(obj->obj_db, aitp);
            return found;
        }
    }

#endif

    /* initialize rest of it */
    aitp->object = obj;
    aitp->n_attribute_values = 0;
    aitp->avps = NULL;

    /* notify */
    database_announce_event(obj->obj_db, ATTRIBUTE_INSTANCE_ADDED, 
        obj, NULL, aitp->attribute_id, NULL);

    /* done */
    return aitp;
}

static int
__object_attribute_add_simple_value (object_t *obj, int attribute_id, 
    long long int simple_value)
{
    attribute_instance_t *aitp;

    aitp = get_attribute_instance_pointer(obj, attribute_id);
    if (NULL == aitp) {
        aitp = attribute_instance_add(obj, attribute_id);
    }
    return
        attribute_add_simple_value(aitp, simple_value);
}

static int
__object_attribute_set_simple_value (object_t *obj, int attribute_id, 
    long long int simple_value)
{
    attribute_instance_t *aitp;

    aitp = get_attribute_instance_pointer(obj, attribute_id);
    if (NULL == aitp) {
        aitp = attribute_instance_add(obj, attribute_id);
    }
    return
        attribute_set_simple_value(aitp, simple_value);
}

static int
__object_attribute_delete_simple_value (object_t *obj, int attribute_id, 
    long long int simple_value)
{
    attribute_instance_t *aitp;

    aitp = get_attribute_instance_pointer(obj, attribute_id);
    return 
        attribute_delete_simple_value(aitp, simple_value);
}

static int
__object_attribute_add_complex_value (object_t *obj, int attribute_id, 
    byte *complex_value_data, int complex_value_data_length)
{
    attribute_instance_t *aitp;

    aitp = get_attribute_instance_pointer(obj, attribute_id);
    if (NULL == aitp) {
        aitp = attribute_instance_add(obj, attribute_id);
    }
    return
        attribute_add_complex_value(aitp,
                complex_value_data, complex_value_data_length);
}

static int
__object_attribute_set_complex_value (object_t *obj, int attribute_id, 
    byte *complex_value_data, int complex_value_data_length)
{
    attribute_instance_t *aitp;

    aitp = get_attribute_instance_pointer(obj, attribute_id);
    if (NULL == aitp) {
        aitp = attribute_instance_add(obj, attribute_id);
    }
    return
        attribute_set_complex_value(aitp,
                complex_value_data, complex_value_data_length);
}

static int
__object_attribute_delete_complex_value (object_t *obj, int attribute_id, 
    byte *complex_value_data, int complex_value_data_length)
{
    attribute_instance_t *aitp;

    aitp = get_attribute_instance_pointer(obj, attribute_id);
    return
        attribute_delete_complex_value(aitp,
                complex_value_data, complex_value_data_length);
}

static int
__object_attribute_instance_destroy (object_t *obj, int attribute_id)
{
    attribute_instance_t *found;

    found = get_attribute_instance_pointer(obj, attribute_id);
    if (found) {
        attribute_instance_destroy(found);
        return 0;
    }
    return ENODATA;
}

static int
__object_get_matching_children (object_t *parent,
        int matching_object_type, 
        object_representation_t *found_objects, int limit)
{
    int index = 0;
    void *v_object_type = integer2pointer(matching_object_type);
    void *v_limit = integer2pointer(limit);

    table_traverse(&parent->children, 
        get_matching_children_tfn,
        v_object_type, found_objects, &index, v_limit);

    return index;
}

static int
__object_get_matching_descendants (object_t *parent,
        int matching_object_type,
        object_representation_t *found_objects, int limit)
{
    int index = 0;
    void *v_object_type = integer2pointer(matching_object_type);
    void *v_limit = integer2pointer(limit);

    table_traverse(&parent->children, 
        get_matching_descendants_tfn,
        v_object_type, found_objects, &index, v_limit);

    return index;
}

#if 0

/******************************************************************************
 *
 * event management related functions
 *
 */

static void
get_both_objects (object_database_t *obj_db,
    event_record_t *evrp,
    object_t **obj, object_t **related_obj)
{
    if (obj) {
        *obj = get_object_pointer(obj_db, 
                    evrp->object_type, evrp->object_instance);
    }
    if (related_obj) {
        *related_obj = get_object_pointer(obj_db,
                            evrp->related_object_type,
                            evrp->related_object_instance);
    }
}

static int
process_object_created_event (object_database_t *obj_db,
    event_record_t *evrp)
{
    object_t *obj;

    obj = object_create_engine(obj_db, 1,
                evrp->related_object_type, evrp->related_object_instance,
                evrp->object_type, evrp->object_instance);
    return obj ? 0 : -1;
}

static int
process_object_destroyed_event (object_database_t *obj_db,
    event_record_t *evrp)
{
    object_t *obj;

    obj = get_object_pointer(obj_db, 
                evrp->object_type, evrp->object_instance);
    if (obj) {
        object_destroy_engine(obj, 1);
    }
    return 0;
}

static int
process_attribute_added_event (object_database_t *obj_db,
    event_record_t *evrp)
{
    object_t *obj;
    attribute_instance_t *aitp;

    get_both_objects(obj_db, evrp, &obj, NULL);
    if (obj) {
        aitp = attribute_instance_add(obj, evrp->attribute_id);
        return aitp ? 0 : -1;
    }
    return -1;
}

static int
process_attribute_deleted_event (object_database_t *obj_db,
    event_record_t *evrp)
{
    object_t *obj;

    get_both_objects(obj_db, evrp, &obj, NULL);
    if (obj) {
        return
            __object_attribute_instance_destroy(obj, evrp->attribute_id);
    }
    return -1;
}

static int
process_attribute_value_added_event (object_database_t *obj_db,
    event_record_t *evrp)
{
    object_t *obj;

    get_both_objects(obj_db, evrp, &obj, NULL);
    if (NULL == obj)
        return -1;
    if (evrp->attribute_value_length == 0) {
        return
            __object_attribute_add_simple_value(obj, evrp->attribute_id,
                    evrp->attribute_value_data);
    } else if (evrp->attribute_value_length > 0) {
        return
            __object_attribute_add_complex_value(obj, evrp->attribute_id,
                    (byte*) &evrp->attribute_value_data, 
                    evrp->attribute_value_length);
    }
    return -1;
}

static int
process_attribute_value_deleted_event (object_database_t *obj_db,
    event_record_t *evrp)
{
    object_t *obj;

    get_both_objects(obj_db, evrp, &obj, NULL);
    if (NULL == obj)
        return -1;
    if (evrp->attribute_value_length == 0) {
        return
            __object_attribute_delete_simple_value(obj, evrp->attribute_id,
                    evrp->attribute_value_data);
    } else if (evrp->attribute_value_length > 0) {
        return
            __object_attribute_delete_complex_value(obj, evrp->attribute_id,
                    (byte*) &evrp->attribute_value_data, 
                    evrp->attribute_value_length);
    }
    return -1;
}

#endif /* 0 */

/*
 * maximum tolarable times a read or a write call can consecutively fail
 */
#define MAX_CONSECUTIVE_FAILURES_ALLOWED        64

/*
 * makes sure it reads/writes all the requested size.  It will not
 * give up until it completes every single byte, unless too many 
 * consecutive number of read/write errors have occured.
 *
 * If reading & writing into a pipe or a socket, 'can_block' should
 * be specified as 'true/1'.
 */
static int 
relentless_read_write (int perform_read,
        int fd, void *buffer, int size, int can_block)
{
    int total, rc, failed;

    total = failed = 0;
    while (size > 0) {

        /* read/write maximum requested amount if it can */
        if (perform_read) {
            rc = read(fd, &((char*) buffer)[total], size);
        } else {
            rc = write(fd, &((char*) buffer)[total], size);
        }

        /* process the read/written amount */
        if (rc > 0) {
            total += rc;
            size -= rc;
            failed = 0;
            continue;
        }

        /* operation failed; has it failed consecutively many times ? */
        if (++failed >= MAX_CONSECUTIVE_FAILURES_ALLOWED) {
            return -1;
        }

        /*
         * these errors may occur rarely and should be recovered from,
         * unless they keep on happening consecutively
         */
        if ((can_block && (EWOULDBLOCK == errno)) ||
            (EINTR == errno) || (EAGAIN == errno)) {
                continue;
        }

        /* an unacceptable error occured, cannot continue */
        return -1;
    }

    /* everything finished ok */
    return 0;
}

static int
read_exact_size (int fd, void *buffer, int size, int can_block)
{
    return
        relentless_read_write(1, fd, buffer, size, can_block);
}

/*
 * read an event record which arrived from a remote database.
 * the space is assumed to be really big so we do not have to worry
 * about limits...
 */
int
read_event_record (int fd, event_record_t *evrp)
{
    int to_read = sizeof(event_record_t);
    int extra_length;

    /* read the basic event record information */
    if (read_exact_size(fd, evrp, to_read, 1) == 0) {
        extra_length = evrp->total_length - to_read;
        if (extra_length > 0) {
            return
                read_exact_size(fd, &evrp->extra_data, extra_length, 1);
        }
        return 0;
    }
    return -1;
}

static event_record_t *
create_event_record (object_database_t *obj_db,
        int event,
        object_t *obj, object_t *obj_related,
        int attribute_id, attribute_value_t *related_attribute_value)
{
    event_record_t *evrp;
    int size;

    /* this is the 'normal' size */
    size = sizeof(event_record_t);

    /*
     * if a complex attribute event, the size may be extended past
     * the end of the structure to account for a complex attribute
     */
    if (is_an_attribute_value_event(event) && related_attribute_value) {
        if (related_attribute_value->attribute_value_length > 0) {
            size += related_attribute_value->attribute_value_length;

            /* first 8 bytes of a complex attribute will fit here */
            size -= sizeof(long long int);
        }
    }

    /* now we have the appropriate size, we can create the event record */
    evrp = malloc(size);
    if (NULL == evrp) return NULL;

    /* set essentials */
    evrp->total_length = size;
    evrp->database_id = obj_db->database_id;
    evrp->event_type = event;
    evrp->attribute_id = attribute_id;

    /* set main object if specified */
    if (obj) {
        evrp->object_type = obj->object_type;
        evrp->object_instance = obj->object_instance;
    }

    /* set related object if specified */
    if (obj_related) {
        evrp->related_object_type = obj_related->object_type;
        evrp->related_object_instance = obj_related->object_instance;
    }

    /* set attribute value if specified */
    if (related_attribute_value) {

        /* copy basic attribute */
        evrp->attribute_value_length = 
            related_attribute_value->attribute_value_length;
        evrp->attribute_value_data = 
            related_attribute_value->attribute_value_data;

        /* overwrite if it was a complex attribute */
        if (evrp->attribute_value_length > 0) {
            memcpy((void*) &evrp->attribute_value_data,
                (void*) &related_attribute_value->attribute_value_data,
                evrp->attribute_value_length);
        }
    }

    return evrp;
}

static int
database_announce_event (object_database_t *obj_db,
    int event,
    object_t *obj, object_t *obj_related, 
    int attribute_id, attribute_value_t *related_attribute_value)
{
    /*
     * we cannot use a local variable here since we do not know the 
     * final size of what the event record should be.  So, we malloc it.
     */
    event_record_t *evrp;

    evrp = create_event_record(obj_db, event, obj, obj_related,
                attribute_id, related_attribute_value);
    if (NULL == evrp) return ENOMEM;

    /* tell event manager to distribute it */
    announce_event(&obj_db->evm, evrp);

    /* we are done */
    free(evrp);

    return 0;
}

/*************** Public functions *********************************************/

PUBLIC int
database_initialize (object_database_t *obj_db,
        int make_it_thread_safe,
        int database_id,
        mem_monitor_t *parent_mem_monitor)
{
    object_t *root_obj;

    memset(obj_db, 0, sizeof(object_database_t));

    MEM_MONITOR_SETUP(obj_db);
    LOCK_SETUP(obj_db);

    /* memset to 0 already does this but just making a point */
    obj_db->processing_remote_event = 0;
    obj_db->blocked_events = 0;

    root_obj = &obj_db->root_object;
    root_obj->obj_db = obj_db;

    root_obj->parent.is_pointer = 1;
    root_obj->parent.u.object_ptr = NULL;

    root_obj->object_type = ROOT_OBJECT_TYPE;
    root_obj->object_instance = ROOT_OBJECT_INSTANCE;

    /* initialize the event manager for this database */
    assert(0 == event_manager_init(&obj_db->evm, 0, obj_db->mem_mon_p));

    /* initialize object lookup indexes */
    assert(0 == table_init(&obj_db->object_index,
                    0, 
                    compare_objects,
                    obj_db->mem_mon_p,
                    use_avl_tree_for_database_object_index));

    obj_db->database_id = database_id;

    /* initialize root object indexes */
    object_indexes_init(obj_db->mem_mon_p, root_obj);

    WRITE_UNLOCK(obj_db);

    /* done */
    return 0;
}

PUBLIC int
object_create (object_database_t *obj_db,
        int parent_object_type, int parent_object_instance,
        int child_object_type, int child_object_instance)
{
    int failed;
    object_t *obj;

    WRITE_LOCK(obj_db);
    obj = object_create_engine(obj_db, 1,
                parent_object_type, parent_object_instance,
                child_object_type, child_object_instance);
    failed = (obj ? 0 : EFAULT);
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_exists (object_database_t *obj_db,
        int object_type, int object_instance)
{
    int failed;

    READ_LOCK(obj_db);
    failed = (NULL != get_object_pointer(obj_db, object_type, object_instance));
    READ_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_attribute_add (object_database_t *obj_db,
        int object_type, int object_instance, int attribute_id)
{
    int failed;
    object_t *obj;
    attribute_instance_t *aitp;

    WRITE_LOCK(obj_db);
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        aitp = attribute_instance_add(obj, attribute_id);
        failed = (aitp ? 0 : ENOMEM);
    }
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_attribute_add_simple_value (object_database_t *obj_db,
        int object_type, int object_instance, int attribute_id,
        long long int simple_value)
{
    int failed;
    object_t *obj;

    WRITE_LOCK(obj_db);
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = __object_attribute_add_simple_value(obj,
                    attribute_id, simple_value);
    }
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_attribute_set_simple_value (object_database_t *obj_db,
        int object_type, int object_instance, int attribute_id,
        long long int simple_value)
{
    int failed;
    object_t *obj;

    WRITE_LOCK(obj_db);
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = __object_attribute_set_simple_value(obj,
                    attribute_id, simple_value);
    }
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_attribute_delete_simple_value (object_database_t *obj_db,
        int object_type, int object_instance, int attribute_id,
        long long int simple_value)
{
    int failed;
    object_t *obj;

    WRITE_LOCK(obj_db);
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = __object_attribute_delete_simple_value(obj,
                    attribute_id, simple_value);
    }
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_attribute_add_complex_value (object_database_t *obj_db,
        int object_type, int object_instance, int attribute_id,
        byte *complex_value_data, int complex_value_data_length)
{
    int failed;
    object_t *obj;

    WRITE_LOCK(obj_db);
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = __object_attribute_add_complex_value(obj, attribute_id,
                    complex_value_data, complex_value_data_length);
    }
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_attribute_set_complex_value (object_database_t *obj_db,
        int object_type, int object_instance, int attribute_id,
        byte *complex_value_data, int complex_value_data_length)
{
    int failed;
    object_t *obj;

    WRITE_LOCK(obj_db);
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = __object_attribute_set_complex_value(obj, attribute_id,
                    complex_value_data, complex_value_data_length);
    }
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_attribute_delete_complex_value (object_database_t *obj_db,
        int object_type, int object_instance, int attribute_id,
        byte *complex_value_data, int complex_value_data_length)
{
    int failed;
    object_t *obj;

    WRITE_LOCK(obj_db);
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = __object_attribute_delete_complex_value(obj, attribute_id,
                    complex_value_data, complex_value_data_length);
    }
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_attribute_get_value (object_database_t *obj_db, 
    int object_type, int object_instance,
    int attribute_id, int nth,
    attribute_value_t **cloned_avtp)
{
    object_t *obj;
    attribute_instance_t *aitp;
    attribute_value_t *avtp;
    int i;

    READ_LOCK(obj_db);

    /* assume failure */
    *cloned_avtp = NULL;

    /* get object */
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        READ_UNLOCK(obj_db);
        return ENODATA;
    }

    /* get attribute */
    aitp = get_attribute_instance_pointer(obj, attribute_id);
    if (NULL == aitp) {
        READ_UNLOCK(obj_db);
        return ENODATA;
    }

    /* trim index to sane limits */
    if (nth < 0)  {
        nth = 0;
    } else if (nth >= aitp->n_attribute_values) {
        nth = aitp->n_attribute_values - 1;
    }

    /* find the n'th attribute value now */
    avtp = aitp->avps;
    for (i = 0; ((i < nth) && avtp); i++) {
        avtp = avtp->next_attribute_value;
    }
    if (avtp) {
        *cloned_avtp = clone_attribute_value(avtp);
        READ_UNLOCK(obj_db);
        return 0;
    }

    READ_UNLOCK(obj_db);
    return EFAULT;
}

PUBLIC int
object_attribute_get_all_values (object_database_t *obj_db,
        int object_type, int object_instance, int attribute_id,
        int *how_many, attribute_value_t *returned_attribute_values[])
{
    READ_LOCK(obj_db);
    READ_UNLOCK(obj_db);
    return 0;
}

PUBLIC int
object_attribute_destroy (object_database_t *obj_db,
        int object_type, int object_instance, int attribute_id)
{
    int failed;
    object_t *obj;

    WRITE_LOCK(obj_db);
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = __object_attribute_instance_destroy(obj, attribute_id);
    }
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC int
object_destroy (object_database_t *obj_db,
        int object_type, int object_instance)
{
    int failed;
    object_t *obj;

    WRITE_LOCK(obj_db);
    obj = get_object_pointer(obj_db, object_type, object_instance);
    if (NULL == obj) {
        failed = ENODATA;
    } else {
        failed = 0;
        object_destroy_engine(obj, 1);
    }
    WRITE_UNLOCK(obj_db);
    return failed;
}

PUBLIC object_representation_t *
object_get_matching_children (object_database_t *obj_db,
        int parent_object_type, int parent_object_instance,
        int matching_object_type, int *returned_count)
{
    int size;
    object_t *root;
    object_representation_t *found_objects;

    READ_LOCK(obj_db);
    *returned_count = 0;
    found_objects = NULL;
    root = get_object_pointer(obj_db,
                parent_object_type, parent_object_instance);
    if (root) {
        size = table_member_count(&root->children);
        found_objects = malloc((size + 1) * sizeof(object_representation_t));
        if (found_objects) {
            __object_get_matching_children(root, 
                    matching_object_type, found_objects, size+1);
            *returned_count = size;
        }
    }
    READ_UNLOCK(obj_db);
    return found_objects;
}

PUBLIC object_representation_t *
object_get_children (object_database_t *obj_db,
        int parent_object_type, int parent_object_instance,
        int *returned_count)
{
    return
        object_get_matching_children(obj_db,
            parent_object_type, parent_object_instance,
            ALL_OBJECT_TYPES, returned_count);
}

PUBLIC object_representation_t *
object_get_matching_descendants (object_database_t *obj_db,
        int parent_object_type, int parent_object_instance,
        int matching_object_type, int *returned_count)
{
    int size;
    object_t *root;
    object_representation_t *found_objects;

    READ_LOCK(obj_db);
    *returned_count = 0;
    found_objects = NULL;
    root = get_object_pointer(obj_db,
                parent_object_type, parent_object_instance);
    if (root) {

        /*
         * We cannot guess in advance how many of these will be so
         * allocate the full size of the database, just in case.
         */
        size = table_member_count(&obj_db->object_index);

        found_objects = malloc((size + 1) * sizeof(object_representation_t));
        if (found_objects) {
            __object_get_matching_descendants(root, 
                    matching_object_type, found_objects, size+1);
            *returned_count = size;
        }
    }
    READ_UNLOCK(obj_db);
    return found_objects;
}

PUBLIC object_representation_t *
object_get_descendants (object_database_t *obj_db,
        int parent_object_type, int parent_object_instance,
        int *returned_count)
{
    return
        object_get_matching_descendants
            (obj_db, parent_object_type, parent_object_instance,
             ALL_OBJECT_TYPES, returned_count);
}

PUBLIC void
database_destroy (object_database_t *obj_db)
{
    WRITE_LOCK(obj_db);

    event_manager_destroy(&obj_db->evm);

    /*
     * we cannot unlock since the lock
     * will also have been destroyed.
     *
     * WRITE_UNLOCK(obj_db);
     *
     */
}

/******************************************************************************
 *
 * Reading and writing the database from/to a file for permanency.
 */

/*
 * acronyms used in the database file
 */
static char *object_acronym = "OBJ";
static char *attribute_id_acronym = "AID";
static char *complex_attribute_value_acronym = "CAV";
static char *simple_attribute_value_acronym = "SAV";

/*************** Writing the database to a file functions *********************/

static int
database_write_one_attribute_value (FILE *fp, attribute_value_t *avtp)
{
    int i;
    byte *bptr;

    /* simple attribute value */
    if (avtp->attribute_value_length == 0) {
        fprintf(fp, "\n    %s %lld ",
            simple_attribute_value_acronym, avtp->attribute_value_data);
        return 0;
    }

    /* complex attribute value */
    fprintf(fp, "\n    %s %d ",
        complex_attribute_value_acronym, avtp->attribute_value_length);
    bptr = (byte*) &avtp->attribute_value_data;
    for (i = 0; i < avtp->attribute_value_length; i++) {
        fprintf(fp, "%d ", *bptr);
        bptr++;
    }

    return 0;
}

static int
database_write_one_attribute (FILE *fp, void *vattr)
{
    attribute_instance_t *attr = (attribute_instance_t*) vattr;
    attribute_value_t *avtp;

    fprintf(fp, "\n  %s %d ", attribute_id_acronym, attr->attribute_id);
    avtp = attr->avps;
    while (avtp) {
        database_write_one_attribute_value(fp, avtp);
        avtp = avtp->next_attribute_value;
    }
    return 0;
}

static int
database_write_one_object_tfn (void *utility_object, void *utility_node,
        void *v_object, void *v_FILE, 
        void *p1, void *p2, void *p3)
{
    object_t *obj = (object_t*) v_object;
    FILE *fp = v_FILE;
    void **attributes = NULL;
    int attr_count = 0;

    /* ignore root object, we do not store this */
    if (obj->object_type == ROOT_OBJECT_TYPE) return 0;

    fprintf(fp, "\n%s %d %d %d %d",
        object_acronym, 
        obj->parent.u.object_ptr->object_type,
        obj->parent.u.object_ptr->object_instance,
        obj->object_type, obj->object_instance);

#ifdef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES
    attributes = dynamic_array_get_all(&obj->attributes, &attr_count);
#else
    attributes = table_get_all(&obj->attributes, &attr_count);
#endif
    if (attributes && attr_count) {
        while (--attr_count >= 0) {
            database_write_one_attribute(fp, attributes[attr_count]);
        }
        free(attributes);
    }
    
    return 0;
}

/*
 * Write the database out to disk.
 *
 * THIS IS *** VERY *** IMPORTANT SO UNDERSTAND IT WELL.
 *
 * We have to ensure that the database is written out in a way such
 * that when we recreate it reading back from the file, the parent 
 * of an object to be created must already have been written out 
 * earlier such that when we create an object, its parent
 * already exists and we can associate the two objects.  Now, to be
 * able to do that, we have to start recursively writing from root
 * object down, first writing out the object and its children later.
 * This way since children are written out later, the parent object has
 * already been created.  So, writing the database to a file involves
 * writing the current object out, followed recusively writing out
 * all its children.
 *
 * This is fine and dandy with ONE HUGE PROBLEM.  For very deep databases,
 * we run out of recursion stack, no matter how hard I tried to extend 
 * the stack and we almost always crash.  
 *
 * So, unfortunately, this method of recursively writing objects followed
 * after by their children, although correct, cannot be used.
 *
 * Here is the alternative.  The 'object_index' in the database holds 
 * ALL the objects but in random order (not neatly parent followed 
 * by children as we want).  But since our tree traversal uses morris
 * traversals, it does not use any extra stack or queue or any kind
 * of memory.  This makes it PERFECT for extremely large databases
 * since we never run out of stack space.  But now, we introduce the
 * problem where we may have to create an object without having yet 
 * created its parent.  So, how do we solve this problem.  This actually
 * is not as difficult but it just needs a second pass over all the 
 * objects.
 *
 * In the first pass, we create the objects and if we happen to have 
 * their parent already available we can do the association immediately.
 * However, there will be many objects in which the parents are NOT
 * yet created.  In those cases, we store the parent object identifier
 * (in the form of type, instance) and move to the next object.
 *
 * Now in the second pass, since we have crerated ALL the objects, we simply
 * scan thru all the objects whose parents have not yet been resolved 
 * and associate them.  When the association is complete all objects
 * will have their parents represented as 'pointer' values.
 *
 * So, in the absolute worst case, we will make 2n object processing 
 * if there are n elements in the database.
 * Since both these passes use iterative methods (morris traverse),
 * we will never run out of stack, even for very large databases.
 * But we will consume more time.  Increasing execution time is a valid
 * solution but crashing due to stack overflow is not.
 */

PUBLIC int
database_store (object_database_t *obj_db)
{
    FILE *fp;
    char database_name [TYPICAL_NAME_SIZE];
    char backup_db_name [TYPICAL_NAME_SIZE];
    char backup_db_tmp [TYPICAL_NAME_SIZE];
    void *unused = NULL;    // shut the compiler up

    READ_LOCK(obj_db);

    sprintf(database_name, "database_%d", obj_db->database_id);
    sprintf(backup_db_name, "%s_BACKUP", database_name);
    sprintf(backup_db_tmp, "%s_tmp", backup_db_name);

    /* does not matter if these fail */
    unlink(backup_db_tmp);
    rename(backup_db_name, backup_db_tmp);
    rename(database_name, backup_db_name);

    fp = fopen(database_name, "w");
    if (NULL == fp) {
        READ_UNLOCK(obj_db);
        return -1;
    }
    table_traverse(&obj_db->object_index, database_write_one_object_tfn,
        fp, unused, unused, unused);

    /* close up the file */
    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);

#if 0 // error
    unlink(database_name);
    rename(backup_db_name, database_name);
    unlink(backup_db_tmp);
#endif
    READ_UNLOCK(obj_db);

    return 0;
}

/*************** Reading back from a file functions ***************************/

static int
load_object (object_database_t *obj_db, FILE *fp,
        object_t **objp)
{
    int count;
    int parent_type, parent_instance, object_type, object_instance;

    count = fscanf(fp, "%d %d %d %d",
                &parent_type, &parent_instance, 
                &object_type, &object_instance);
    if (4 != count) return -1;
    *objp = object_create_engine(obj_db, 0,
                parent_type, parent_instance,
                object_type, object_instance);
    return *objp ? 0 : -1;
}

static int
load_attribute_id (object_database_t *obj_db, FILE *fp,
    object_t *obj, attribute_instance_t **aitpp)
{
    int aid;

    /* we should NOT have a NULL object at this point */
    if (NULL == obj) return -1;

    if (fscanf(fp, "%d", &aid) != 1)
        return -1;
    *aitpp = attribute_instance_add(obj, aid);
    if (NULL == *aitpp)
        return -1;

    return 0;
}

static int
load_simple_attribute_value (object_database_t *obj_db, FILE *fp,
    attribute_instance_t *aitp)
{
    long long int value;

    /* we should NOT have a NULL attribute id at this point */
    if (NULL == aitp) return -1;

    if (fscanf(fp, "%lld", &value) != 1) return -1;
    return
        attribute_add_simple_value(aitp, value);

    return 0;
}

static int
load_complex_attribute_value (object_database_t *obj_db, FILE *fp,
    attribute_instance_t *aitp)
{
    byte *value;
    int i, len;
    int err;

    /* we should NOT have a NULL attribute id at this point */
    if (NULL == aitp) return -1;

    /* read length */
    if (fscanf(fp, "%d", &len) != 1) return -1;

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
    err = attribute_add_complex_value(aitp, value, len);

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
    void *user_data, void *v_obj_db,
    void *v_1, void *v_2, void *v_3)
{
    object_database_t *obj_db;
    object_t *obj, *parent;

    obj_db = (object_database_t*) v_obj_db;
    obj = (object_t*) user_data;
    if (!(obj->parent.is_pointer)) {
        parent = get_object_pointer(obj_db,
                    obj->parent.u.object_id.object_type,
                    obj->parent.u.object_id.object_instance);
        assert(NULL != parent);
        obj->parent.is_pointer = 1;
        obj->parent.u.object_ptr = parent;
    }
    return 0;
}

static int
database_resolve_all_parents (object_database_t *obj_db)
{
    void *unused = 0;

    table_traverse(&obj_db->object_index, resolve_parent_tfn, obj_db, 
        unused, unused, unused);
    return 0;
}

PUBLIC int
database_load (int database_id, object_database_t *obj_db)
{
    char database_name [TYPICAL_NAME_SIZE];
    FILE *fp;
    int failed, count;
    object_t *obj, *parent;
    attribute_instance_t *aitp;
    attribute_value_t *avtp;
    char string [TYPICAL_NAME_SIZE];

    sprintf(database_name, "database_%d", database_id);
    fp = fopen(database_name, "r");
    if (NULL == fp) return -1;

    //database_destroy(obj_db);
    if (database_initialize(obj_db, 1, database_id, NULL) != 0) {
        return -1;
    }

    obj = parent = NULL;
    aitp = NULL;
    avtp = NULL;
    failed = 0;

    while ((count = fscanf(fp, "%s", string)) != EOF) {

        /* skip empty lines */
        if (count != 1) continue;

        if (strcmp(string, object_acronym) == 0) {
            if (load_object(obj_db, fp, &obj) != 0) {
                failed = -1;
                break;
            }
        } else if (strcmp(string, attribute_id_acronym) == 0) {
            if (load_attribute_id(obj_db, fp, obj, &aitp) != 0) {
                failed = -1;
                break;
            }
        } else if (strcmp(string, simple_attribute_value_acronym) == 0) {
            if (load_simple_attribute_value(obj_db, fp, aitp) != 0) {
                failed = -1;
                break;
            }
        } else if (strcmp(string, complex_attribute_value_acronym) == 0) {
            if (load_complex_attribute_value(obj_db, fp, aitp) != 0) {
                failed = -1;
                break;
            }
        }
    }
    fclose(fp);

    /*
     * now perform a second pass over the database 
     * to resolve all un-resolved parent pointers
     */
    if (0 == failed) {
        return
            database_resolve_all_parents(obj_db);
    }

    return failed;
}

#ifdef __cplusplus
} // extern C
#endif 



