
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

#ifndef __UTILS_COMMON_H__
#define __UTILS_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

// #define _GNU_SOURCE

#include <sys/types.h>
#include <stdint.h>
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
#include <sys/uio.h>
#include <sys/syscall.h>
#include <sched.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/resource.h>

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ CONVENTIONS
**
**      All functions which return type of 'error_t' will
**	return a 0 for success and NON 0 far an error.	 
**
**	All functions which return an 'int' are actually 
**	returning a usable value and not an error code.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

/*
 * Apple specific oddities
 */
#ifdef __APPLE__

#include <malloc/malloc.h>

#define CLOCK_PROCESS_CPUTIME_ID	1

static inline int
clock_gettime (int clock_id, struct timespec *ts)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) < 0) return -1;
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

#else // !__APPLE__

#include <malloc.h>

#endif // __APPLE__

/*
** Unix style, all calls return 0 if no error
*/
#define error           1
#define ERROR		error

#define ok              0
#define OK		ok

typedef int error_t;

#define SUCCEEDED(exp)	((exp) == 0)
#define FAILED(exp)	((exp) != 0)

#ifndef KILO
#define KILO            1024
#endif // KILO

#ifndef MEGA
#define MEGA		(KILO * 1024)
#endif // MEGA

#ifndef GIGA
#define GIGA            (MEGA * 1024)
#endif // GIGA

#ifndef MILLION
#define MILLION		(1000 * 1000)
#endif // MILLION

#ifndef BILLION
#define BILLION         (MILLION * 1000)
#endif // BILLION

#ifndef TYPE_ULONG
#define TYPE_ULONG
typedef long long int64;
typedef unsigned long long uint64;
#endif // TYPE_ULONG

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
typedef int boolean;
typedef char tinybool;
typedef tinybool bool;
#ifndef TRUE
#define TRUE	((boolean) 1)
#endif
#define true    TRUE
#ifndef FALSE
#define FALSE	((boolean) 0)
#endif
#define false   FALSE
#endif // TYPE_BOOLEAN

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE	8
#endif // BITS_PER_BYTE

static inline boolean
no_space (int rv)
{
    return 
        (ENOSPC == rv) || 
        (ENOMEM == rv);
}

#define SAFE_POINTER_SET(ptr, data)	    if ((ptr)) *(ptr) = (data)

/*
 * a generic structure which can store multiple values.  In the past,
 * when 32 bit integers and 64 bit pointers were mixed, it was causing
 * lots of issues.  Also the type 'intptr_t' is not available on all
 * compilers.  So, this is the most portable way which will work with
 * all C compilers.
 */
typedef union datum_u {

    void *pointer;
    int64 integer;

} datum_t;

#define NULLIFY_DATUM(datum)		    ((datum).pointer = NULL)
#define NULLIFY_DATUMP(datump)		    ((datump)->pointer = NULL)
#define DATUM_IS_NULL(datum)		    (NULL == (datum).pointer)
#define DATUM_NOT_NULL(datum)		    ((datum).pointer != NULL)

#define SAFE_DATUMP_SET(datump, datum)	    SAFE_POINTER_SET(datump, datum)
#define SAFE_DATUMP_IS_NULL(datump)	    ((datump) && (NULL == (datump)->pointer))
#define SAFE_NULLIFY_DATUMP(datump)	    if (datump) (datump)->pointer = NULL
#define SAFE_DATUMP_NOT_NULL(datump)	    if ((datump) && ((datump)->pointer))

/*
 * Tree traversals may return a collection of datums consecutively
 * formed into an array.  This is the typedef which represents it.
 * 'n' is the number of elements always consecutively arranged in
 * the array. 'elements[n-1]' is the last datum in the array.
 *
 * Do NOT use 'max' and do NOT manipulate this object IN ANY WAY
 * except just to iterate thru it 'n' times.  This struct will 
 * always be temporarily created using malloc, so when finished
 * iterating, always use 'datum_list_free' to get rid of it.
 */
typedef struct datum_list_s {

    datum_t *datums;
    int max, n;

} datum_list_t;

/* internally used, NOT typically called by users */
extern datum_list_t *datum_list_allocate (int initial_size);
extern error_t datum_list_add (datum_list_t *dlp, datum_t dt);

/* This CAN be called by users */
extern void datum_list_free (datum_list_t *dlp);

#ifndef TYPE_COMPARISON_FUNCTION
#define TYPE_COMPARISON_FUNCTION
/*
 * generally used for comparison two structures 
 * to determine their ordering.  Like 'strcmp'
 * but for structures.
 */
typedef int (*comparison_function_pointer) (datum_t p0, datum_t p1);
#endif // TYPE_COMPARISON_FUNCTION

/*
 * A generic function pointer used in traversing trees or tries etc.
 * If the return value is 0, iteration will continue.  Otherwise,
 * iteration will stop.  
 *
 * The first parameter is always the object pointer in the context
 * this function is called.  For example, if it is being called in
 * the context of an avl tree, it will be the avl tree object pointer.
 * If it is being called in the context of an index object, it
 * will be the index object pointer.
 *
 * The second parameter is the object node in question.  Just like
 * the first param above, it can be an avl tree node or an index node.
 *
 * The third parameter will always be the actual user data stored
 * in the utility node.
 *
 * The rest of the parameters are user passed and fed into the function.
 *
 */
#ifndef TYPE_TRAVERSE_FUNCTION
#define TYPE_TRAVERSE_FUNCTION
typedef error_t (*traverse_function_pointer)
    (void *utility_object, void *utility_node, datum_t node_data, 
     datum_t extra_parameter_0,
     datum_t extra_parameter_1,
     datum_t extra_parameter_2,
     datum_t extra_parameter_3);
#endif // TYPE_TRAVERSE_FUNCTION

/*
 * This is very similar to the traverse function above but slightly
 * different.  In the case of tries, the first param will always be
 * the trie or ntrie object pointer, the second param will be the 
 * fully assembled key, the next param will be the length of the key
 * and the next one will be the user data stored against that key.
 * The rest of them are user supplied.
 */
#ifndef TRIE_TRAVERSE_FUNCTION
typedef error_t (*trie_traverse_function_pointer)
    (void *utility_object, void *node,
     byte *key, int key_length, void *key_data,
     void *p0, void *p1, void *p2, void *p3);
#define TRIE_TRAVERSE_FUNCTION
#endif

#ifndef PUBLIC
#define PUBLIC
#endif // PUBLIC

#ifndef PRIVATE
#define PRIVATE static
#endif // PRIVATE

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Some useful stuff
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

/*
 * get Linux thread id
 */
static inline int
get_thread_id (void)
{
#ifdef __APPLE__
    return 0;
#else
    return syscall(__NR_gettid);
#endif
}

extern boolean 
process_is_dead (pid_t pid);

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Time measurement functions
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

typedef struct timer_obj_s {

    struct timespec start, end;

} timer_obj_t;

static inline void 
timer_start (timer_obj_t *tp)
{ clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp->start); }

static inline void 
timer_end (timer_obj_t *tp)
{ clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp->end); }

extern 
void timer_report (timer_obj_t *tp, int64 iterations);

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Some pretty standard comparison functions
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

extern int 
compare_ints (datum_t d1, datum_t d2);

extern int
compare_pointers (datum_t d1, datum_t d2);

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Memory book keeping object
**       Use these instead of malloc/free and it keeps a very accurate
**	 count of how much memory has been used, how many times
**	 allocation has been called and how many times free has been 
**	 called.
**
**       It also zeroes out the allocated memory too.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

typedef struct mem_monitor_s {

    uint64 bytes_used;
    uint64 allocations;
    uint64 frees;

} mem_monitor_t;

extern void
mem_monitor_init (mem_monitor_t *mem);

extern void *
mem_monitor_allocate (mem_monitor_t *mem, int size);

extern void *
mem_monitor_reallocate (mem_monitor_t *mem, void *ptr, int newsize);

extern void
mem_monitor_free (mem_monitor_t *mem, void *ptr);

#define MEM_MON_VARIABLES \
    mem_monitor_t mem_mon, *mem_mon_p

#define MEM_MONITOR_SETUP(objp) \
    do { \
        mem_monitor_init(&objp->mem_mon); \
        objp->mem_mon_p = \
            parent_mem_monitor ? \
                parent_mem_monitor : &objp->mem_mon; \
    } while (0)

#define MEM_MONITOR_ALLOC(objp, size) \
    mem_monitor_allocate(objp->mem_mon_p, size)

#define MEM_REALLOC(objp, oldp, newsize) \
    mem_monitor_reallocate(objp->mem_mon_p, oldp, newsize)

#define MEM_MONITOR_FREE(objp, ptr) \
    mem_monitor_free(objp->mem_mon_p, ptr)

#define OBJECT_MEMORY_USAGE(objp, size_in_bytes, size_in_megabytes) \
    do { \
        size_in_bytes = ((int64) (sizeof(*(objp)) + \
            (objp)->mem_mon_p->bytes_used)); \
        size_in_megabytes = ((double) size_in_bytes / (double) MEGA); \
    } while (0)

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Some basic file/socket operations
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

extern error_t
read_exact_size (int fd, void *buffer, int size, boolean can_block);

extern error_t
write_exact_size (int fd, void *buffer, int size, boolean can_block);

extern error_t
filecopy (char *in_file, char *out_file);

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Simple bitlist functions
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

static inline int 
bitlist_get (byte *bytes, int bit_number)
{ return ((bytes[bit_number >> 3]) & (1 << (bit_number % 8))); }

static inline void
bitlist_set (byte *bytes, int bit_number)
{ bytes[bit_number >> 3] |= (1 << (bit_number % 8)); }

static inline void
bitlist_clear (byte *bytes, int bit_number)
{ bytes[bit_number >> 3] &= (~(1 << (bit_number % 8))); }

#ifdef __cplusplus
} // extern C
#endif 

#endif // __UTILS_COMMON_H__



