
#include "lock_object.h"

#define ARRAY_SIZE          100000000
#define MAX_THREADS         256
#define LOCK_COUNT          100

int array [ARRAY_SIZE];
lock_obj_t lock;
byte thread_complete_array [MAX_THREADS] = { 0 };

void *thread_function (void *arg)
{
    int *intp = (int*) arg;
    int value = *intp;
    int i, failures;

    free(intp);
    for (i = 0; i < LOCK_COUNT; i++) grab_write_lock(&lock);
    printf("filling array with value %d\n", value);
    for (i = 0; i < ARRAY_SIZE; i++) {
        array[i] = value;
    }
    printf("now validating array for value %d\n", value);
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
    for (i = 0; i < LOCK_COUNT; i++) release_write_lock(&lock);
    if (thread_complete_array[value] != 0) {
        fprintf(stderr, "OOOPPPPS, have revisited %d\n", value);
    }
    thread_complete_array[value] = 1;
    fflush(stdout);
    fflush(stderr);
    return NULL;
}

void *validate_array_thread (void *arg)
{
    int i, value, failures;

    for (i = 0; i < LOCK_COUNT; i++) grab_read_lock(&lock);
    value = array[0];
    printf("READ validating now for value %d\n", value);
    failures = 0;
    for (i = 0; i < ARRAY_SIZE; i++) {
        if (array[i] != value) failures++;
    }
    if (failures) {
        printf("READ validation FAILED for value %d: %d entries\n",
            value, failures);
    } else {
        printf("READ validation PASSED for value %d\n", value);
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

    rv = lock_obj_init(&lock);
    if (rv) {
        fprintf(stderr, "lock_obj_init failed: <%s>\n", strerror(rv));
        return -1;
    }

    for (i = 0; i < MAX_THREADS; i++) {
        intp = malloc(sizeof(int));
        if (intp) {
            *intp = i;
        } else {
            fprintf(stderr, "malloc FAILED\n");
        }
        rv = pthread_create(&tid, NULL, thread_function, intp);
        if (rv) {
            fprintf(stderr, "pthread_create FAILED at iteration %d\n", i);
        }

        /* every other thread, also create a read thread to validate */
        if (i & 1) { 
            rv = pthread_create(&tid, NULL, validate_array_thread, NULL);
            if (rv) {
                fprintf(stderr, "pthread_create FAILED for validation\n");
            }
        }
    }
    fflush(stdout);
    fflush(stderr);
    while (1) {
        not_all_threads_complete:
        for (i = 0; i < MAX_THREADS; i++) {
            if (thread_complete_array[i] == 0) {
                fprintf(stderr, "thread array %d not done yet\n", i);
                sleep(1);
                goto not_all_threads_complete;
            }
        }
        printf("all threads have finished, size of one lock object is %lu bytes\n",
                sizeof(lock_obj_t));
        fflush(stdout);
        return 0;
    }
    return 0;
}




