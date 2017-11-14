
#include "sll_object.h"

#define START_INT   1
#define END_INT     4000
#define SIZE	    (END_INT + 1)

byte hashmap [SIZE] = { 0 };

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
    int result;
    int i, j, count;
    sll_object_t sll;
    sll_node_t *node;

    rv = sll_object_init(&sll, false, compare_ints, NULL);
    if (rv) {
        fprintf(stderr, "sll_object_init failed: %d\n", rv);
        return rv;
    } else {
        printf("list initialized\n");
    }
    flush();

    for (i = START_INT; i <= END_INT; i++) {
        rv = sll_object_add_once_integer(&sll, i);
        if (rv) {
            fprintf(stderr, "adding %d failed\n", i);
        }
	hashmap[i] = 1;
        rv = sll_object_add_once_integer(&sll, i);
        if (rv) {
            fprintf(stderr, "adding %d failed in 2nd attempt\n", i);
        }
        rv = sll_object_add_once_integer(&sll, i);
        if (rv) {
            fprintf(stderr, "adding %d failed in 3rd attempt\n", i);
        }
    }
    printf("all %d items added to list\n", sll.n);
    assert(sll.n == (END_INT - START_INT + 1));
    flush();

    printf("checking that the list is ordered\n");
    node = sll.head;
    result = -1000;
    while (not_sll_end_node(node)) {
        if (node->user_datum.integer < result) {
            fprintf(stderr, "list order incorrect, current: %lld expected: >= %d\n",
                node->user_datum.integer, result);
            result = node->user_datum.integer;
        }
        node = node->next;
    }


    /* now delete one at a time and make sure everything is consistent */
    printf("now deleting and verifying one at a time\n");
    flush();

    count = sll.n;
    int s, d;
    for (i = END_INT; i >= START_INT; i--) {

        rv = sll_object_delete_integer(&sll, i, &d);
        if (rv == 0) {
            count--;
            assert(count == sll.n);
            assert(i == d);
	    hashmap[i] = 0;

            /* 
             * make sure that the deleted entry is NOT in the list 
             * but ALL others are
             */
            for (j = START_INT; j <= END_INT; j++) {
		if (hashmap[j]) {
                    assert(sll_object_search_integer(&sll, j, &s) == 0);
                    assert(j == s);
		} else {
                    assert(sll_object_search_integer(&sll, j, &s) == ENODATA);
		}
            }
        } else {
            fprintf(stderr, "sll_object_delete for %d failed\n", i);
        }
    }
    assert(sll.n == 0);
    printf("all data verified, list is sane\n");

    return 0;
}


