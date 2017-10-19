
#include "queue_object.h"

#define QUEUE_SIZE	100000

int main (int argc, char *argv[])
{
    queue_obj_t qobj;
    int i;
    datum_t queued_data, dequeued_data, old_dequeued_data;
    int queued_count, dequeued_count;
    int qblock_count, dqblock_count;
    int filled = 0;

    if (queue_obj_init(&qobj, true, QUEUE_SIZE)) {
	fprintf(stderr, "queue_obj_init failed\n");
	return -1;
    }

    queued_data.integer = 0;
    old_dequeued_data.integer = -1;
    queued_count = dequeued_count = 0;
    qblock_count = dqblock_count = 0;
    while (1) {

	/* queue faster than dequeue */
	if (!filled) {
	    for (i = 0; i < QUEUE_SIZE/500; i++) {
		if (queue_obj_queue(&qobj, queued_data) == OK) {
		    queued_data.integer++;
		    queued_count++;
		} else {
		    filled = 1;
		    break;
		}
	    }
	    qblock_count++;
	    printf("queued %d times\n", i);
	}

	for (i = 0; i < QUEUE_SIZE/700; i++) {
	    if (queue_obj_dequeue(&qobj, &dequeued_data) != OK) {
		goto all_done;
	    }
	    if (dequeued_data.integer != (old_dequeued_data.integer + 1)) {
		fprintf(stderr, "data mismatch, read %d expected %d\n",
			dequeued_data.integer, old_dequeued_data.integer + 1);
		return ERROR;
	    }
	    old_dequeued_data = dequeued_data;
	    dequeued_count++;
	}
	dqblock_count++;
	printf("dequeued %d times\n", i);
    }

all_done:
    printf("queue object ok, queued %d (%d) times, dequeued %d (%d) times\n",
	    qblock_count, queued_count, dqblock_count, dequeued_count);
    return OK;
}



