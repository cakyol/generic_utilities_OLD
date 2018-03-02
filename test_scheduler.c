
#include <stdio.h>
#include "scheduler.h"

void task_function (void *arg) 
{
    int value = *((int*) arg);

    printf("%d ", value);
    fflush(stdout);
}

int main (int argc, char *argv[])
{
    int rv, i;
    task_t *tptr;

    rv = task_scheduler_init();
    if (rv) {
	fprintf(stderr, "initialize_task_scheduler failed\n");
	return -1;
    }
    for (i = 0; i < 100; i++) {
	rv = task_schedule(i+2, 0, task_function, &i, &tptr);
	if (rv) {
	    fprintf(stderr, "schedule_task at iteration %d failed\n", i);
	} else {
	    printf("task %d scheduled for %lld\n",
		i, tptr->abs_firing_time_nsecs);
	}
    }
    while (1) sleep(1);
    return 0;
}








