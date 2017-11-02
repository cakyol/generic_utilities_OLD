
#include "lock_object.h"

#define ARRAY_SIZE          100000000
#define MAX_THREADS         512
#define LOCK_COUNT          1

int array [ARRAY_SIZE];
lock_obj_t lock;

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
    fflush(stdout);
    fflush(stdout);
    for (i = 0; i < LOCK_COUNT; i++) release_write_lock(&lock);
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
    }
    fflush(stdout);
    fflush(stderr);
    while (1);
    return 0;
}




