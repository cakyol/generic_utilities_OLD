
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

static inline boolean
thread_got_the_write_lock (lock_obj_t *lck)
{
    int tid;

    if (lck->readers > 0) return 0;
    tid = get_thread_id();
    if (lck->writer_thread_id < 0) {
        lck->writer_thread_id = tid;
        return 1;
    }
    return
        lck->writer_thread_id == tid;
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

    /* no readers or writers to start with */
    lck->readers = lck->pending_writers = 0;

    /* no writer thread either */
    lck->writer_thread_id = -1;

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
    while (lck->pending_writers > 0) {
	pthread_mutex_unlock(&lck->mtx);
	sched_yield();
	pthread_mutex_lock(&lck->mtx);
	continue;
    }
    lck->readers++;
    pthread_mutex_unlock(&lck->mtx);
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
    lck->pending_writers++;
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
    if (--lck->pending_writers <= 0) {
        lck->writer_thread_id = -1;
    }
    pthread_mutex_unlock(&lck->mtx);
}

void
lock_obj_destroy (lock_obj_t *lck)
{
    pthread_mutex_destroy(&lck->mtx);
    memset(lck, 0, sizeof(lock_obj_t));
}



