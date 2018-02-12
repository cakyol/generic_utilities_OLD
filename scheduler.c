
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

#ifdef __cplusplus
extern "C" {
#endif

#include "scheduler.h"

/*
 * It is very important to understand why there are two lists separated
 * as shown below.  When a user schedules a task into the future, it gets
 * placed into the 'scheduled_tasks' list.  When the timers fire and we want
 * to pick up tasks from 'scheduled_tasks' lists to execute, we must first 
 * place them into ANOTHER list, which is the 'executable_tasks' list.
 * The reason for this is the following.  Typically, when a task is executed,
 * if it wants a repeated execution, it will schedule itself again into the 
 * future.  When we execute tasks, we lock the list to stop any list corruption
 * while all the tasks complete.  So, if an executing task wants to reschedule
 * itself, it will attempt to place a new task into the already locked list.
 * This will cause a deadlock.  Therefore, the execution list MUST be separate
 * from the scheduling list so that a task indeed CAN (without a deadlock)
 * reschedule itself again to be excuted in the future.  Basically, the
 * scheduled_tasks list MUST be open to write into when we are executing
 * from the executable_tasks_list.
 */
static linkedlist_t scheduled_tasks_list;
static linkedlist_t *scheduled_tasks = &scheduled_tasks_list;
static linkedlist_t executable_tasks_list;
static linkedlist_t *executable_tasks = &executable_tasks_list;

/*
 * tasks are ordered based on their firing time values which is in
 * nanoseconds.
 */
static int
compare_tasks (void *vt1, void *vt2)
{
    task_t *t1 = (task_t*) vt1;
    task_t *t2 = (task_t*) vt2;

    /*
     * do not do a simple subtraction here since if the values are
     * too far apart to overflow an integer, we will have wrong
     * values.  Instead compare directly.
     */
    if (t1->abs_firing_time_nsecs == t2->abs_firing_time_nsecs) return 0;
    if (t1->abs_firing_time_nsecs < t2->abs_firing_time_nsecs) return -1;
    return 1;
}

/*
 * the next task's ('later_task') execution time is close enuf (within 
 * resolution time) to the execution time of the 'first_task' so that 
 * they can be assumed to execute at the same time.
 */
static inline int
within_resolution (task_t *first_task, task_t *later_task)
{
    int diff = 
	later_task->abs_firing_time_nsecs - 
	first_task->abs_firing_time_nsecs;
    return (diff >= -RESOLUTION_NSECS) && (diff <= RESOLUTION_NSECS);
}

static int 
schedule_next_alarm_signal (long long int interval)
{
    struct itimerval itim;

    itim.it_interval.tv_sec = 0;
    itim.it_interval.tv_usec = 0;
    itim.it_value.tv_sec = interval / SEC_TO_NSEC_FACTOR;
    itim.it_value.tv_usec = (interval % SEC_TO_NSEC_FACTOR) / 1000;
    return
	setitimer(ITIMER_REAL, &itim, NULL);
}

static void
__alarm_signal_handler (int signo)
{
    int rv;
    linkedlist_node_t *d;
    task_t *tp, *first_task = NULL;
    struct timespec current_time;
    long long int current_time_nsec;

    /* some sanity checking */
    assert(SIGALRM == signo);

    /* re-instate the alarm handler again for next time */
    signal(SIGALRM, __alarm_signal_handler);

    /* we must do this since we will manipulate lists */
    WRITE_LOCK(scheduled_tasks);
    WRITE_LOCK(executable_tasks);

    /* if no task, finish (spurious alarm ?) */
    if (scheduled_tasks->n <= 0) {
	WRITE_UNLOCK(executable_tasks);
	WRITE_UNLOCK(scheduled_tasks);
	return;
    }

    /*
     * first transfer all tasks at the head of the scheduler list
     * which fall approximately into the range of the timer resolution,
     * to the execution list one at a time.  Check the comment at around
     * line number 30 to understand this carefully.  It is CRITICAL.
     */
    first_task = (task_t*) scheduled_tasks->head->user_data;
    while (not_endof_linkedlist(scheduled_tasks->head)) {
	tp = (task_t*) scheduled_tasks->head->user_data;
	if (within_resolution(first_task, tp)) {

	    /* remove task from head of scheduled_tasks list */
	    d = scheduled_tasks->head;
	    scheduled_tasks->head = d->next;
	    scheduled_tasks->n--;
	    free(d);

	    /*
	     * now txfer it to the executable_tasks list.
	     * Make sure we use the 'raw' unprotected function
	     * here since this list is already write locked and
	     * if we attempt to lock it again using one of the
	     * public functions, we will get a write deadlock.
	     */
	    rv = thread_unsafe_linkedlist_add_to_head(executable_tasks, tp);
	} else {
	    break;
	}
    }

    /* unlock scheduler tasks lists in case during the executions in the
     * executable_tasks lists, some functions reschedule themselves back
     * into the scheduler task list.  This is why the scheduler tasks list
     * opened again for writing but the executable tasks list is still
     * write locked so we can start executing from it one at a time.
     */
    WRITE_UNLOCK(scheduled_tasks);

    /*
     * now execute all tasks in the executable_tasks list now one at a time.
     */
    while (not_endof_linkedlist(executable_tasks->head)) {
	tp = (task_t*) executable_tasks->head->user_data;
	tp->efn(tp->argument);
	d = executable_tasks->head;
	executable_tasks->head = d->next;
	executable_tasks->n--;
	free(d);
    }

    /*
     * now schedule the alarm signal to fire for the next task.  The
     * next task is now at the head of the scheduled_tasks list.
     */
    READ_LOCK(scheduled_tasks);
    if (scheduled_tasks->n > 0) {
	tp = (task_t*) scheduled_tasks->head->user_data;

	/* what is the current time ? */
	clock_gettime(CLOCK_MONOTONIC, &current_time);
	current_time_nsec = 
	    (current_time.tv_sec * SEC_TO_NSEC_FACTOR) + current_time.tv_nsec;
	schedule_next_alarm_signal(tp->abs_firing_time_nsecs - current_time_nsec);
    }
    READ_UNLOCK(scheduled_tasks);
}

/*
 * The 'executable_tasks_list' does not have any ordering in it.
 * It is simply used as a list.  So, the compare function always
 * returns 0 for it.  Every entry will always match.
 */
static int executable_tasks_list_comparer (void *vt1, void *vt2)
{ return 0; }

int
initialize_task_scheduler (void)
{
    int rv;

    /* install the alarm signal handler */
    if (SIG_ERR == signal(SIGALRM, __alarm_signal_handler))
	return errno;

    rv = linkedlist_init(executable_tasks, 1, executable_tasks_list_comparer, NULL);
    if (rv) return rv;
    rv = linkedlist_init(scheduled_tasks, 1, compare_tasks, NULL);
    return rv;
}

/*
 * schedule function 'fn' to be called in future with
 * the argument specified in 'arg'.
 */
task_t * 
schedule_task (int secs, int usecs, simple_function_pointer fn, void *arg)
{
    return NULL;
}

#ifdef __cplusplus
} // extern C
#endif









