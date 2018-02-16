
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

#ifdef USE_GNUC_CAS
    lck->mtx = 0;
#else

    /* init with the desired attributes */
    if ((rv = pthread_mutex_init(&lck->mtx, &mtxattr)))
	return rv;
#endif

    return 0;
}

#ifdef USE_GNUC_CAS

static inline void 
grab_mutex (char *mtx)
{
    while (1) {
	if (__sync_val_compare_and_swap(mtx, 0, 1) == 0) return;
	sched_yield();
    }
}

static inline void 
release_mutex (char *addr)
{ *addr = 0; }

#else

static inline void
grab_mutex (pthread_mutex_t *mtx)
{ pthread_mutex_lock(mtx); }

static inline void
release_mutex (pthread_mutex_t *mtx)
{ pthread_mutex_unlock(mtx); }

#endif

PUBLIC void 
grab_read_lock (lock_obj_t *lck)
{
    while (1) {
	grab_mutex(&lck->mtx);
	if (lck->write_pending == 0) {
	    lck->readers++;
	    release_mutex(&lck->mtx);
            return;
        }
	release_mutex(&lck->mtx);
	sched_yield();
    }
}

PUBLIC void 
release_read_lock (lock_obj_t *lck)
{
    grab_mutex(&lck->mtx);
    if (--lck->readers < 0) lck->readers = 0;
    release_mutex(&lck->mtx);
}

PUBLIC void 
grab_write_lock (lock_obj_t *lck)
{
    while (1) {
	grab_mutex(&lck->mtx);
        lck->write_pending = 1;
	if ((lck->readers <= 0) && (lck->writing == 0)) {
	    lck->writing = 1;
	    release_mutex(&lck->mtx);
	    return;
	}
	release_mutex(&lck->mtx);
	sched_yield();
    }
}

PUBLIC void 
release_write_lock (lock_obj_t *lck)
{
    grab_mutex(&lck->mtx);
    lck->write_pending = lck->writing = 0;
    release_mutex(&lck->mtx);
}

void
lock_obj_destroy (lock_obj_t *lck)
{
#ifdef USE_GNUC_CAS
    lck->mtx = 0;
#else
    pthread_mutex_destroy(&lck->mtx);
#endif
    memset(lck, 0, sizeof(lock_obj_t));
}

#ifdef __cplusplus
} // extern C
#endif 



