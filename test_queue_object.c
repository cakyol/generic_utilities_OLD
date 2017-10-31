
#include "queue_object.h"

#define QUEUE_SIZE	                1000
#define QUEUE_EXPANSION_INCREMENT       128
#define ITER_COUNT                      (QUEUE_SIZE * 100)



int main (int argc, char *argv[])
{
    queue_obj_t qobj;
    int i, j, n_stored;
    int64 bytes;
    double mbytes;

    if (queue_obj_init(&qobj, true, QUEUE_SIZE, QUEUE_EXPANSION_INCREMENT, NULL)) {
	fprintf(stderr, "queue_obj_init failed\n");
	return -1;
    }

    /* fill up the fifo */
    for (i = 0; i < ITER_COUNT; i++) {
        if (FAILED(queue_obj_queue_integer(&qobj, i))) {
            fprintf(stderr, "queueing %d failed\n", i);
            return -1;
        }
    }
    OBJECT_MEMORY_USAGE(&qobj, bytes, mbytes);

    /* now read back and verify */
    i = 0;
    n_stored = qobj.n;
    while (SUCCEEDED(queue_obj_dequeue_integer(&qobj, &j))) {
        if (j == i) {
            // printf("queue data %d %d verified\n", i, j);
        } else {
            fprintf(stderr, "dequeue data mismatch: dqed %d, should be %d\n",
                j, i);
        }
        i++;
    }
    assert((qobj.n == 0) && (i == n_stored));
    printf("queue object is sane, capacity %d, expanded %d times, "
            "memory %lld bytes %f mbytes\n",
        qobj.maximum_size, qobj.expansion_count, bytes, mbytes);
    return OK;
}



