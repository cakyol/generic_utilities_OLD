
#include <stdio.h>
#include "radix_tree_object.h"
#include "timer_object.h"
#include "test_data_generator.h"

#define ITER                    4
#define MAX_DATA                (6 * 1024 * 1024)

timer_obj_t timr;
radix_tree_t radix_tree_obj;

int main (int argc, char *argv[])
{
    int iter, i, valid, failed, found, total;
    long long int mem;
    double megabytes;
    void *data, *found_data;
    int key_size = sizeof(int);

    printf("\nSIZE OF RADIX TREE NODE = %lu BYTES\n", sizeof(radix_tree_node_t));
    total = valid = failed = found = 0;
    radix_tree_init(&radix_tree_obj, 0, NULL);
    printf("\nPOPULATING RADIX TREE\n");
    timer_start(&timr);
    for (iter = 0; iter < ITER; iter++) {
        for (i = 1; i < MAX_DATA; i++) {
            total++;
            data = integer2pointer(i);
            if (radix_tree_insert(&radix_tree_obj, &i, key_size,
                    data, &found_data)) {
                        failed++;
            } else {
                valid++;
                if (found_data) found++;
            }
        }
    }
    timer_end(&timr);
    timer_report(&timr, total, NULL);
    printf("successfully added %d (found %d failed %d) of %d data (nodes %d)\n", 
        valid, found, failed, total, radix_tree_obj.node_count);
    OBJECT_MEMORY_USAGE(&radix_tree_obj, mem, megabytes);
    printf("total memory used is %llu bytes (%lf Mbytes)\n",
        mem, megabytes);

    printf("\nSEARCHING RADIX TREE\n");
    total = valid = failed = found = 0;
    timer_start(&timr);
    for (iter = 0; iter < ITER; iter++) {
        for (i = 1; i < MAX_DATA; i++) {
            total++;
            if (radix_tree_search(&radix_tree_obj, &i, key_size, &found_data)) {
                failed++;
            } else {
                found++;
                if (data == found_data) valid++;
            }
        }
    }
    timer_end(&timr);
    timer_report(&timr, total, NULL);
    printf("successfully found %d valid entries out of a total of %d entries\n",
        valid, total);

    return 0;
}



