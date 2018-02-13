
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol, gee.akyol@gmail.com, gee_akyol@yahoo.com
** Copyright: Cihangir Metin Akyol, April 2014 -> ....
**
** All this code has been personally developed by and belongs to 
** Mr. Cihangir Metin Akyol.  It has been developed in his own 
** personal time using his own personal resources.  Therefore,
** it is NOT owned by any establishment, group, company or 
** consortium.  It is the sole property and work of the named
** individual.
**
** It CAN be used by ANYONE or ANY company for ANY purpose as long 
** as ownership and/or patent claims are NOT made to it by ANYONE
** or ANY ENTITY.
**
** It ALWAYS is and WILL remain the property of Cihangir Metin Akyol.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <signal.h>
#include <sys/time.h>
#include "function_types.h"
#include "timer_object.h"
#include "linkedlist.h"

typedef long long int nano_seconds_t;

/* second to nanosecond multiplier */
#define SEC_TO_NSEC_FACTOR			(1000000000LL)

/* one hundred milliseconds in nano seconds */
#define ONE_HUNDRED_MSEC_IN_NSEC		(100000000LL)

/*
 * tasks within this much spacing is assumed
 * to approximately fire all at the same time,
 * this is the lowest granularity which we can
 * separate the firings.  No less.
 */
#define RESOLUTION_NSECS			(ONE_HUNDRED_MSEC_IN_NSEC)

/*
 * A task has an absolute execution time, a function to execute
 * when its timer expires and an argument to pass to that execution
 * function.  When a task is scheduled, the current time is added to
 * its future execution time and placed in 'abs_firing_time_nsecs'.
 * The list is ordered in increasing number of this value.
 */
typedef struct task_s {

    nano_seconds_t abs_firing_time_nsecs;
    simple_function_pointer efn;
    void *argument;

} task_t;

extern int
initialize_task_scheduler (void);

/*
 * schedule function 'fnp' to be executed in the future by the specified
 * amount, with the specified argument.  Return value is a void * which
 * should be used as a handle in case the event needs to be cancelled
 * before it takes place.  A NULL return value means scheduling did not
 * succeed.  This handle is placed into 'returned_task_handle'.  The
 * function return value is 0 for success or an errno if could not be
 * scheduled.
 */
extern int
schedule_task (int seconds_from_now, nano_seconds_t nano_seconds_from_now,
        simple_function_pointer fnp, void *argument,
        task_t *task_handle);

extern int 
cancel_task (task_t *task_handle);

#ifdef __cplusplus
} // extern C
#endif

#endif // __SCHEDULER_H__



