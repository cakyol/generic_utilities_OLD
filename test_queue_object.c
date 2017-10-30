
#include "queue_object.h"

#define QUEUE_SIZE	1000
#define ITER_COUNT      (QUEUE_SIZE * 20)



int main (int argc, char *argv[])
{
    queue_obj_t qobj;
    int i, j, n_stored;

    if (queue_obj_init(&qobj, true, QUEUE_SIZE, 64, NULL)) {
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

    /* now read back and verify */
    i = 0;
    n_stored = qobj.n;
    while (SUCCEEDED(queue_obj_dequeue_integer(&qobj, &j))) {
        if (j == i) {
            printf("queue data %d %d verified\n", i, j);
        } else {
            fprintf(stderr, "dequeue data mismatch: dqed %d, should be %d\n",
                j, i);
        }
        i++;
    }
    assert((qobj.n == 0) && (i == n_stored));

    return OK;
}



