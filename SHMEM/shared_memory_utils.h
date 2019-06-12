
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol (gee_akyol@yahoo.com)
** Copyright: Cihangir Metin Akyol, November 2013
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

#ifndef __SHARED_MEMORY_UTILS_H__
#define __SHARED_MEMORY_UTILS_H__

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>

#ifndef TYPE_UINT
#define TYPE_UINT
typedef unsigned int uint;
#endif // TYPE_UINT

#ifndef TYPE_USHORT
#define TYPE_USHORT
typedef unsigned short ushort;
#endif // TYPE_USHORT

#ifndef TYPE_BYTE
#define TYPE_BYTE
typedef unsigned char byte;
#endif // TYPE_BYTE

#ifndef TYPE_BOOLEAN
#define TYPE_BOOLEAN
typedef unsigned char bool;
#ifndef TRUE
#define TRUE	((bool) 1)
#define true    TRUE
#endif
#ifndef FALSE
#define FALSE	((bool) 0)
#define false   FALSE
#endif
#endif // TYPE_BOOLEAN

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE	8
#endif // BITS_PER_BYTE

#define PUBLIC
#define PRIVATE         static

/* 
** comparing function.
** Compares to "elements" and returns < 0, 0 or > 0
** depending on the values compared.
*/
#ifndef COMPARING_FUNCTION
#define COMPARING_FUNCTION
typedef int (*comparing_function)(void *key, int data);
#endif // COMPARING_FUNCTION

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** @@@@@ 
**
** Debugging support
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

/*****************************************************************************
** get Linux thread id
*/
static inline int
get_thread_id (void)
{ return syscall(__NR_gettid); }

/*****************************************************************************
** IMPORTANT: Maintain the ordering but relative numbers are ok.
** Ie, make sure that DEBUG_LEVEL_LOW < DEBUG_LEVEL_NORMAL <
**	DEBUG_LEVEL_ERROR < DEBUG_LEVEL_CRITICAL
*/
#define DEBUG_LEVEL_LOW		(-100)
#define DEBUG_LEVEL_NORMAL	(-50)
#define DEBUG_LEVEL_ERROR	(-10)
#define DEBUG_LEVEL_FATAL	0
#define ERR		        (strerror(errno))

/*****************************************************************************
** overall debugging control
*/
extern bool debugging_on;
extern int current_debug_level;

/*****************************************************************************
** DONT EVER USE THIS DIRECTLY.
** Instead, always use the macros below
*/
extern void print_error (int debug_level, const char *source_file, 
    const char *function_name, int line_number, char *fmt, ...);

/*****************************************************************************/
static inline void turn_debugging_on (void)
{ debugging_on = TRUE; }

/*****************************************************************************/
static inline void turn_debugging_off (void)
{ debugging_on = FALSE; }

/*****************************************************************************/
static inline void set_debugging_level (int level)
{ current_debug_level = level; }

/*****************************************************************************/
extern void turn_logging_on (char *user_supplied_log_filename);
extern void turn_logging_off (void);

/*****************************************************************************/
#define DEFINE_DEBUG_VARIABLE(debv) \
    bool debv = FALSE; \
    void on_ ## debv (void) { debv = TRUE; } \
    void off_ ## debv (void) { debv = FALSE; } \
    int value_ ## debv (void) { return debv; }

/*****************************************************************************/
#define PRINT_INFO(fmt, args...) \
    if (debugging_on && (current_debug_level <= DEBUG_LEVEL_NORMAL)) { \
	print_error (DEBUG_LEVEL_NORMAL, __FILE__, __FUNCTION__, __LINE__, \
	    fmt, ## args); \
    }

/*****************************************************************************/
#define PRINT_DEBUG(debug_flag, fmt, args...) \
    if (debugging_on && debug_flag && (current_debug_level <= DEBUG_LEVEL_LOW)) { \
	print_error (DEBUG_LEVEL_LOW, __FILE__, __FUNCTION__, __LINE__, \
	    fmt, ## args); \
    }

/*****************************************************************************/
#define PRINT_ERROR(fmt, args...) \
    print_error (DEBUG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, \
	fmt, ## args);

/*****************************************************************************/
#define FATAL_ERROR(fmt, args...) \
    print_error (DEBUG_LEVEL_FATAL, __FILE__, __FUNCTION__, __LINE__, \
	fmt, ## args);

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** @@@@@ 
** 
** READ/WRITE LOCKS (CAN BE USED IN SHARED MEMORY)
**
** Read/Write synchronizer which can be used between processes as
** well as threads.
**
** Its characteristics are:
**
**  - No limit on readers (well.. MAXINT readers max).
**
**  - Only one active writer at a time.
**
**  - Read requests cannot stafailede out a writer.
**
**  - Re-enterable by the same writer thread.
**
**  - Can be called to be non-blocking if requested.
**
** For this to be usable between processes, the object
** MUST be defined in a shared memory area.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

typedef struct rwlock_s {

    pthread_mutex_t mtx;	// protects the rest of the variables
    int readers;		// number of concurrent readers
    int writers;		// pending/current write requests
    pid_t writer_thread_id;	// thread which is ACTUALLY writing

} rwlock_t;

/*****************************************************************************
** will return FALSE if something goes wrong
*/
extern bool rwlock_init (rwlock_t *rwl);

/*****************************************************************************
** WILL block until it gets the read lock
*/
extern void rwlock_read_lock (rwlock_t *rwl);

/*****************************************************************************
** will NOT block if it cannot obtain the read lock.
** Returns TRUE if it DID get the lock, FALSE otherwise.
*/
extern bool rwlock_read_lock_unblocked (rwlock_t *rwl);

/*****************************************************************************/
extern void rwlock_read_unlock (rwlock_t *rwl);

/*****************************************************************************
** WILL block until it gets the write lock
*/
extern void rwlock_write_lock (rwlock_t *rwl);

/*****************************************************************************
** will NOT block if it cannot obtain the write lock.
** Returns TRUE if it DID get the lock, FALSE otherwise.
*/
extern void rwlock_write_lock_unblocked (rwlock_t *rwl);

/*****************************************************************************/
extern void rwlock_write_unlock (rwlock_t *rwl);

/***************************************************************
****************************************************************
**
** @@@@@
**
** STACK OVERLAY OBJECT; USED FOR ALL SHARED MEM STUFF
**
****************************************************************
***************************************************************/

#define STACK_OBJECT_COMMON_FIELDS \
    rwlock_t lock; \
    int maxsize; \
    int n

/*
** generic stack overlay structure that can store ints
*/
typedef struct shm_stack_s {
    STACK_OBJECT_COMMON_FIELDS;
    int data [0];
} shm_stack_t;

#define DEFINE_STACK_OBJECT(typename, maximum_size) \
    typedef struct { \
	STACK_OBJECT_COMMON_FIELDS; \
	int data [maximum_size]; \
    } shm_stack_ ## typename ## _t

#define DECLARE_STACK_OBJECT(typename, name) \
    shm_stack_ ## typename ## _t name

extern void shm_stack_init (shm_stack_t *stack,
    char *name, bool init_data_part, uint maxsize);

extern bool shm_stack_push (shm_stack_t *stack, int value,
    bool lock_it);

extern bool shm_stack_pop (shm_stack_t *stack, int *value,
    bool lock_it);

extern bool shm_stack_empty (shm_stack_t *stack);

extern void shm_stack_remove (shm_stack_t *stack, int value,
    bool lock_it);

/*********************************************************************
**********************************************************************
**
** @@@@@
**
** OVERLAY FOR THE BIT LIST OBJECT
**
**********************************************************************
*********************************************************************/

#define BITLIST_OBJECT_COMMON_FIELDS \
    rwlock_t lock; \
    int size_in_bits; \
    int size_in_bytes; \
    int size_in_ints; \
    int n

typedef struct shm_bitlist_s {
    BITLIST_OBJECT_COMMON_FIELDS;
    int bits [0];
} shm_bitlist_t;

/* how many bits in one int */
#define INT_BITS		(sizeof(int) * BITS_PER_BYTE)
#define BITS_PER_INT		(INT_BITS)

/* how many whole ints we need for so many bits */
#define INTS_FOR_BITS(bits)	((int)((bits/INT_BITS) + 1))

#define DEFINE_BITLIST_OBJECT(typename, total_bits_needed) \
    typedef struct { \
	BITLIST_OBJECT_COMMON_FIELDS; \
	int bits [INTS_FOR_BITS(total_bits_needed)]; \
    } shm_bitlist_ ## typename ## _t

#define DECLARE_BITLIST_OBJECT(typename, name) \
    shm_bitlist_ ## typename ## _t name

extern void
shm_bitlist_init (shm_bitlist_t *bl, char *name, 
    uint desired_size_in_bits);

/*
** If bit_number is out of shm_bitlist bounds, 
** function return will be FALSE.  Otherwise,
** function result will be TRUE. 
**
** IMPORTANT:  A returned_bit_value of 0 means
** bit was not set.  Any other value means bit is set.
*/
extern bool
shm_bitlist_get_bit (shm_bitlist_t *bl, int bit_number,
    int *returned_bit_value, bool lock_it);

extern bool
shm_bitlist_set_bit (shm_bitlist_t *bl, int bit_number, 
    bool lock_it);

extern bool
shm_bitlist_clear_bit (shm_bitlist_t *bl, int bit_number, 
    bool lock_it);

extern void 
shm_bitlist_collect_ones (shm_bitlist_t *bl,
    int *returned_values, int *howmany, bool lock_it);

extern void
shm_bitlist_reset (shm_bitlist_t *bl, bool lock_it);

/***************************************************************
****************************************************************
**
** @@@@@
**
** SHARED MEMORY MALLOC
**
** This set of functions manage a shared memory segment
** and allows it to be used like malloc & free.  The
** segment grows as more memory is requested.
**
** It is NOT the most efficient or the most space saving,
** but it is SIMPLE and it works.
**
** Users must first create a "handle" and alloc & free
** using that handle.
**
****************************************************************
****************************************************************/

/*
** This is a pointer independent way of representing a shared
** memory address.  Given a shared memory manager handle, every
** process can access the correct memory location, regardless of
** where their memory is mapped on to.  Users are not supposed
** to use this directly but use it to obtain a pointer to their
** data and vice versa.  This is the kind of structure that should
** be stored in the shared memory itself, instead of pointers.
**
*/
typedef struct shm_address_s {
    int block_number;
    int offset;
} shm_address_t;

/*
** These function create/map a shared memory malloc/free
** implementation to the calling process.  The "create" version
** creates it for the first time in memory and the "attach"
** version attaches to an already existing memory.  
**
** The name is a shared memory segment name and should 
** always be in the form of slash followed by a name 
** like "/name".
*/
extern void *shm_allocator_init (char *name, bool create);
extern void *shm_allocator_create (char *name);
extern void *shm_allocator_attach (char *name);

/*
** Obtains "size" bytes from the memory region
** defined by "handle".  NULL is returned if
** no more memory remains.  If memory is available,
** a pointer to it is returned and the pointer
** independent address is returned in "shm_addr".
** Note that if not requested, shm_addr can be 
** specified as NULL.
*/
extern byte *shm_alloc (void *handle, int size, shm_address_t *shm_addr,
    bool lock_it);

/*
** converts an shm_address to a local process pointer
** value
*/
extern byte *shm_addr2addr (void *handle, shm_address_t *shmaddr,
    bool lock_it);

/*
** This frees up the memory block allocated by shm_alloc.
** It uses the shm_address_t as the memory to free up.
*/
extern void shm_address_free (void *handle, shm_address_t *shm_addr,
    bool lock_it);

/*
** Frees a memory allocated by shm_alloc.  It is very
** important that the handle passed into shm_free is
** exactly the same as the one for shm_alloc used in
** obtaining that memory.  And that the pointer value
** passed in is the same one obtained in the SAME process.
** A process which is NOT the mallocing process should NOT
** ever free the memory using this call.  It should instead
** use shm_address_free above.
*/
extern void shm_free (void *handle, byte *ptr, 
    bool lock_it);

/*
** Make an shm address into NULL
*/
extern void nullify_shm_address (shm_address_t *shmaddr);

/*
** If either block or offset number is < 0, the address
** is assumed NULL.
*/
extern bool null_shm_address (shm_address_t *shmaddr);

/***************************************************************
****************************************************************
**
** @@@@@
**
** SHARED MEMORY INDEX OBJECT
**
** This object provides index functionality to any user defined
** data.  It is very similar in nature to a object manager index
** concept.  This implementation is tailored for shared memory
** usage and has locks.
**
** WARNING:  The comparison routine should be written to accept the
**	     "data" parameter FIRST followed by the "user supplied"
**	     element pointer SECOND.
**
** Note that index object stores an element with the same key ONLY 
** once, ie, duplicate objects are just ignored.
**
****************************************************************
****************************************************************/

typedef struct shm_indxobj_node_s {
    int refcount;
    int data;
} shm_indxobj_node_t;

#define INDEX_OBJECT_COMMON_FIELDS \
    rwlock_t lock; \
    int maxsize; \
    bool duplicates_allowed; \
    int n

/*
** Generic index overlay structure that can store int.
** Note that we cannot pass a function pointer to be
** stored as part of the index object in shared memory.
** instead, we pass it every time we call a relevant function
** as needed by the routine.
*/
typedef struct shm_indxobj_s {
    INDEX_OBJECT_COMMON_FIELDS;
    shm_indxobj_node_t elements [0];
} shm_indxobj_t;

#define DEFINE_INDEX_OBJECT(typename, maximum_size) \
    typedef struct { \
	INDEX_OBJECT_COMMON_FIELDS; \
	shm_indxobj_node_t elements [maximum_size]; \
    } index_ ## typename ## _t

#define DECLARE_INDEX_OBJECT(typename, name) \
    index_ ## typename ## _t name

/*
** initialize an index object
*/
extern void shm_indxobj_init (shm_indxobj_t *indxobj, 
    char *name, int max_size, bool duplicates_allowed);

/*
** add the element specified by "data" to the index 
** object, based on the rules as defined by the 
** "cmp" comparison function using the "key".
*/
extern void shm_indxobj_insert (shm_indxobj_t *indxobj,
    comparing_function cmp, void *key, int data, 
    int *refcount, bool lock_it );

/*
** search and return a pointer to user element identified 
** by "key"  by the rules specified in comparison functiion cmp. 
**
**	If (found_it == TRUE), function return value
**	   denotes the data found.
**
**	If (found_it == FALSE), data could not be found
**	   and the function return value should not be used.
*/
extern void shm_indxobj_search (shm_indxobj_t *indxobj,
    comparing_function cmp, void *key, 
    int *refcount, int *returned_data,
    bool lock_it);

/*
** Remove the entry represented by "key" from the index
** object based on the rules of the comparison function
** cmp.
** 
** Here are the meanings of the return parameters:
**
**	If (removed == TRUE), it means removal succeeded
**	   and the data that was stored in the index
**	   based on key is returned as the function
**	   return value.
**
**	If (removed == FALSE), it means removal failed,
**	  and function return value should NOT be used.
*/
extern void shm_indxobj_remove (shm_indxobj_t *indxobj,
    comparing_function cmp, void *key, int how_many_to_remove,
    int *how_many_actually_removed, int *how_many_remaining,
    int *data_value_removed,
    bool lock_it);

/*
** Return the n'th element in the index.
**
** If (success == FALSE) it means no such element
** exists (n out of bounds).  
**
** If (success == TRUE), function return value
** denotes the data requested.
*/
extern int shm_indxobj_nth (shm_indxobj_t *ind, int n, 
    bool *success, bool lock_it);

static inline
bool shm_indxobj_full (shm_indxobj_t *ind)
{ return ind->n >= ind->maxsize; }

/*
** returns the LAST element in the list *AND* takes it
** OUT of the list.  If (success == TRUE), function
** return value is the data requested, otherwise,
** there are no more entries left in the index.
**
** As a side note, if ordering is not an issue, this 
** routine is highly recommended to be used since
** it is MUCH faster than a front pop.  It empties
** out the index in the reverse order.
*/
extern int shm_indxobj_end_pop (shm_indxobj_t *ind, bool *success,
    bool lock_it);

/* 
** Empty out all elements from the index.
*/
extern void shm_indxobj_reset (shm_indxobj_t *indxobj, bool lock_it);

/*
** These are provided for the user who wants to perform
** many consecutive operations on an index and the index
** needs to be protected during the entire time this
** is happening.  Note that these should NOT be used for
** the already defined simple index operations above
** since those protect the index by themselves already.
*/
extern void shm_indxobj_read_lock (shm_indxobj_t *indxobj);
extern void shm_indxobj_release_read_lock (shm_indxobj_t *indxobj);

/***************************************************************
****************************************************************
**
** @@@@@
**
** SHARED MEMORY CHUNK OBJECT
**
****************************************************************
***************************************************************/

/* not done yet */

/***************************************************************
****************************************************************
**
** @@@@@
**
** SHARED MEMORY AVL TREES
**
****************************************************************
***************************************************************/

typedef struct shm_avl_node_s {
    int index;			// array index of this entry
    int data;			// user data
    int refcount;		// refcount of user data
    int left, right;		// left & right links
    int parent;			// parent of this node
    int balance;		// AVL balance
} shm_avl_node_t;

#define AVL_TREE_COMMON_FIELDS \
    rwlock_t lock; \
    int size; \
    bool duplicates_allowed; \
    int root; \
    int count; \
    int first, last; \
    int offset_to_stack

/* AVL tree overlay object */
typedef struct shm_avl_tree_s {
    AVL_TREE_COMMON_FIELDS;
    shm_avl_node_t shm_avl_nodes [0];
} shm_avl_tree_t;

#define DEFINE_AVLTREE_OBJECT(typename, max_nodes) \
    DEFINE_STACK_OBJECT(typename, max_nodes); \
    typedef struct { \
	AVL_TREE_COMMON_FIELDS; \
	shm_avl_node_t shm_avl_nodes [max_nodes]; \
	DECLARE_STACK_OBJECT(typename, free_shm_avl_nodes); \
    } shm_avltree_ ## typename ## _t;

#define DECLARE_AVLTREE_OBJECT(typename, name) \
    shm_avltree_ ## typename ## _t name;

extern void shm_avl_tree_init (shm_avl_tree_t *tree, char *name, int size,
    bool duplicate_entries_allowed);

extern void shm_avl_tree_insert (shm_avl_tree_t *tree, 
    comparing_function cmpf,
    void *key, int data_tobe_inserted, 
    int *refcount, bool lock_it);

extern void shm_avl_tree_search (shm_avl_tree_t *tree, 
    comparing_function cmpf, void *key, 
    int *refcount, int *returned_data,
    bool lock_it);

extern void shm_avl_tree_remove (shm_avl_tree_t *tree, 
    comparing_function cmpf,
    void *key, int howmany_entries_to_remove, 
    int *howmany_actually_removed, int *still_remaining, 
    int *data_value_removed,
    bool lock_it);

#if 0
extern void shm_avl_tree_pre_traverse (shm_avl_tree_t *tree,
    iterator_function invoke_fn);

extern void shm_avl_tree_in_traverse (shm_avl_tree_t *tree,
    iterator_function invoke_fn);

extern void shm_avl_tree_post_traverse (shm_avl_tree_t *tree,
    iterator_function invoke_fn);

extern void shm_avl_tree_depth_traverse (shm_avl_tree_t *tree,
    iterator_function invoke_fn);

extern void shm_avl_tree_breadth_traverse (shm_avl_tree_t *tree,
    iterator_function invoke_fn);
#endif

/***************************************************************
****************************************************************
**
**		   GENERIC ADDRESS MAPPING
** 
****************************************************************
***************************************************************/

/*
** map a shared memory into user space.
** each unique shared memory segment mapped
** is identified by the unique name provided.
** The memory segment is created ONLY if the 
** create flag is set, otherwise it is simply mapped.
*/
extern byte *map_shared_memory (char *shared_memory_name, 
    int size, bool create);

#endif // __SHARED_MEMORY_UTILS_H__


