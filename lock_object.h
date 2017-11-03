
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
** @@@@@
** 
** READ/WRITE LOCKS 
**
** Read/Write synchronizer to be used between processes as
** well as threads.  It can achieve the following:
**
**	- No limit on readers (well.. MAXINT readers max)
**	- Only one active writer at a time
**	- read requests will not starve out a writer
**
** For this to be usable between processes, the object
** must be defined in a shared memory area.  If inter 
** process sync is not required, then it does not have
** to be declared in shared memory.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __LOCK_OBJ_H__
#define __LOCK_OBJ_H__

#include "utils_common.h"

/*
 * If so many read lock requests are pending on a lock,
 * no more write lock requests will be granted to not
 * starve the read lock requests.
 */
#define MAX_PENDING_READ_LOCK_REQUESTS      5

/*
 * Conversely, if so many write lock requests are pending 
 * on a lock, no more read locks will be granted to not
 * starve the write lock requests.
 *
 * Usually, write lock requests should get higher priority
 * and that is why this number is less.  If this number
 * is 1, write lock requests will ALWAYS trump a read lock
 * request.
 */
#define MAX_PENDING_WRITE_LOCK_REQUESTS     2

typedef struct lock_obj_s {

    pthread_mutex_t mtx;	    // protects the rest of the variables
    int readers;		    // number of actual concurrent readers
    int write_lock_requests;        // pending write lock requests
    int write_lock_reentry_count;   // rentrant write lock count
    int writer_thread_id;	    // actual thread which has the write lock

} lock_obj_t;

extern error_t 
lock_obj_init (lock_obj_t *lck);

extern void
grab_read_lock (lock_obj_t *lck);

extern void 
release_read_lock (lock_obj_t *lck);

extern void 
grab_write_lock (lock_obj_t *lck);

extern void
release_write_lock (lock_obj_t *lck);

extern void 
lock_obj_destroy (lock_obj_t *lck);

#ifdef LOCKABILITY_REQUIRED

    #define LOCK_VARIABLES \
        lock_obj_t lock; \
        boolean make_it_thread_safe

    #define LOCK_SETUP(obj) \
        do { \
            obj->make_it_thread_safe = make_it_thread_safe; \
            if (make_it_thread_safe) { \
                if (FAILED(lock_obj_init(&obj->lock))) { \
                    return EFAULT; \
                } \
                grab_write_lock(&obj->lock); \
            } \
        } while (0)

    #define READ_LOCK(obj) \
        if (obj->make_it_thread_safe) grab_read_lock(&obj->lock)

    #define WRITE_LOCK(obj) \
        if (obj->make_it_thread_safe) grab_write_lock(&obj->lock)

    #define READ_UNLOCK(obj) \
        if (obj->make_it_thread_safe) release_read_lock(&obj->lock)

    #define WRITE_UNLOCK(obj) \
        if (obj->make_it_thread_safe) release_write_lock(&obj->lock)

    #define LOCK_OBJ_DESTROY(obj) \
        do { lock_obj_destroy(&obj->lock); } while (0)

#else // !LOCKABILITY_REQUIRED

    #define LOCK_VARIABLES

    #define LOCK_SETUP(obj)

    #define READ_LOCK(obj) 

    #define WRITE_LOCK(obj)

    #define READ_UNLOCK(obj)

    #define WRITE_UNLOCK(obj)

    #define LOCK_OBJ_DESTROY(obj)           do { } while (0)

#endif // !LOCKABILITY_REQUIRED

#endif // __LOCK_OBJ_H__


