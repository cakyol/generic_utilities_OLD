
#include <stdio.h>
#include "dynamic_array_object.h"

#define EXCESS              10000
#define VALID_START         -5000
#define VALID_END           5000
#define SIZE                (VALID_END - VALID_START + 1)

int array[SIZE];

int main (int argc, char *argv[])
{
    dynamic_array_t dyn;
    int i, j, *data;
    int errors, failed;

    dynamic_array_init(&dyn, 0, 8, NULL);
    printf("initialized dynamic array\n");
    fflush(stdout);

    for (i = VALID_START, j = 0; i <= VALID_END; i++, j++)  {
        failed = dynamic_array_insert(&dyn, i, &array[j]);
        if (failed != 0) {
            printf("dynamic_array_insert failed at index %d\n", i);
            fflush(stdout);
        }
    }
    printf("inserted data into dynamic array\n");
    fflush(stdout);

    errors = j = 0;
    for (i = VALID_START - EXCESS; i <= (VALID_END + EXCESS); i++) {
        failed = dynamic_array_get(&dyn, i, (void**) &data);
        if ((i >= VALID_START) && (i <= VALID_END)) {
            if (failed || (data != &array[j])) {
                printf("invalid entry at index %d\n", i);
                errors++;
            }
            j++;
        } else {
            if (failed == 0) {
                printf("invalid entry unexpected at index %d\n", i);
                errors++;
            }
        }
        fflush(stdout);
    }
    printf("dynamic array is%s sane\n", errors ? " NOT" : "");

    return 0;
}
