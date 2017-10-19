
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
**	This is a VERY fast and a VERY flexible object database.
**
**	Almost every data structure is either an avl tree or a binary
**	index object, making it very efficient.
**
**	An object always have ONE parent but can have many children.  
**	It is up to the user to make sure an object's child cannot 
**	also be a parent otherwise infinite loops will be created.
**
**	An object can have any number of attributes and these can be
**      added and/or deleted dynamically during an object's lifetime.
**
**      Each such object attribute also can have MANY values.  Those
**	are maintained in a random linked list.  This makes the attribute
**	value manipulation somewhat slower but typically, it is not expected
**	that an attribute will contain more than at most a few values.
**
**	An object is identified distinctly by its type & instance and hence
**	there can be only ONE instance of an object of the same type in one
**	database.  All object types & instances MUST be > 0.  Values <= 0 are
**	reserved and should not be used.
**
**	Conversely, an attribute is also distinctly identified by an integer
**	of value > 0.  Do not use attribute ids of <= 0.
**
**	An object always has ONLY ONE parent.
**
**	An object can have 0 -> many children.
**
**	An object can have 0 -> many attributes.
**
**	An object can add/delete children at any time.
**
**	If an object is destroyed, all its children will also be destroyed.
**
**	Object attributes can be and are dynamically addable and deletable
**		from an object.  There is no limit.
**
**	Object attributes are multi valued.  An attribute can hold
**		multiple simple or complex values.  There is no
**		limit.
**
**		Note that there is a distinction between an 'attribute
**		value add' and an 'attribute value set'.  The 'set' will
**		delete all values and re-set the attribute value to
**		only those values specified in the call, whereas an
**		attribute 'add' is additive, it will actually add to
**		the already existing values.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __GENERIC_OBJECT_DATABASE_H__
#define __GENERIC_OBJECT_DATABASE_H__

/* This uses more memory for widespread attribute values
 * but is extremely fast.  If attribute id numbers are
 * kept consecutive, then it also uses much less memory
 */
#define USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES

#include "table.h"
#include "dynamic_array.h"
#include "event_types.h"
#include "object_types.h"

typedef struct attribute_value_s attribute_value_t;
typedef struct attribute_instance_s attribute_instance_t;
typedef struct object_s object_t;
typedef struct object_database_s object_database_t;
typedef struct event_record_s event_record_t;
typedef void (*event_handler_function)(event_record_t*);

/******************************************************************************
 *
 * attribute related
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
 * This is not so bad given the simplicity it provides.
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
 * many values, this implementation is suficient.  After all,
 * how many values is expected ?  Bear in mind obviously that if
 * one attribute starts storing lots of values, performance
 * will start suffering.
 *
 */
struct attribute_value_s {

    attribute_value_t *next_attribute_value;
    int attribute_value_length;
    int64 attribute_value_data;
};

struct attribute_instance_s {

    object_t *object;		// which object does this attribute belong to
    int attribute_id;		// which attribute is it
    int n_attribute_values;	// how many values does it have
    attribute_value_t *avps;	// linked list of the actual values
};

/*
 * add the specified simple value 'value' to the attribute.
 * if the value is already there, it is a no op but not an error.
 * The value can be any simple byte up to the size of int64.
 */
extern error_t
attribute_instance_add_simple_value (attribute_instance_t *aitp, 
	int64 value);

/*
 * SET the attribute to this specific value.  All other already
 * existing values will be gone.  The attribute value will be
 * reset ONLY to this value.
 */
extern error_t
attribute_instance_set_simple_value (attribute_instance_t *aitp, 
	int64 value);

/*
 * reverse of the above.  Delete the value from the list of values.
 * if the value is not found, it IS considered an error
 */
extern error_t
attribute_instance_delete_simple_value (attribute_instance_t *aitp,
	int64 value);

/*
 * same as the group of functions above but for a complex attribute
 */
extern error_t
attribute_instance_add_complex_value (attribute_instance_t *aitp,
	byte *stream, int length);

extern error_t
attribute_instance_set_complex_value (attribute_instance_t *aitp, 
	byte *stream, int length);

extern error_t
attribute_instance_delete_complex_value (attribute_instance_t *aitp,
	byte *stream, int length);

/*
 * obtains the n'th value from the attribute values.
 * if n < 0, the first value will be returned.  If
 * n > maximum number of values, the last value
 * will be returned.
 *
 * Note that first attribute value is the 0'th value (not 1st).
 */
extern error_t
attribute_instance_get_value (attribute_instance_t *aitp, int nth,
	attribute_value_t **returned_attribute_value);

/*
 * destroys an attribute and all its values from an object
 */
extern void
attribute_instance_destroy (attribute_instance_t *aitp);

/******************************************************************************
 *
 * object related
 *
 */

struct object_s {

    /* which database this object belongs to */
    object_database_t *obj_db;

    /*
     * Unique identification of this object.  This combination is
     * always unique per database and distinctly identifies an object.
     * This combination is always the unique 'key'.
     */ 
    int object_type;
    int object_instance;

    /*
     * Unless it is the root object, its parent
     * Root object's parent is NULL.
     */ 
    object_t *parent;

    /* Can have many children; no limit */
    table_t children;

    /* All the attributes of this object */
#ifdef USE_DYNAMIC_ARRAYS_FOR_ATTRIBUTES
    dynamic_array_t attributes;
#else 
    table_t attributes;
#endif

    /* for private use, do NOT use */
    unsigned int private_flags;
};

static inline boolean
object_is_root (object_t *obj)
{ return ROOT_OBJECT_TYPE == obj->object_type; }

static inline int
object_child_count (object_t *obj)
{ return table_member_count(&obj->children); }

extern object_t *
object_create (object_t *parent, 
	int object_type, int object_instance);

extern attribute_instance_t *
object_attribute_instance_add (object_t *obj, int attribute_id);

extern error_t
object_attribute_add_simple_value (object_t *obj, int attribute_id,
	int64 value);

extern error_t
object_attribute_set_simple_value (object_t *obj, int attribute_id,
	int64 value);

extern error_t
object_attribute_delete_simple_value (object_t *obj, int attribute_id,
	int64 value);

extern error_t
object_attribute_add_complex_value (object_t *obj, int attribute_id,
	byte *stream, int length);

extern error_t
object_attribute_set_complex_value (object_t *obj, int attribute_id,
	byte *stream, int length);

extern error_t
object_attribute_delete_complex_value (object_t *obj, int attribute_id,
	byte *stream, int length);

extern attribute_instance_t *
object_find_attribute_instance (object_t *obj, int attribute_id);

extern error_t
object_attribute_get_nth_value (object_t *obj, int attribute_id, int nth,
	attribute_value_t **avtpp);

extern error_t
object_attribute_instance_destroy (object_t *obj, int attribute_id);

extern void
object_destroy (object_t *obj);

/******************************************************************************
 *
 * database related
 *
 */

#define TYPICAL_NAME_SIZE			(64)

struct object_database_s {

    /* dynamic memory bookkeeper */
    mem_monitor_t mem_mon, *mem_mon_p;

    /* unique integer for this database */
    int database_id;

    /*
     * Objects are always uniquely indexed by 'object_type' & 
     * 'object_instance'.  There can be only ONE object of
     * this unique combination in every database.
     */
    table_t object_index;

    /*
     * the root object; this is special, can NOT be deleted
     * This is used to store all the objects (children) of 
     * the database.
     */
    object_t root_object;

    /* notify whoever is interested */
    event_handler_function evhf;

    /*
     * There are two reasons for database changes.  Ones produced locally
     * by direct function calls from the application itself, or ones that
     * happen because of processing events arriving from remote databases
     * to synchronise them all.
     *
     * In the case of local changes, this database must send the change
     * to all other databases so that they can be kept in sync.
     *
     * If on the other hand, an event was received from another database,
     * after syncing this database to it, the event must be locally
     * advertised to the registered local event handler.
     *
     * When either case is happening, the other case must not be executed
     * or the database will start spinning and chase its own tail 
     * continuously.
     *
     * This variable controls this situation.
     */
    boolean processing_remote_event;

    // for spurious event suppression
    int blocked_events;
};

extern error_t
database_initialize (object_database_t *obj_db,
        int db_id, event_handler_function evhf,
        mem_monitor_t *parent_mem_monitor);

static inline int
database_object_count (object_database_t *obj_db)
{ return table_member_count(&obj_db->object_index); }

extern void
database_register_evhf (object_database_t *obj_db,
	event_handler_function evhf);

extern object_t *
database_object_find (object_database_t *obj_db, 
	int object_type, int object_instance);

extern int
database_get_objects_of_type (object_t *root_object, 
	int object_type, object_t **found_objects, int limit);

extern error_t
database_object_find_and_destroy (object_database_t *obj_db,
	int object_type, int object_instance);

/******************************************************************************
 *
 *  Database reading & writing into a file.
 *
 *  The database in a file conforms to the below format:
 *
 *  - The filename is formed by concatanating "database_" & id number,
 *    such as "database_29".
 *
 *  - One or more object records where each one is an object,
 *    its parent and all its attributes and attribute values.
 *
 *  OPTIONAL
 *  - Last entry is a checksum.  This stops the database file from being
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
 * Writes out the database to a file.
 */
extern error_t
database_store (object_database_t *obj_db);

/*
 * reads a database from a file
 */
extern error_t
database_load (int database_id, object_database_t *obj_db);

extern void
database_destroy (object_database_t *obj_db);

extern uint64
database_memory_usage (object_database_t *obj_db, double *mega_bytes);

/******************************************************************************
 *
 * events related
 *
 */

/*
 * This structure is used to notify every possible event that could
 * possibly happen in the system.  ALL the events fall into the
 * events category above.
 *
 * The structure includes all the information needed to process
 * the event completely and it obviously cannot contain any pointers.
 *
 * The 'event' field determines which part of the rest of the structure
 * elements are needed (or ignored).
 *
 * One possible explanation is needed in the case of an attribute event.
 * If this was an attribute event which involves a variable size
 * attribute value, the length will be specified in 'attribute_value_length'
 * and the byte stream would be available from '&attribute_value_data'
 * onwards.  This makes the structure of variable size, based on
 * the parsed parameters, with a complex attribute directly attached to the
 * end of the structure.
 */
struct event_record_s {

    /*
     * this must be first since during reads & writes, it can
     * be determined at the very beginning
     */
    int total_length;

    /* which database this event/command applies to */
    int database_id;

    /* what is the event/command */
    int event;

    /* object to which the event is directly relevant to */
    int object_type;
    int object_instance;

    /*
     * if the event involves another object, which is it.
     * Not used if another object is not involved.  One case
     * it is involved is object creation event.  This object
     * represents the PARENT in this case.
     */
    int related_object_type;
    int related_object_instance;

    /* if the event involves an attribute, these are used */
    int attribute_id;
    int attribute_value_length;
    int64 attribute_value_data;
    byte extra_data [0];
};

#endif // __GENERIC_OBJECT_DATABASE_H__


