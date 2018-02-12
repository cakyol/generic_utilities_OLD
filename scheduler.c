
#include <signal.h>
#include <sys/time.h>

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

    long long int abs_firing_time_nsecs;
    simple_function_pointer efn;
    void *argument;

} task_t;

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
    int rv;

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

static void
__alarm_signal_handler (int signo)
{
    int rv;
    linkedlist_node_t *d;
    task_t *tp, *first_task = NULL;
    timer_obj_t delay;
    long long int nsec_delay;

    /*
     * calculate how much time we spend in this entire function so
     * we can compensate for it when we schedule the next 
     * task in.  The excution time here will be deducted so that
     * the next set of events fire at the time nearest to as 
     * correct as it can be.
     */
    start_timer(&delay);

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

    /* execute all tasks in the executable_tasks list now one at a time */

    /*
     * record how much time we actually took to do all this work.
     * In trying to schedule the next firing of the alarm, we must
     * take this delay into account
     */
    end_timer(&delay);

    nsec_delay = timer_delay_nsecs(&delay);

    /* now schedule the next event (set the alarm to the future to fire)
     * to continue executing tasks when the time comes again.


}

/*
 * The 'executable_tasks_list' does not have ay ordering in it.
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
}









