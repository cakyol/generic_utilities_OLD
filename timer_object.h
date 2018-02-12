
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Time measurement functions
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __TIMER_OBJ_H__
#define __TIMER_OBJ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>
#include <time.h>

#define NANOSEC_TO_SEC      ((long long int) 1000000000)

typedef struct timer_obj_s {

    struct timespec start, end;

} timer_obj_t;

#ifdef __APPLE__

#define CLOCK_REALTIME		1

static inline int
clock_gettime (int clock_id, struct timespec *ts)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) < 0) return -1;
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

#endif // __APPLE__

static inline void 
start_timer (timer_obj_t *tp)
{ clock_gettime(CLOCK_REALTIME, &tp->start); }

static inline void 
end_timer (timer_obj_t *tp)
{ clock_gettime(CLOCK_REALTIME, &tp->end); }

static inline long long int
timer_delay_nsecs (timer_obj_t *tp)
{
    return
	(tp->end.tv_sec * NANOSEC_TO_SEC) + tp->end.tv_nsec -
	(tp->start.tv_sec * NANOSEC_TO_SEC) + tp->start.tv_nsec;
}

extern 
void report_timer (timer_obj_t *tp, long long int iterations);

#ifdef __cplusplus
} // extern C
#endif 

#endif // __TIMER_OBJ_H__




