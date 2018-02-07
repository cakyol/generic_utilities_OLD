
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "timer_object.h"
#include "lock_object.h"

#define ARRAY_SIZE          100000000
#define MAX_THREADS         128
#define MAX_ITERATION       50000000

/* locks are not recursive, do not set this to > 1 */
#define LOCK_COUNT          1

int array [ARRAY_SIZE];
lock_obj_t lock;
char thread_complete_array [MAX_THREADS] = { 0 };

void *thread_function (void *arg)
{
    int *intp = (int*) arg;
    int value = *intp;
    int i, failures;
    pthread_t tid = pthread_self();

    free(intp);
    grab_write_lock(&lock);
    printf("entered thread 0x%x, data %d .. ", (unsigned int) tid, value);
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
        printf("validation PASSED for value %d\n", value);
    }
    if (thread_complete_array[value] != 0) {
        printf("OOOPPPPS, have revisited %d\n", value);
    }
    thread_complete_array[value] = 1;
    fflush(stdout);
    release_write_lock(&lock);
    return NULL;
}

void *validate_array_thread (void *arg)
{
    int i, value, failures;

    for (i = 0; i < LOCK_COUNT; i++) grab_read_lock(&lock);
    value = array[0];
    //printf("READ validating now for value %d\n", value);
    failures = 0;
    for (i = 0; i < ARRAY_SIZE; i++) {
        if (array[i] != value) failures++;
    }
    if (failures) {
        printf("READ validation FAILED for value %d: %d entries\n",
            value, failures);
    } else {
        //printf("READ validation PASSED for value %d\n", value);
    }
    fflush(stdout);
    for (i = 0; i < LOCK_COUNT; i++) release_read_lock(&lock);
    return NULL;
}

int main (int argc, char *argv[])
{
    int rv;
    pthread_t tid;
    int i;
    int *intp;
    timer_obj_t timr;

    printf("\nsize of mutex object is %ld lock object is %ld bytes\n",
        sizeof(pthread_mutex_t), sizeof(lock_obj_t));

    rv = lock_obj_init(&lock);
    if (rv) {
        printf("lock_obj_init failed: <%s>\n", strerror(rv));
        return -1;
    }


    /* first do a speed test */
    printf("performing a lock performance test\n");
    start_timer(&timr);
    for (i = 0; i < MAX_ITERATION; i++) {
        grab_write_lock(&lock);
        release_write_lock(&lock);
    }
    end_timer(&timr);
    report_timer(&timr, MAX_ITERATION);
    /* give time to view screen before scroll off begins below */
    sleep(3);

    for (i = 0; i < MAX_THREADS; i++) {
        intp = malloc(sizeof(int));
        if (intp) {
            *intp = i;
        } else {
            printf("malloc FAILED\n");
        }
        rv = pthread_create(&tid, NULL, thread_function, intp);
        if (rv) {
            printf("pthread_create FAILED at iteration %d\n", i);
        }

#if 0
        /* every other thread, also create a read thread to validate */
        if (i & 1) { 
            rv = pthread_create(&tid, NULL, validate_array_thread, NULL);
            if (rv) {
                printf("pthread_create FAILED for validation\n");
            }
        }
#endif
    }
    fflush(stdout);
    while (1) {
        not_all_threads_complete:
        for (i = 0; i < MAX_THREADS; i++) {
            if (thread_complete_array[i] == 0) {
                //printf("thread array %d not done yet\n", i);
                sleep(1);
                goto not_all_threads_complete;
            } else {
                if (thread_complete_array[i] != 2) {
                    //printf("thread %d completed\n", i);
                    thread_complete_array[i] = 2;
                }
            }
        }
        printf("all threads have finished\n");
        fflush(stdout);
        return 0;
    }
    return 0;
}




