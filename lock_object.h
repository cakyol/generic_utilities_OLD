
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

typedef struct lock_obj_s {

    /* this protects access to all the variables */
    pthread_mutex_t mtx;

    /* number of distinct threads which have acquired the read lock */
    ushort  readers;

    /*
     * number of threads waiting in line to obtain a write lock.
     * This is important since it marks that a write lock request
     * is pending, which blocks any new read lock requests.
     */
    ushort write_lock_requests;

    /* how many consecutive times the current thread acquired the write lock */
    ushort write_lock_reentry_count;

    /*
     * The thread which has the write lock now.  If 'writer_thread_id_set'
     * is true, it means 'tid' is valid.  Since there is 'NULL'
     * equivalent of a thread id, the boolean is used to indicate
     * whether it is set or not set.
     */
    bool writer_thread_id_set;
    pthread_t writer_thread_id;

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


