
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
#include <stdio.h>

extern int
thread_unsafe_linkedlist_add_to_head (linkedlist_t *listp, void *user_data);

extern int
thread_unsafe_linkedlist_node_delete (linkedlist_t *listp,
    linkedlist_node_t *node_tobe_deleted);

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

static itimerval_t zero_itim = { { 0, 0 }, { 0, 0 } };

/*
 * tasks to be executed this close to each other will all be
 * executed immediately after one another.  This is basically
 * the minimum resolution of this timer system.
 */
#define RESOLUTION_MILLI_SECONDS        20
#define RESOLUTION_NANO_SECONDS         (RESOLUTION_MILLI_SECONDS * 1000000LL)  

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
 * they can be assumed to execute at the same time.  If 'first_task'
 * is NULL, then the current time is assumed.  The return value is
 * when the alarm should be fired in nano seconds from now.
 */
static nano_seconds_t
next_firing_time (task_t *first_task, task_t *later_task,
    nano_seconds_t lateness)
{
    nano_seconds_t now, next_firing;

    if (first_task) {
        now = first_task->abs_firing_time_nsecs;
    } else {
        now = time_now();
    }
    next_firing = later_task->abs_firing_time_nsecs - now - lateness;
    if (next_firing <= RESOLUTION_NANO_SECONDS)
        next_firing = RESOLUTION_NANO_SECONDS;
    return next_firing;
}

static inline int
within_resolution (task_t *first_task, task_t *later_task)
{
    if (first_task == later_task)
        return 1;
    return
        next_firing_time(first_task, later_task, 0) <= 
            RESOLUTION_NANO_SECONDS;
}

/*
 * Schedules an alarm to go off 'interval' nanoseconds from 
 * the time this function is called.
 */
static int 
schedule_next_alarm_nsecs (nano_seconds_t interval)
{
    itimerval_t itim;

    itim.it_interval.tv_sec = 0;
    itim.it_interval.tv_usec = 0;
    itim.it_value.tv_sec = interval / SEC_TO_NSEC_FACTOR;
    itim.it_value.tv_usec = (interval % SEC_TO_USEC_FACTOR);
    fflush(stdout);
    return
        setitimer(ITIMER_REAL, &itim, NULL);
}

static inline int
terminate_alarm (void)
{
    return
        setitimer(ITIMER_REAL, &zero_itim, NULL);
}

/*
 * This function is called whenever a change occurs and we know that
 * we have to re-schedule the next alarm signal.  It always checks
 * the first task at the front of the scheduler list to calculate
 * and set the alarm to its execution time.  It should be called
 * whenever the task scheduler has changed, such as, a new task has
 * been added, a task has been cancelled etc.
 */
static void
reschedule_next_alarm (nano_seconds_t lateness)
{
    task_t *tp;

    READ_LOCK(scheduled_tasks);

    /* no task scheduled, stop the alarms */
    if (scheduled_tasks->n <= 0) {
        terminate_alarm();
        READ_UNLOCK(scheduled_tasks);
        return;
    }

    /*
     * if we are here, we have at least one task to be scheduled.
     * So, calculate its time and schedule it into the future.
     */
    tp = (task_t*) scheduled_tasks->head->user_data;
    schedule_next_alarm_nsecs(next_firing_time(NULL, tp, lateness));
    READ_UNLOCK(scheduled_tasks);
}

static void
__alarm_signal_handler (int signo)
{
    int rv;
    linkedlist_node_t *d;
    task_t *tp, *first_task = NULL;
    timer_obj_t execution_time;

    /*
     * start timing the delay this function call will introduce
     * so that we can compensate this delay when we schedule the
     * next alarm signal.
     */
    timer_start(&execution_time);

    /* some sanity checking */
    assert(SIGALRM == signo);

    /* re-instate the alarm handler again for next time */
    signal(SIGALRM, __alarm_signal_handler);

    /* we must do this since we will manipulate both lists */
    WRITE_LOCK(scheduled_tasks);
    WRITE_LOCK(executable_tasks);

    /* if no task, finish (spurious alarm ?) */
    if (scheduled_tasks->n <= 0) {
        WRITE_UNLOCK(executable_tasks);
        WRITE_UNLOCK(scheduled_tasks);
        return;
    }

    /*
     * first transfer all tasks from the head of the scheduler list
     * which fall approximately into the range of the timer resolution,
     * to the execution list one at a time.  Check the comment at around
     * line number 30 to understand this carefully.  It is CRITICAL.
     */
    first_task = (task_t*) scheduled_tasks->head->user_data;
    while (scheduled_tasks->n > 0) {
        tp = (task_t*) scheduled_tasks->head->user_data;
        if (within_resolution(first_task, tp)) {

            /* remove task from head of scheduled_tasks list */
            d = scheduled_tasks->head;
            scheduled_tasks->head = d->next;
            scheduled_tasks->n--;
            MEM_MONITOR_FREE(scheduled_tasks, d);

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

    /*
     * unlock scheduler tasks lists in case during the executions in the
     * executable_tasks lists, some functions reschedule themselves back
     * into the scheduler task list.  This is why the scheduler tasks list
     * opened again for writing but the executable tasks list is still
     * write locked so we can start executing from it one at a time.
     */
    WRITE_UNLOCK(scheduled_tasks);

    /*
     * now execute all tasks in the executable_tasks list now one at a time
     * picking them from the head of the list and deleting them from the
     * list as we execute them.
     */
    while (executable_tasks->n > 0) {
        tp = (task_t*) executable_tasks->head->user_data;
        tp->efn(tp->argument);
        free(tp);
        d = executable_tasks->head;
        executable_tasks->head = d->next;
        executable_tasks->n--;
        MEM_MONITOR_FREE(executable_tasks, d);
    }

    /* ok we executed everything, unlock now */
    WRITE_UNLOCK(executable_tasks);

    /*
     * record our finishing time so we can calculate the
     * delay introduced by all the work we did above.
     */
    timer_end(&execution_time);

    /* set the next alarm firing time for the rest of the tasks */
    reschedule_next_alarm(timer_delay_nsecs(&execution_time));
}

/*
 * The 'executable_tasks_list' does not have any ordering in it.
 * It is simply used as a list.  So, the compare function always
 * returns 0 for it.  Every entry will always match.
 */
static int dummy_comparer (void *vt1, void *vt2)
{ return 0; }

int
task_scheduler_init (void)
{
    int rv;

    /* install the alarm signal handler */
    if (SIG_ERR == signal(SIGALRM, __alarm_signal_handler))
        return errno;

    rv = linkedlist_init(executable_tasks, 1, dummy_comparer, NULL);
    if (rv) return rv;
    rv = linkedlist_init(scheduled_tasks, 1, compare_tasks, NULL);
    return rv;
}

/*
 * schedule function 'fn' to be called in future with
 * the argument specified in 'arg'.  If the scheduling
 * process is such that the new task gets inserted into
 * the beginning of the scheduled_tasks list, it also
 * ensures that the next alarm time is recalculated & set
 * accordingly.
 */
int
task_schedule (int seconds_from_now, nano_seconds_t nano_seconds_from_now,
        simple_function_pointer efn, void *argument,
        task_t **scheduled_task)
{
    task_t *tp = (task_t*) malloc(sizeof(task_t));
    linkedlist_node_t *first;
    int rv, head_changed;

    *scheduled_task = NULL;
    if (NULL == tp) return ENOMEM;
    tp->efn = efn;
    tp->argument = argument;
    tp->abs_firing_time_nsecs = time_now() + 
            (seconds_from_now * SEC_TO_NSEC_FACTOR) + nano_seconds_from_now;
    READ_LOCK(scheduled_tasks);
    first = scheduled_tasks->head;
    READ_UNLOCK(scheduled_tasks);
    rv = linkedlist_add(scheduled_tasks, tp);
    READ_LOCK(scheduled_tasks);
    head_changed = first != scheduled_tasks->head;
    READ_UNLOCK(scheduled_tasks);
    if (0 == rv) {
        *scheduled_task = tp;
        if (head_changed) reschedule_next_alarm(0);
    } else {
        free(tp);
    }
    return rv;
}

int
task_cancel (task_t *tp)
{
    task_t *t;
    int rv = ENODATA, head_changed = 0;

    WRITE_LOCK(scheduled_tasks);
    FOR_ALL_LINKEDLIST_ELEMENTS(scheduled_tasks, t) {
        if (t == tp) {
            if (scheduled_tasks->head->user_data == (void*) tp) head_changed = 1;
            thread_unsafe_linkedlist_node_delete(scheduled_tasks, __n__);
            rv = 0;
            break;
        }
    }
    WRITE_UNLOCK(scheduled_tasks);
    if (head_changed) reschedule_next_alarm(0);
    return rv;
}

#ifdef __cplusplus
} // extern C
#endif









