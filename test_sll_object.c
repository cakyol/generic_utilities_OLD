
#include "sll_object.h"

#define START_INT   25
#define END_INT     500

static void
flush (void)
{
    fflush(stdout);
    fflush(stderr);
    sleep(1);
}

int main (int argc, char *argv[])
{
    error_t rv;
    int i, j, count;
    datum_t d1, d2;
    sll_object_t sll;

    rv = sll_object_init(&sll, false, compare_ints, NULL);
    if (rv) {
        fprintf(stderr, "sll_object_init failed: %d\n", rv);
        return rv;
    } else {
        printf("list initialized\n");
    }
    flush();

    for (i = START_INT; i <= END_INT; i++) {
        d1.integer = i;
        rv = sll_object_add_once(&sll, d1);
        if (rv) {
            fprintf(stderr, "adding %d failed\n", i);
        }
        rv = sll_object_add_once(&sll, d1);
        if (rv) {
            fprintf(stderr, "adding %d failed in 2nd attempt\n", i);
        }
        rv = sll_object_add_once(&sll, d1);
        if (rv) {
            fprintf(stderr, "adding %d failed in 3rd attempt\n", i);
        }
    }
    printf("all data (%d of them) added to list\n", sll.n);
    flush();

    assert(sll.n == (END_INT - START_INT + 1));

    /* now delete one at a time and make sure everything is consistent */
    printf("now deleting and verifying one at a time\n");
    flush();

    count = sll.n;
    for (i = START_INT; i <= END_INT; i++) {

        d1.integer = i;
        rv = sll_object_delete(&sll, d1, &d2);
        if (rv == 0) {

            count--;
            assert(count == sll.n);
            assert(d1.integer == d2.integer);

            /* 
             * make sure that the deleted entry is NOT in the list 
             * but ALL others are
             */
            for (j = START_INT; j <= END_INT; j++) {
                d1.integer = j;
                if (j <= i) {
                    assert(sll_object_search(&sll, d1, &d2) == ENODATA);
                } else {
                    assert(sll_object_search(&sll, d1, &d2) == 0);
                    assert(d1.integer == d2.integer);
                }
            }
        } else {
            fprintf(stderr, "sll_object_delete for %d failed\n", i);
        }
    }
    assert(sll.n == 0);

    return 0;
}


