
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

#include "lock_object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUBLIC

/*
 * A process is dead when a kill 0 signal cannot be sent to it.
 */
static inline int
process_is_dead (int pid)
{
    /* process is alive */
    if (kill(pid, 0) == 0) return 0;

    /* process could not be found, it is definitely dead */
    if (ESRCH == errno) return 1;

    /* 
     * permission problem, cannot send signal 0 to the
     * specified process so it must therefore be alive.
     */
    if (EPERM == errno) return 0;

    /* cannot possibly be here unless definition of 'kill' has been changed */
    assert(0);

    /* shut the compiler up */
    return 0;
}

static inline int
valid_mp_thread (mp_thread_id_t *mpt)
{ return (mpt->pid >= 0); }

/*
 * MUST be called already knowing both entries are valid.
 */
static inline int
same_mp_threads (mp_thread_id_t *mt1, mp_thread_id_t *mt2)
{
    return
        (mt1->pid == mt2->pid) &&
        pthread_equal(mt1->tid, mt2->tid);
}

/*
 * This is a dual purpose function.  It checks whether the write
 * lock has already been acquired by a process/thread AND whether
 * that process is still alive.  If process has died, it releases
 * the lock automatically.
 */
static inline int
write_locked_already (lock_obj_t *lck)
{ 
    /* not write locked */
    if (lck->writer.pid < 0) return 0;

    /* check whether whoever locked it is still alive */
    if (process_is_dead(lck->writer.pid)) {
	lck->writer.pid = -1;
	return 0;
    }
    return 1;
}

static int
thread_got_the_write_lock (lock_obj_t *lck)
{
    /* mark that a write lock is awaiting */
    lck->pending_writer = 1;

    /* A writer already exists and do not allow recursive writes */
    if (write_locked_already(lck)) return 0;

    /* if any current readers, we cannot get the write lock */
    if (lck->readers > 0) return 0;

    /* if we are here, there are no readers or curent writers, so grab it */
    lck->writer.pid = getpid();
    lck->writer.tid = pthread_self();
    lck->pending_writer = 0;

    return 1;
}

static int
thread_got_the_read_lock (lock_obj_t *lck)
{
    /* if a pending writer exists, cannot get it */
    if (lck->pending_writer) return 0;

    /* if a current writer exists, cannot get it */
    if (write_locked_already(lck)) return 0;

    /* the thread can get the read lock */
    lck->readers++;

    return 1;
}

/*
 * Initialize a lock object.
 * Successfull execution returns 'OK'.
 */
PUBLIC int 
lock_obj_init (lock_obj_t *lck)
{
    pthread_mutexattr_t mtxattr;
    int rv;

    /* clear all variables */
    memset(lck, 0, sizeof(lock_obj_t));

    /* set default mutex attributes */
    if ((rv = pthread_mutexattr_init(&mtxattr)))
	return rv;

#if 0

    /* 
     * make mutex recursively enterable, for 
     * nested calls by the same process/thread
     */
    if ((rv = pthread_mutexattr_settype(&mtxattr, PTHREAD_MUTEX_RECURSIVE)))
        return rv;

    /* make mutex robust */
    if ((rv = pthread_mutexattr_setrobust(&mtxattr, PTHREAD_MUTEX_ROBUST)))
        return rv;

#endif // 0

    /* make mutex sharable between processes */
    if ((rv = pthread_mutexattr_setpshared(&mtxattr, PTHREAD_PROCESS_SHARED)))
        return rv;

    /* init with the desired attributes */
    if ((rv = pthread_mutex_init(&lck->mtx, &mtxattr)))
	return rv;

    /* no writer present at first */
    lck->writer.pid = -1;

    return 0;
}

/*
 * Obtains a read lock.  The way this works is that a read access
 * can be given only if there are no writers.
 */
PUBLIC void 
grab_read_lock (lock_obj_t *lck)
{
    pthread_mutex_lock(&lck->mtx);
    while (1) {
        if (thread_got_the_read_lock(lck)) {
            pthread_mutex_unlock(&lck->mtx);
            return;
        }
	pthread_mutex_unlock(&lck->mtx);
	sched_yield();
	pthread_mutex_lock(&lck->mtx);
	continue;
    }
}

/*
 * Unlock a read lock.  This call should be made ONLY if there are
 * actual read requests outstanding, otherwise the results of further
 * locks will be unpredictable and may cause deadlocks & hangs.
 * It is the caller's responsibility to ensure that.
 */
PUBLIC void 
release_read_lock (lock_obj_t *lck)
{
    pthread_mutex_lock(&lck->mtx);
    if (--lck->readers < 0) lck->readers = 0;
    pthread_mutex_unlock(&lck->mtx);
}

/*
 * Very similar to obtaining a read lock as described above.
 * The difference is that write lock is exclusive and can be
 * granted to only ONE thread at a time, whereas a read lock
 * can be granted to many threads.  The way this works is that
 * first a writer request is marked.  This blocks out any further
 * readers.  If there are no readers, and also no other writers, 
 * then an exclusive write access is granted.  As long as the
 * existing readers are present or another thread is actually
 * writing, this will block.
 *
 * Note that this lock is re-entrant by the SAME thread.  This
 * means that a thread may write lock repeatedly as long as
 * it was granted the lock in the first place.  If that is the
 * case, the corresponding unlock MUST be issued just as many
 * times to actually release the write lock.
 */
PUBLIC void 
grab_write_lock (lock_obj_t *lck)
{
    pthread_mutex_lock(&lck->mtx);
    while (1) {
	if (thread_got_the_write_lock(lck)) {
	    pthread_mutex_unlock(&lck->mtx);
	    return;
	}
	pthread_mutex_unlock(&lck->mtx);
	sched_yield();
	pthread_mutex_lock(&lck->mtx);
	continue;
    }
}

/*
 * Unlock a write lock.  It is the caller's responsibility
 * to make sure that this function is called appropriately.
 * Otherwise, behaviour is unpredictable and deadlocks and 
 * corrupted data will result.  For example, calling this
 * when the lock is not even acquired will definitely corrupt
 * the data structures.
 */
PUBLIC void 
release_write_lock (lock_obj_t *lck)
{
    pthread_mutex_lock(&lck->mtx);
    lck->pending_writer = 0;
    lck->writer.pid = -1;
    pthread_mutex_unlock(&lck->mtx);
}

void
lock_obj_destroy (lock_obj_t *lck)
{
    pthread_mutex_destroy(&lck->mtx);
    memset(lck, 0, sizeof(lock_obj_t));
}

#ifdef __cplusplus
} // extern C
#endif 



