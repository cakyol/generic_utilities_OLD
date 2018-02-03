
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
**	- No limit on readers (well.. MAXUSHORT)
**	- read locks ARE recursive altho discouraged.
**	- Only one active writer at a time
**      - write locks are *NOT* recursive, deadlock will occur if recursive
**        write locking is attempted.
**	- A dead process which may have write locked will be detected
**	  when another write lock is attempted and will be cleaned up.
**	  Unfortunately however, this will NOT work for read locks.
**	  Ie, if a process which may have acquired the read lock dies
**	  before releasing the read lock, things will just go to 
**	  worse than bad.
**	- read locks will not starve a write lock.
**	- Competing write locks MAY however starve each other.
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

/*
 * For locks to be usable in shared memory by multiple processes & threads,
 * simply using the thread id is not enuf to identify a unique execution unit.
 * A combination of BOTH the process id AND thread id must be used.  This
 * structure defines this tuple.  If pid is -1, it indicates an invalid
 * entry.  Note however that since we are not allowing recursive locks,
 * for the time being we do not actually need to use the tid.
 */
typedef struct mp_thread_id_s {

    int pid;
    pthread_t tid;

} mp_thread_id_t;

typedef struct lock_obj_s {

    /* protects ALL the variables below */
    pthread_mutex_t mtx;

    /* count of current active readers */
    short readers;

    /* a boolean indicating if at least one writer is waiting */
    unsigned char pending_writer;

    /* the current writer, if any */
    mp_thread_id_t writer;

} lock_obj_t;

extern int 
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


