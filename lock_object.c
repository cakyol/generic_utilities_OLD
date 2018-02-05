
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
 * Note that instead of using pid, we could simply have used a boolean
 * to implement write lock.  We could have done it such that a true means
 * the write lock is taken and a 0 means vice versa.  This would eliminate
 * the extra calls to get the pid.  However, we need the pid for the extra 
 * checking we do to determine if the holder of a write lock is still
 * alive or not, so that we can clear the lock in case the process holding 
 * it has died.  That is why we use the poid instead of a simple boolean.
 */
static inline int
get_cached_pid (void)
{
static int cached_pid = -1;
    while (1) {
	if (cached_pid >= 0) return cached_pid;
	cached_pid = getpid();
    }
}

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
write_locked_already (lock_obj_t *lck)
{ 
    /* not write locked */
    if (lck->writer_pid < 0) return 0;

    /* check whether whoever locked it is still alive */
    if (process_is_dead(lck->writer_pid)) {
	lck->writer_pid = -1;
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
    lck->writer_pid = get_cached_pid();
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

/******* Public functions start here *****************************************/

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
    lck->writer_pid = -1;

    return 0;
}

PUBLIC void 
grab_read_lock (lock_obj_t *lck)
{
    while (1) {
	pthread_mutex_lock(&lck->mtx);
        if (thread_got_the_read_lock(lck)) {
            pthread_mutex_unlock(&lck->mtx);
            return;
        }
	pthread_mutex_unlock(&lck->mtx);
	sched_yield();
    }
}

PUBLIC void 
release_read_lock (lock_obj_t *lck)
{
    pthread_mutex_lock(&lck->mtx);
    if (--lck->readers < 0) lck->readers = 0;
    pthread_mutex_unlock(&lck->mtx);
}

PUBLIC void 
grab_write_lock (lock_obj_t *lck)
{
    while (1) {
	pthread_mutex_lock(&lck->mtx);
	if (thread_got_the_write_lock(lck)) {
	    pthread_mutex_unlock(&lck->mtx);
	    return;
	}
	pthread_mutex_unlock(&lck->mtx);
	sched_yield();
    }
}

PUBLIC void 
release_write_lock (lock_obj_t *lck)
{
    pthread_mutex_lock(&lck->mtx);
    lck->pending_writer = 0;
    lck->writer_pid = -1;
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



