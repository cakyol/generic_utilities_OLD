
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

static boolean
thread_got_the_write_lock (lock_obj_t *lck)
{
    pthread_t tid;
    boolean granted = false;

    /* if any current readers, we cannot get the write lock */
    if (lck->readers > 0) {
        return false;
    }

    /*
     * If we are here, write lock can be obtained if noone else
     * has acquired it already or this thread already owns it.
     */
    tid = pthread_self();
    if (!lck->writer_thread_id_set) {
        lck->writer_thread_id = tid;
	lck->writer_thread_id_set = true;
        granted = true;
    } else {
        granted = pthread_equal(lck->writer_thread_id, tid);
    }
    if (granted) {
        // lck->write_lock_requests--;
        lck->write_lock_count++;
    }
    return granted;
}

static boolean
thread_got_the_read_lock (lock_obj_t *lck)
{
    /* if there is a current writer, cannot grant read lock */
    if (lck->write_lock_count > 0) {
        return false;
    }

    /* 
     * What to do when a write lock request comes in when a reader already
     * has a read lock.  Should we not allow any further read locks or not ?
     * Two possibilities exist:
     *
     * - Do not allow recursive locks.
     *   This may become too restrictive.
     *
     * - Do not allow a write lock request.
     *   This may starve the writer but is safer.  Alternative is a possible
     *   deadlock.  Say thread 1 gets the read lock.  Now thread 2 wants the
     *   write lock but cannot get it.  However, if we do not allow further
     *   read locks but the 1st thread recursively wants a read lock again
     *   and is not granted, 1st thread will wait on a read lock and the 2nd
     *   will wait till the read unlocks.  They both become deadlocked.
     *
     * So, to be able to provide recursive locks, AND ensure that no deadlocks
     * ever occur, we may have to starve the writer.  This will delay the
     * write lock but would avoid a deadlock, which is much worse.
     *
     */
#if 0 // see the argument above, let the write lock starve

    if (lck->write_lock_requests > 0) {
        return false;
    }

#endif // 0

    /* the thread can get the read lock */
    lck->readers++;
    return true;
}

/*
 * Initialize a lock object.
 * Successfull execution returns 'OK'.
 */
PUBLIC error_t 
lock_obj_init (lock_obj_t *lck)
{
    pthread_mutexattr_t mtxattr;
    int rv;

    /* clear all variables */
    memset(lck, 0, sizeof(lock_obj_t));

    /* get default mutex attributes */
    if ((rv = pthread_mutexattr_init(&mtxattr)))
	return rv;

    /* 
     * make mutex recursively enterable, for 
     * nested calls by the same process/thread
     */
    if ((rv = pthread_mutexattr_settype(&mtxattr, PTHREAD_MUTEX_RECURSIVE)))
        return rv;

    /* make mutex sharable between processes */
    if ((rv = pthread_mutexattr_setpshared(&mtxattr, PTHREAD_PROCESS_SHARED)))
        return rv;

#if 0

    /* make mutex robust */
    if ((rv = pthread_mutexattr_setrobust(&mtxattr, PTHREAD_MUTEX_ROBUST)))
        return rv;

#endif // 0

    /* init with the desired attributes */
    if ((rv = pthread_mutex_init(&lck->mtx, &mtxattr)))
	return rv;

    return 0;
}

/*
 * Obtains a read lock.  The way this works is that a read access
 * can be given only if there are no writers, pending or active.
 *
 * A pending writer is a thread waiting to obtain a write lock but
 * has not quite got it yet.  It may be awaiting for all the readers
 * to finish or another thread to finish writing.
 *
 * A pending writer always gets priority.  If writers are pending,
 * further read accesses are not granted.  This is so that readers 
 * cannot lock out a writer indefinitely.  If a writer is active/pending, 
 * as soon as the current readers are done, the writer gets priority.  
 * Until then, new read requests will not be granted.
 *
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
    lck->readers--;
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
    // lck->write_lock_requests++;
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
    if (--lck->write_lock_count <= 0) {
        lck->writer_thread_id_set = false;
    }
    pthread_mutex_unlock(&lck->mtx);
}

void
lock_obj_destroy (lock_obj_t *lck)
{
    pthread_mutex_destroy(&lck->mtx);
    memset(lck, 0, sizeof(lock_obj_t));
}



