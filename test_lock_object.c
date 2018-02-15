
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "timer_object.h"
#include "lock_object.h"

#define ARRAY_SIZE          10000000
#define MAX_THREADS         (16 * 1024)
#define MAX_ITERATION       50000000

/* locks are not recursive, do not set this to > 1 */
#define LOCK_COUNT          1

int array [ARRAY_SIZE];
lock_obj_t lock;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char thread_complete_array [MAX_THREADS] = { 0 };
int max_threads = 0;

void *thread_function (void *arg)
{
    int tid = *((int*) arg);
    int value = -tid * 12;
    int i, failures;

    free(arg);
    for (i = 0; i < LOCK_COUNT; i++) grab_write_lock(&lock);
    //printf("WRITE thread %d, data %d .. ", tid, value);
    fflush(stdout);
    fflush(stdout);
    //printf("filling array with value %d\n", value);
    for (i = 0; i < ARRAY_SIZE; i++) {
        array[i] = value;
    }
    //printf("now validating array for value %d\n", value);
    failures = 0;
    for (i = 0; i < ARRAY_SIZE; i++) {
        if (array[i] != value) failures++;
    }
    if (failures) {
        printf("validation FAILED for value %d: %d entries\n",
            value, failures);
    } else {
        //printf("validation PASSED for value %d\n", value);
    }
    if (thread_complete_array[tid] != 0) {
        printf("OOOPPPPS, have revisited thread %d\n", tid);
    }
    thread_complete_array[tid] = 1;
    fflush(stdout);
    for (i = 0; i < LOCK_COUNT; i++) release_write_lock(&lock);
    return NULL;
}

void *validate_array_thread (void *arg)
{
    int i, value, failures;
    int tid = *((int*) arg);

    free(arg);
    for (i = 0; i < LOCK_COUNT; i++) grab_read_lock(&lock);
    value = array[0];
    //printf("READ thread %d validating now for value %d\n ..", tid, value);
    fflush(stdout);
    failures = 0;
    for (i = 0; i < ARRAY_SIZE; i++) {
        if (array[i] != value) failures++;
    }
    if (failures) {
        printf("FAILED for value %d: %d entries\n", value, failures);
    } else {
        //printf("done\n");
    }
    fflush(stdout);
    if (thread_complete_array[tid] != 0) {
        printf("OOOPPPPS, have revisited thread %d\n", tid);
    }
    thread_complete_array[tid] = 1;
    for (i = 0; i < LOCK_COUNT; i++) release_read_lock(&lock);
    return NULL;
}

int main (int argc, char *argv[])
{
    int rv;
    pthread_t tid;
    int i;
    int *intp;

    printf("\nsize of mutex object is %ld lock object is %ld bytes\n",
        sizeof(pthread_mutex_t), sizeof(lock_obj_t));

    rv = lock_obj_init(&lock);
    if (rv) {
        printf("lock_obj_init failed: <%s>\n", strerror(rv));
        return -1;
    }

#if 0
    timer_obj_t timr;

    /* first do a speed test for mutex ONLY */
    printf("performing a lock performance test\n");
    start_timer(&timr);
    for (i = 0; i < MAX_ITERATION; i++) {
        pthread_mutex_lock(&mutex);
        pthread_mutex_unlock(&mutex);
    }
    end_timer(&timr);
    printf("MUTEX ONLY:\n");
    report_timer(&timr, MAX_ITERATION);
    /* give time to view screen before scroll off begins below */
    sleep(3);

    /* now do a speed test for the actual lock itself */

    printf("performing a lock performance test\n");
    start_timer(&timr);
    for (i = 0; i < MAX_ITERATION; i++) {
        grab_write_lock(&lock);
        release_write_lock(&lock);
    }
    end_timer(&timr);
    printf("ENTIRE LOCK OBJECT:\n");
    report_timer(&timr, MAX_ITERATION);
    /* give time to view screen before scroll off begins below */
    sleep(3);
#endif

    /* create all the threads until no more can be created */
    printf("NOW TESTING FOR LOCK VERIFICATION\n");
    for (i = 0; i < MAX_THREADS; i++) {
        intp = malloc(sizeof(int));
        if (intp) {
            *intp = i;
        } else {
            printf("malloc FAILED\n");
            break;
        }
        if (i & 1) {
            rv = pthread_create(&tid, NULL, validate_array_thread, intp);
            if (rv) break;
            max_threads++;
        } else {
            rv = pthread_create(&tid, NULL, thread_function, intp);
            if (rv) break;
            max_threads++;
        }
    }

    fflush(stdout);

    /* now wait until they all complete */
not_all_threads_complete:
    for (i = 0; i < max_threads; i++) {
	if (thread_complete_array[i] == 0) {
	    sleep(5);
	    goto not_all_threads_complete;
	}
    }
    printf("all %d threads have finished\n", max_threads);
    fflush(stdout);
    return 0;
}




