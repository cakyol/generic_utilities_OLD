
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
** @@@@@ GENERIC OBJECT DATABASE
**
**      This is a VERY fast and a VERY flexible hierarchical object manager.
**
**      Almost every data structure is either an avl tree or a binary
**      index object, making it very efficient.
**
**      An object always has ONE parent but can have MANY children.  
**      It is up to the user to make sure an object's child cannot 
**      also be a parent otherwise infinite loops will be created.
**
**      An object can have any number of attributes and these can be
**      added and/or deleted dynamically during an object's lifetime.
**
**      Each such object attribute also can have MANY values.  Those
**      are maintained in a random linked list.  This makes the attribute
**      value manipulation somewhat slower but typically, it is not expected
**      that an attribute will contain more than at most a few values.
**
**      An object is identified distinctly by its type & instance and hence
**      there can be only ONE instance of an object of the same type in one
**      manager.  All object types & instances MUST be > 0.  Values <= 0 are
**      reserved and should not be used.
**
**      Conversely, an attribute is also distinctly identified by an integer
**      of value > 0.  Do not use attribute ids of <= 0.
**
**      An object always has ONLY ONE parent.
**
**      An object can have 0 -> many children.
**
**      An object can have 0 -> many attributes.
**
**      An object can add/delete children at any time.
**
**      If an object is destroyed, all its children will also be destroyed.
**
**      Object attributes can be and are dynamically addable and deletable
**              from an object.  There is no limit.
**
**      Object attributes are multi valued.  An attribute can hold
**              multiple simple or complex values.  There is no
**              limit.
**
**              Note that there is a distinction between an 'attribute
**              value add' and an 'attribute value set'.  The 'set' will
**              delete all values and re-set the attribute value to
**              only those values specified in the call, whereas an
**              attribute 'add' is additive, it will actually add to
**              the already existing values.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __OBJECT_MANAGER_H__
#define __OBJECT_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"
#include "table.h"
#include "dynamic_array_object.h"
#include "object_types.h"
#include "event_manager.h"

/* 
 * This uses more memory for widespread attribute ids
 * but is extremely fast.  If attribute id numbers are
 * kept consecutive, then it would use less memory.
 */
#define USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES

#define TYPICAL_NAME_SIZE                       (64)

typedef struct attribute_value_s attribute_value_t;
typedef struct attribute_instance_s attribute_instance_t;
typedef struct object_identifier_s object_identifier_t;
typedef struct object_representation_s object_representation_t;
typedef struct object_s object_t;
typedef struct object_manager_s object_manager_t;

/******************************************************************************
 *
 * attribute related structures
 *
 */

/*
 * The structure below can be used for every possible kind of 
 * data type.  This includes all the basic types as well
 * as pointers to byte streams.  Every possible data is either a
 * 'simple' type (byte, char, short, int, int64) or complex
 * type (pointer to a byte stream and its length).
 *
 * One consequence of this method is that ALL simple data types,
 * regardless of their size, will always be stored as int64.
 * This is a bit wasteful but not so bad given the simplicity it 
 * provides.
 *
 * Here is the important differentiation points between 'simple'
 * and 'complex' attribute values:
 *
 * - an attribute value length should never be < 0.  This shows an
 *   an internal error.
 *
 * - if length == 0, it indicates a 'simple' attribute value type.
 *
 * - if length > 0, it represents a pointer to a byte stream
 *   whose length is as given.
 *
 * The way both types of data stored is a bit strange but very
 * convenient in the sense that only ONE malloc allocates all the
 * needed space.  Basically, for byte stream data, the data starts 
 * WITH the 'attribute_value_data'.  It 'becomes' part of the 
 * stream.  In this case, the first 8 bytes of the stream data
 * will always be stored in the 'attribute_value_data' as 'bytes'.
 *
 * Since an attribute can have multiple values, simple or complex,
 * there is a linked list implementation of values for each attribute
 * id.  Since it is not expected for a single attribute id to have 
 * very many values, this implementation is sufficient.  If however,
 * an attribute starts storing lots of values, performance will
 * start suffering.
 *
 */
struct attribute_value_s {

    attribute_value_t *next_attribute_value;
    int attribute_value_length;
    long long int attribute_value_data;
};

struct attribute_instance_s {

    object_t *object;           /* which object does this attribute belong to */
    int attribute_id;           /* which attribute is it */
    int n_attribute_values;     /* how many values does it have */
    attribute_value_t *avps;    /* linked list of the attribute values */
};

/******************************************************************************
 *
 * object related structures
 *
 */

/*
 * User facing APIs only deal with this representation of an object
 * since pointers are always hidden from the user and the only way
 * to address an object is thru this.
 */
struct object_identifier_s {
    int object_type;
    int object_instance;
};

/*
 * Internal APIs mostly use pointers since they are thread protected.
 * and pointers are safe to use.  So, this one type can be used both 
 * for user facing APIs as well as internal uses.
 */
struct object_representation_s {

    /* set if the representation is a pointer */
    byte is_pointer;

    /* which address type to use */
    union {
        object_identifier_t object_id;
        object_t *object_ptr;
    } u;
};

struct object_s {

    /* which object manager this object belongs to */
    object_manager_t *omp;

    /*
     * Unique identification of this object.  This combination is
     * always unique per object manager and distinctly identifies an object.
     * This combination is always the unique 'key'.
     */ 
    int object_type;
    int object_instance;

    /*
     * Parent of this object.  If object is root, it
     * has no parent.
     */ 
    object_representation_t parent;

    /* Can have many children; no limit */
    table_t children;

    /* All the attributes of this object */
#ifdef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES
    dynamic_array_t attributes;
#else 
    table_t attributes;
#endif

};

/******************************************************************************
 *
 * object manager related structures
 *
 */

struct object_manager_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    /* unique integer for this manager */
    int manager_id;

    /* event manager for this object manager */
    event_manager_t evm;

    /*
     * Objects are always uniquely indexed by 'object_type' & 
     * 'object_instance'.  There can be only ONE object of
     * this unique combination in every manager.
     */
    table_t object_index;

    /*
     * the root object; this is special, can NOT be deleted
     * This is used to store all the objects (children) of 
     * the manager.
     */
    object_t root_object;

    /*
     * There are two reasons for manager changes.  Ones produced locally
     * by direct function calls from the application itself, or ones that
     * happen because of processing events arriving from remote managers
     * to synchronise them all.
     *
     * In the case of local changes, this manager must send the change
     * to all other object managers so that they can be kept in sync.
     *
     * If on the other hand, an event was received from another manager,
     * after syncing this object manager to it, the event must be locally
     * advertised to the registered local event handler.
     *
     * When either case is happening, the other case must not be executed
     * or the object manager will start spinning and chase its own tail 
     * continuously.
     *
     * This variable controls this situation.
     */
    int processing_remote_event;

    /* for spurious event suppression */
    int blocked_events;
};

/************* User functions ************************************************/

extern int
om_initialize (object_manager_t *omp,
        int make_it_thread_safe,
        int manager_id,
        mem_monitor_t *parent_mem_monitor);

static inline int
om_register_for_object_events (object_manager_t *omp,
        int object_type,
        event_handler_t ehfp, void *extra_arg)
{
    return
        register_for_object_events(&omp->evm, object_type, ehfp, extra_arg);
}

static inline void
om_un_register_from_object_events (object_manager_t *omp,
        int object_type,
        event_handler_t ehfp, void *extra_arg)
{
    un_register_from_object_events(&omp->evm, object_type, ehfp, extra_arg);
}

static inline int
om_register_for_attribute_events (object_manager_t *omp,
        int object_type,
        event_handler_t ehfp, void *extra_arg)
{
    return
        register_for_attribute_events(&omp->evm, object_type, ehfp, extra_arg);
}

static inline void
om_un_register_from_attribute_events (object_manager_t *omp,
        int object_type,
        event_handler_t ehfp, void *extra_arg)
{
    un_register_from_attribute_events(&omp->evm, object_type, ehfp, extra_arg);
}

extern int
object_create (object_manager_t *omp,
        int parent_object_type, int parent_object_instance,
        int child_object_type, int child_object_instance);

extern int
object_exists (object_manager_t *omp,
        int object_type, int object_instance);

extern int
object_attribute_add (object_manager_t *omp,
        int object_type, int object_instance, int attribute_id);

extern int
object_attribute_add_simple_value (object_manager_t *omp,
        int object_type, int object_instance, int attribute_id,
        long long int simple_value);

extern int
object_attribute_set_simple_value (object_manager_t *omp,
        int object_type, int object_instance, int attribute_id,
        long long int simple_value);

extern int
object_attribute_delete_simple_value (object_manager_t *omp,
        int object_type, int object_instance, int attribute_id,
        long long int simple_value);

extern int
object_attribute_add_complex_value (object_manager_t *omp,
        int object_type, int object_instance, int attribute_id,
        byte *complex_value_data, int complex_value_data_length);

extern int
object_attribute_set_complex_value (object_manager_t *omp,
        int object_type, int object_instance, int attribute_id,
        byte *complex_value_data, int complex_value_data_length);

extern int
object_attribute_delete_complex_value (object_manager_t *omp,
        int object_type, int object_instance, int attribute_id,
        byte *complex_value_data, int complex_value_data_length);

extern int
object_attribute_get_value (object_manager_t *omp,
        int object_type, int object_instance, 
        int attribute_id, int nth,
        attribute_value_t **cloned_attribute_value);

extern int
object_attribute_get_all_values (object_manager_t *omp,
        int object_type, int object_instance, int attribute_id,
        int *how_many, attribute_value_t *returned_attribute_values[]);

extern int
object_attribute_destroy (object_manager_t *omp,
        int object_type, int object_instance, int attribute_id);

/*
 * Returns only the FIRST level of children objects MATCHING ONLY 
 * the specified type.
 * Returns in the form of object identification (type, instance).
 */
extern object_representation_t *
object_get_matching_children (object_manager_t *omp,
        int parent_object_type, int parent_object_instance,
        int matching_object_type, int *returned_count);

/*
 * Returns ALL of the FIRST level children objects.
 * Returns in the form of object identification (type, instance).
 */
extern object_representation_t *
object_get_children (object_manager_t *omp,
        int parent_object_type, int parent_object_instance,
        int *returned_count);

/*
 * NOT RECOMMENDED TO BE USED EXTERNALLY, INTERNAL USE ONLY.
 *
 * Returns ALL LEVELS of children objects MATCHING ONLY the specified type.
 * Returns in the form of object pointer.
 */
extern object_representation_t *
object_get_matching_descendants (object_manager_t *omp,
        int parent_object_type, int parent_object_instance,
        int matching_object_type, int *returned_count);

/*
 * NOT RECOMMENDED TO BE USED EXTERNALLY, INTERNAL USE ONLY.
 *
 * Returns ALL LEVELS of children of ALL objects of the parent.
 * Returns in the form of object pointer.
 */
extern object_representation_t *
object_get_descendants (object_manager_t *omp,
        int parent_object_type, int parent_object_instance,
        int *returned_count);

extern int
object_destroy (object_manager_t *omp,
        int object_type, int object_instance);

static inline int
om_object_count (object_manager_t *omp)
{ return table_member_count(&omp->object_index); }

/******************************************************************************
 *
 *  Object manager reading & writing into a file.
 *
 *  The object manager in a file conforms to the below format:
 *
 *  - The filename is formed by concatanating "om_" & id number,
 *    such as "om_29".
 *
 *  - One or more object records where each one is an object,
 *    its parent and all its attributes and attribute values.
 *
 *  OPTIONAL
 *  - Last entry is a checksum.  This stops the object manager file from being
 *    hand modified.
 *
 *  Everything is in readable text.  All numbers are in decimal.
 *  The format is one or more objects grouped like below and repeated
 *  as many times as there are objects, where each object is separated
 *  by blank lines to improve readability.  The line breaks are delimiters.
 *
 *  OBJ %d %d %d %d
 *      (parent type, instance, object type, instance)
 *  AID %d
 *      (attribute id)
 *  SAV %ll
 *      (simple attribute value)
 *  CAV %d %d %d %d %d %d ....
 *      (complex attribute value, first int is the length & the rest
 *       are all the data bytes)
 *
 *  Parsing of an object is very context oriented.  When a 'OBJ' line
 *  is parsed, it defines the object context to which all consecutive
 *  lines apply, until another 'OBJ' line is reached where the context
 *  changes and so on till the end of the file.
 *  When an 'AID' is parsed it defines an new attribute id for the current
 *  object in context.
 *  When a 'CAV' or 'SAV' is encountered the attribute value is defined
 *  for the current attribute id in context.
 */

/*
 * Writes out the object manager to a file.
 */
extern int
om_store (object_manager_t *omp);

/*
 * reads a object manager from a file
 */
extern int
om_load (int manager_id, object_manager_t *omp);

extern void
om_destroy (object_manager_t *omp);

#ifdef __cplusplus
} /* extern C */
#endif 

#endif /* __OBJECT_MANAGER_H__ */


