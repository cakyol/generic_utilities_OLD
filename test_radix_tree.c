
#include <stdio.h>
#include "radix_tree_object.h"
#include "timer_object.h"
#include "test_data_generator.h"

#define ITER			1
#define MAX_DATA		(6 * 1024 * 1024)

timer_obj_t timr;
radix_tree_t radix_tree_obj;

int main (int argc, char *argv[])
{
    int iter, i, rv, valid, failed, found_count;
    long long int mem;
    double megabytes;
    void *data, *found;
    int key_size = sizeof(void*);

    printf("\nSIZE OF NTRIE NODE = %lu BYTES\n", sizeof(radix_tree_node_t));
    valid = failed = found_count = 0;
    radix_tree_init(&radix_tree_obj, 1, NULL);
    printf("\nPOPULATING TRIE\n");
    timer_start(&timr);
    for (iter = 0; iter < ITER; iter++) {
	for (i = 0; i < MAX_DATA; i++) {
	    data = integer2pointer(i);
	    rv = radix_tree_insert(&radix_tree_obj, &data, key_size, data, &found);
	    if (rv) {
		failed++;
	    } else {
		valid++;
		if (found) found_count++;
	    }
	}
    }
    timer_end(&timr);
    timer_report(&timr, ITER * MAX_DATA);
    printf("successfully added %d (found %d failed %d) of %d data (nodes %d)\n", 
	valid, found_count, failed, ITER*MAX_DATA, radix_tree_obj.node_count);
    OBJECT_MEMORY_USAGE(&radix_tree_obj, mem, megabytes);
    printf("total memory used is %llu bytes (%lf Mbytes)\n",
	mem, megabytes);

#if 0
    printf("\nSEARCHING ALL AVAILABLE STRINGS\n");
    count = total = 0;
    timer_start(&timr);
    for (iter = 0; iter < ITER; iter++) {
        FOR_ALL_POSSIBLE_STRINGS
            total++;
            if (radix_tree_search(&radix_tree_obj, string, SIZE, &found) == 0) {
                count++;
            }
        }
    }
    timer_end(&timr);
    printf("found %d strings of %d strings\n", count, total);
    timer_report(&timr, total);
#endif

    return 0;
}


