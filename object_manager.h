
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
** @@@@@ GENERIC OBJECT MANAGER
**
**      This is a hierarchical object manager.  It is very fast
**      to search an object and/or an attribute & value of an
**      object.  Inserting & deleting objects are somewaht a bit
**      slower.  It is more suited to create/modify a few times but
**      search many times type of processes.
**      
**      An object (as defined by its uniqueness) can only appear ONCE
**      in the manager.
**
**      An object does not need to have a parent but can have MANY children.  
**      This allows multiple dangling objects with or without children
**      to be in the object manager.  However, if an object is a child
**      of an object, then it MUST have a parent.  The object manager
**      enforces this logic.
**
**      It is up to the user to make sure an object's child cannot 
**      also be its parent otherwise cyclic loops will be created.
**      This error is not checked by the object manager.
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
**      manager.
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
**              limit.  It can even be the same value repeated
**              many times.
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
#include "debug_framework.h"
#include "mem_monitor_object.h"
#include "lock_object.h"
#include "avl_tree_object.h"
#include "index_object.h"

#define TYPICAL_NAME_SIZE                       (64)

typedef struct complex_attribute_value_s complex_attribute_value_t;
typedef struct attribute_value_s attribute_value_t;
typedef struct attribute_s attribute_t;
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
 * Attribute values can be simple or complex.
 * A simple attribute is any data which can fit into a long long int.
 * A complex data is a stream of bytes and its length.  It does NOT
 * have to be NULL terminated.
 *
 * The structure below can be used for both of these kinds of 
 * attribute data types.
 *
 * One consequence of this is that ALL simple data types,
 * regardless of their size, will always be stored as int64.
 * This is a bit wasteful but not so bad given the coding simplicity
 * it provides.
 *
 * The type of the attribute is determined by the bits in the
 * 'attribute_flags' field.
 *
 * The EXACT same attribute value can be added to an attribute, which
 * increments the 'ref_count' value.  This means an attribute
 * can have the same value (simple or complex) multiple times.
 * Since an attribute can have multiple values, simple or complex,
 * there is a linked list implementation of values for each attribute
 * id.  Since it is not expected for a single attribute id to have 
 * very many values, this implementation is sufficient.  If however,
 * an attribute starts storing lots of values, performance will
 * start suffering.
 *
 */

#define ATTRIBUTE_VALUE_SIMPLE_FLAG         (0x1)
#define ATTRIBUTE_VALUE_COMPLEX_FLAG        (0x2)

struct complex_attribute_value_s {

    int length;
    byte *data;

};

struct attribute_value_s {

    /* information about the attribute type */
    byte attribute_flags;

    /* same identical value can be present more than once */
    short ref_count;

    /* for chaining multiple values */
    attribute_value_t *next_avp;

    /* exact value of the simple or complex attribute */
    union {

        long long int simple_attribute_value;
        complex_attribute_value_t complex_attribute_value;

    } attribute_value_u;

};

/*
 * This is an attribute instance.
 * It can hold many attribute values.
 *
 * An example of an attribute instance can be 'port speed'.  It may
 * have a simple attribute value of 100M to represent 100 Mbits.
 */
struct attribute_s {

    object_t *object;               /* object of this attribute */
    int attribute_id;               /* which attribute is it */
    int n;                          /* how many values does it have */
    attribute_value_t *avp_list;    /* linked list of the attribute values */

};

/******************************************************************************
 *
 * object related structures
 *
 */

/*
 * User facing APIs mostly deal with this representation of an object
 * since pointers are mostly hidden from the user and the only way
 * to address an object is thru this.  This reduces the chance of a
 * user program corrupting pointers or accessing them incorrectly.
 */
struct object_identifier_s {

    int object_type;
    int object_instance;

};

/*
 * Internal APIs mostly use pointers since they are more protected.
 * and pointers are safe to use.  So, this one type can be used both 
 * for user facing APIs as well as internal uses.
 */
struct object_representation_s {

    /* set if the representation is a pointer */
    tinybool is_pointer;

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

    /* All the children of this object */
    index_obj_t children;

    /* All the attributes of this object */
    index_obj_t attributes;

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

    /* if a traversal is taking place, the object manager cannot be modified */
    boolean should_not_be_modified;

    /*
     * Objects are always uniquely indexed by 'object_type' & 
     * 'object_instance'.  There can be only ONE object of
     * this unique combination in every manager.  They are
     * kept in this avl tree for fast access.
     */
    avl_tree_t om_objects;

}; 

/************* User functions ************************************************/

/* debug infrastructure */
extern debug_module_block_t om_debug;

/*
 * initialize object manager
 */
extern int
om_init (object_manager_t *omp,
        boolean make_it_thread_safe,
        int manager_id,
        mem_monitor_t *parent_mem_monitor);

/*
 * creates an object with given id & type under the
 * specified parent.  Success returns 0.  If it already
 * exists, this is a no-op, unless the required parents
 * do not match the existing parents.  In that case,
 * the object creation is refused.
 */
extern int
om_object_create (object_manager_t *omp,
        int parent_object_type, int parent_object_instance,
        int object_type, int object_instance);

/*
 * returns true if the specified object exists in the object manager.
 */
extern bool
om_object_exists (object_manager_t *omp,
        int object_type, int object_instance);

static inline int
om_object_count (object_manager_t *omp)
{ return omp->om_objects.n; }

/*
 * add an attribute (id) to an object.  If it already exists, this is
 * a no-op.  If memory fails, error will be returned.  Success will
 * return a value of 0.
 */
extern int
om_attribute_add (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id);

/*
 * adds a simple value to the attribute (id).  Same exact value can
 * be added multiple times.  'howmany_extras' will add ADDITIONAL
 * copies of the value.
 *
 * If the specified attribute does not exist, it will also automatically
 * be added to the object.
 */
extern int
om_attribute_simple_value_add (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        long long int value, int howmany_extras);

/*
 * This function deletes all existing values of the attribute (id) 
 * and sets it ONLY to the new value specified.  Multiple copies
 * can be set specified by 'ref_count'.
 */
extern int
om_attribute_simple_value_set (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        long long int value, int ref_count);

/*
 * Deletes a simple attribute value from the attribute (id).
 * Specifies how many copies to be deleted in 'howmany'.  If
 * that is greater than the ref count of the value, all will be
 * deleted.
 */
extern int
om_attribute_simple_value_remove (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        long long int value, int howmany);

/*
 * same as above but for complex values.
 */
extern int
om_attribute_complex_value_add (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        byte *data, int len, int howmany_extras);

extern int
om_attribute_complex_value_set (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        byte *data, int len, int ref_count);

extern int
om_attribute_complex_value_remove (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        byte *data, int len, int howmany);

/*
 * This will return all the attribute value pointers in the
 * specified array 'avp_list' bounded by 'limit'.  Upon
 * completion, 'limit' will contain how many values have been
 * returned.
 */
extern int
om_attribute_values_get (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id,
        attribute_value_t *avp_list [], int *limit);

/*
 * destroys the attribute (id) and all associated values
 * of that attribute, from the specified object.
 */
extern int
om_attribute_remove (object_manager_t *omp,
        int object_type, int object_instance,
        int attribute_id);

/*
 * get the parent type & instance of an object and
 * place them in the addresses respectively.
 * If not found, error is returned.  Success
 * returns 0.
 */
extern int
om_parent_get (object_manager_t *omp,
        int object_type, int object_instance,
        int *parent_object_type, int *parent_object_instance);

/*
 * destroys an object and all its children, including attributes,
 * values, everything.
 */
extern int
om_object_remove (object_manager_t *omp,
        int object_type, int object_instance);

/*
 * traverses the object AND all its children applying the
 * function 'tfn' to all of them.  The parameters passed 
 * to the 'tfn' function will be:
 *
 *      param0: object manager pointer
 *      param1: the child pointer being traversed
 *      param2: p0
 *      param3: p1
 *      param4: p2
 *      param5: p3
 *      param6: p4
 *
 * The return value is the first error encountered by 'tfn'.
 * 0 means no error was seen.
 */
extern int
om_traverse (object_manager_t *omp,
        int object_type, int object_instance,
        traverse_function_pointer tfn,
        void *p0, void *p1, void *p2, void *p3, void *p4);

/*
 * destroys the entire object manager.  It cannot be used again
 * until re-initialised.
 */
extern void
om_destroy (object_manager_t *omp);

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
om_write (object_manager_t *omp);

/*
 * reads a object manager from a file
 */
extern int
om_read (int manager_id, object_manager_t *omp);

extern void
om_destroy (object_manager_t *omp);

#ifdef __cplusplus
} /* extern C */
#endif 

#endif /* __OBJECT_MANAGER_H__ */


