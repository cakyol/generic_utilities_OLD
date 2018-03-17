
#include <stdio.h>
#include "ntrie_object.h"
#include "timer_object.h"
#include "test_data_generator.h"

#define DATA_SIZE	(32 * 1024 * 1024)

timer_obj_t timr;

int main (argc, argv)
int argc;
char *argv [];
{
    int data_count = DATA_SIZE;
    test_data_t *test_data = generate_test_data(&data_count);
    int rv, i, total, valid, failed;
    void *found;
    long long int mem;
    double megabytes;
    ntrie_t ntrie_obj;

    assert(test_data);
    printf("\nPOPULATING TRIE\n");
    ntrie_init(&ntrie_obj, 1, NULL);
    printf("\nSIZE OF NTRIE NODE = %lu BYTES\n", sizeof(ntrie_node_t));
    total = valid = failed = 0;
    timer_start(&timr);
    for (i = 0; i < data_count; i++) {
	total++;
	rv = ntrie_insert(&ntrie_obj, test_data[i].key, KEY_SIZE,
		&test_data[i], &found);
	if (rv) {
	    failed++;
	} else {
	    valid++;
	}
    }
    timer_end(&timr);
    timer_report(&timr, total);
    printf("successfully added %d (failed %d) of %d data to trie (nodes %d)\n", 
	valid, failed, total, ntrie_obj.node_count);
    OBJECT_MEMORY_USAGE(&ntrie_obj, mem, megabytes);
    printf("total memory used is %llu bytes (%lf Mbytes)\n",
	mem, megabytes);

#if 0
    printf("\nSEARCHING ALL AVAILABLE STRINGS\n");
    count = total = 0;
    timer_start(&timr);
    for (iter = 0; iter < ITER; iter++) {
        FOR_ALL_POSSIBLE_STRINGS
            total++;
            if (ntrie_search(&ntrie_obj, string, SIZE, &found) == 0) {
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



