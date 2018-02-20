
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
**	- Can be used in shared memory between multiple processes.
**	- No limit on readers (well.. MAXUSHORT).
**	- read locks ARE recursive altho discouraged.
**	- Only one active writer at a time.
**      - write locks are *NOT* recursive, deadlock will occur if recursive
**        write locking is attempted.
**	- A dead process which may have write locked will be detected
**	  when another write lock is attempted and will be cleaned up.
**	  Unfortunately however, this will NOT work for read locks.
**	  Ie, if a process which may have acquired the read lock dies
**	  before releasing the read lock, things will just go to 
**	  worse from bad.
**	- read locks will not starve a write lock.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __LOCK_OBJ_H__
#define __LOCK_OBJ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>

typedef struct lock_obj_s {

    volatile char mtx;
    volatile short readers;
    volatile char write_pending;
    volatile char writing;

} lock_obj_t;

extern int 
lock_obj_init (volatile lock_obj_t *lck);

extern void
grab_read_lock (volatile lock_obj_t *lck);

extern void 
release_read_lock (volatile lock_obj_t *lck);

extern void 
grab_write_lock (volatile lock_obj_t *lck);

extern void
release_write_lock (volatile lock_obj_t *lck);

extern void 
lock_obj_destroy (lock_obj_t *lck);

#define LOCK_VARIABLES \
    lock_obj_t *lock; \

#define LOCK_SETUP(obj) \
    do { \
        obj->lock = 0; \
        if (make_it_thread_safe) { \
            obj->lock = MEM_MONITOR_ALLOC(obj, sizeof(lock_obj_t)); \
            if (0 == obj->lock) return ENOMEM; \
            int __rv__ = lock_obj_init(obj->lock); \
            if (__rv__) return __rv__; \
            grab_write_lock(obj->lock); \
        } \
    } while (0)

#define READ_LOCK(obj) \
    if (obj->lock) grab_read_lock(obj->lock)

#define WRITE_LOCK(obj) \
    if (obj->lock) grab_write_lock(obj->lock)

#define READ_UNLOCK(obj) \
    if (obj->lock) release_read_lock(obj->lock)

#define WRITE_UNLOCK(obj) \
    if (obj->lock) release_write_lock(obj->lock)

#define LOCK_OBJ_DESTROY(obj) \
    do { \
        if (obj->lock) { \
            lock_obj_destroy(obj->lock); \
            MEM_MONITOR_FREE(obj, obj->lock); \
            obj->lock = 0; \
        } \
    } while (0)

#ifdef __cplusplus
} // extern C
#endif 

#endif // __LOCK_OBJ_H__


