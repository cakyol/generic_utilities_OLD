
#include "dynamic_array.h"

#define START	(-32)
#define END	(32)
#define EXTREME	100
#define EXTREME_DATA    123456

int main (int argc, char *argv[])
{
    dynamic_array_t dyn;
    int i;
    datum_t datum;

    dynamic_array_init(&dyn, false, 8, NULL);

    printf("initialized dynamic array\n");
    fflush(stdout);

    for (i = END; i >= START; i--) {
	datum.integer = i;
	if (SUCCEEDED(dynamic_array_insert(&dyn, i, datum))) {
            printf("inserted %d into index %d\n", i, i);
        } else {
	    printf("insert failed for %d\n", i);
	}
    }

    datum.integer = EXTREME_DATA;
    if (SUCCEEDED(dynamic_array_insert(&dyn, EXTREME, datum))) {
        printf("inserted %d into index %d\n", datum.integer, EXTREME);
    } else {
        printf("inserting %d into index %d FAILED\n", datum.integer, EXTREME);
    }

    datum.integer = EXTREME_DATA;
    if (SUCCEEDED(dynamic_array_insert(&dyn, -EXTREME, datum))) {
        printf("inserted %d into index %d\n", datum.integer, -EXTREME);
    } else {
        printf("inserting %d into index %d FAILED\n", datum.integer, -EXTREME);
    }

    for (i = -EXTREME-10; i < EXTREME+10; i++) {
	if (SUCCEEDED(dynamic_array_get(&dyn, i, &datum))) {
	    printf("value for %d is %d\n", i, datum.integer);
	} else {
	    printf("dynamic_array_get failed for entry %d\n", i);
	}
    }
    return 0;
}
