
#include "stack_object.h"

#define STACK_SIZE	1000
#define STACK_GROWTH	500

int main (int argc, char *argv[])
{
    stack_obj_t stk;
    int i, data, last_pushed_data, returned_data;

    if (stack_obj_init(&stk, true, STACK_SIZE, STACK_GROWTH)) {
	fprintf(stderr, "stack_obj_init failed\n");
	return -1;
    }

    /* push all data */
    for (i = 0, data = 0; i < 5*STACK_SIZE; i++, data += 3) {
	if (stack_obj_push_integer(&stk, data) != OK) {
	    fprintf(stderr, "stack_obj_push_integer of data %d failed\n", data);
	    return -1;
	}
	last_pushed_data = data;
    }

    /* now pop them & verify */
    for (i = 0, data = last_pushed_data; i < 5*STACK_SIZE; i++, data -= 3) {
	if (stack_obj_pop_integer(&stk, &returned_data) != OK) {
	    fprintf(stderr, "stack_obj_pop_integer of iteration %d failed\n", i);
	    return -1;
	}
	if (returned_data != data) {
	    fprintf(stderr, "returned data %d doesnt match %d for iteration %d\n",
		returned_data, data, i);
	}
    }

    printf("stack object works fine; expanded %d times\n", stk.expansion_count);
    return 0;
}



