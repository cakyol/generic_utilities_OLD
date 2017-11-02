
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

/*
 * Initialize a lock object.
 * Successfull execution returns 'OK'.
 */
PUBLIC error_t 
lock_obj_init (lock_obj_t *lck)
{
    lck->recursive_writes = 0;
    return
        pthread_rwlock_init(&lck->rwlock, NULL);
}

PUBLIC int 
grab_read_lock (lock_obj_t *lck)
{
    return
        pthread_rwlock_rdlock(&lck->rwlock);
}

PUBLIC int 
release_read_lock (lock_obj_t *lck)
{
    return
        pthread_rwlock_unlock(&lck->rwlock);
}

PUBLIC int 
grab_write_lock (lock_obj_t *lck)
{
    int rv;

again:
    rv = pthread_rwlock_trywrlock(&lck->rwlock);
    if (EBUSY == rv) {
        sched_yield();
        goto again;
    }
    if (EDEADLK == rv) {
        lck->recursive_writes++;
    }
    return 0;
}

PUBLIC int 
release_write_lock (lock_obj_t *lck)
{
    lck->recursive_writes--;
    if (lck->recursive_writes <= 0) {
        return
            pthread_rwlock_unlock(&lck->rwlock);
    }
    return 0;
}

PUBLIC int
lock_obj_destroy (lock_obj_t *lck)
{
    if (lck->recursive_writes > 0) {
        return EBUSY;
    }
    return
        pthread_rwlock_destroy(&lck->rwlock);
}



