
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

#define compare_and_swap(variable, checked, set) \
    __sync_val_compare_and_swap((variable), (checked), (set))

#define PUBLIC

/******* Public functions start here *****************************************/

PUBLIC int 
lock_obj_init (volatile lock_obj_t *lck)
{
    memset((void*) lck, 0, sizeof(lock_obj_t));
    return 0;
}

PUBLIC void 
grab_read_lock (volatile lock_obj_t *lck)
{
    while (1) {
        if (compare_and_swap(&lck->mtx, 0, 1) == 0) {
            if (lck->write_pending == 0) {
                lck->readers++;
                lck->mtx = 0;
                return;
            }
            lck->mtx = 0;
        }
        sched_yield();
    }
}

PUBLIC void 
release_read_lock (volatile lock_obj_t *lck)
{
    while (1) {
        if (compare_and_swap(&lck->mtx, 0, 1) == 0) {
            if (--lck->readers < 0) lck->readers = 0;
            lck->mtx = 0;
            return;
        }
        sched_yield();
    }
}

PUBLIC void 
grab_write_lock (volatile lock_obj_t *lck)
{
    while (1) {
        if (compare_and_swap(&lck->mtx, 0, 1) == 0) {
            lck->write_pending = 1;
            if ((lck->readers <= 0) && (lck->writing == 0)) {
                lck->writing = 1;
                lck->mtx = 0;
                return;
            }
            lck->mtx = 0;
        }
        sched_yield();
    }
}

PUBLIC void 
release_write_lock (volatile lock_obj_t *lck)
{
    while (1) {
        if (compare_and_swap(&lck->mtx, 0, 1) == 0) {
            lck->write_pending = lck->writing = 0;
            lck->mtx = 0;
            return;
        }
        sched_yield();
    }
}

void
lock_obj_destroy (lock_obj_t *lck)
{
    memset(lck, 0, sizeof(lock_obj_t));
}

#ifdef __cplusplus
} // extern C
#endif 



