
#include "queue_object.h"

#define QUEUE_SIZE	                750000
#define QUEUE_EXPANSION_INCREMENT       128
#define ITER_COUNT                      (QUEUE_SIZE - 5)

int main (int argc, char *argv[])
{
    queue_obj_t qobj;
    int i, j, n_stored;
    int64 bytes;
    double mbytes;
    timer_obj_t timr;

    if (queue_obj_init(&qobj, true, QUEUE_SIZE, QUEUE_EXPANSION_INCREMENT, NULL)) {
	fprintf(stderr, "queue_obj_init failed\n");
	return -1;
    }

    /* fill up the fifo */
    start_timer(&timr);
    printf("Populating the queue\n");
    fflush(stdout);
    for (i = 0; i < ITER_COUNT; i++) {
        if (FAILED(queue_obj_queue_integer(&qobj, i))) {
            fprintf(stderr, "queueing %d failed\n", i);
            return -1;
        }
    }
    end_timer(&timr);
    report_timer(&timr, ITER_COUNT);

    n_stored = qobj.n;
    OBJECT_MEMORY_USAGE(&qobj, bytes, mbytes);

    /* now read back and verify */
    i = 0;
    printf("Now dequeuing & verifying\n");
    start_timer(&timr);
    while (SUCCEEDED(queue_obj_dequeue_integer(&qobj, &j))) {
        if (j == i) {
            // printf("queue data %d %d verified\n", i, j);
        } else {
            fprintf(stderr, "dequeue data mismatch: dqed %d, should be %d\n",
                j, i);
        }
        i++;
    }
    end_timer(&timr);
    report_timer(&timr, i);
    assert((qobj.n == 0) && (i == n_stored));
    printf("\nqueue object is sane\n  capacity %d\n  expanded %d times\n"
            "  memory %lld bytes %f mbytes\n",
        qobj.maximum_size, qobj.expansion_count, bytes, mbytes);
    return OK;
}



