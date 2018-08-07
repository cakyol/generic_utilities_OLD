
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "linkedlist_object.h"
#include "pointer_manipulations.h"

#define START_INT   1
#define END_INT     4000
#define SIZE        (END_INT + 1)

byte hashmap [SIZE] = { 0 };

static void
flush (void)
{
    fflush(stdout);
    fflush(stderr);
    sleep(1);
}

int compare_pointers (void *ptr1, void *ptr2)
{
    return ((byte*) ptr1) - ((byte*) ptr2);
}

int main (int argc, char *argv[])
{
    int rv;
    int result;
    int i, j, count;
    linkedlist_t sll;
    linkedlist_node_t *node;
    void *value;
    void *found;

    rv = linkedlist_init(&sll, 1, compare_pointers, NULL);
    if (rv) {
        fprintf(stderr, "linkedlist_init failed: %d\n", rv);
        return rv;
    } else {
        printf("list initialized\n");
    }
    flush();

    for (i = START_INT; i <= END_INT; i++) {
        value = integer2pointer(i);
        rv = linkedlist_add_once(&sll, value, &found);
        if (rv) {
            fprintf(stderr, "adding %d failed\n", i);
        }
        hashmap[i] = 1;
        rv = linkedlist_add_once(&sll, value, &found);
        if (rv) {
            fprintf(stderr, "adding %d failed in 2nd attempt\n", i);
        }
        rv = linkedlist_add_once(&sll, value, &found);
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
    while (not_endof_linkedlist(node)) {
        printf("%lld ", pointer2integer(node->user_data));
        if (pointer2integer(node->user_data) < result) {
            fprintf(stderr, "list order incorrect, current: %lld expected: >= %d\n",
                pointer2integer(node->user_data), result);
            result = pointer2integer(node->user_data);
        }
        node = node->next;
    }
    printf("\n");
    fflush(stdout);
    fflush(stdout);

    /* now delete one at a time and make sure everything is consistent */
    printf("now deleting and verifying one at a time\n");
    flush();

    count = sll.n;
    int *s, *d;
    for (i = END_INT; i >= START_INT; i--) {

        rv = linkedlist_delete(&sll, integer2pointer(i), (void**) &d);
        if (rv == 0) {
            count--;
            assert(count == sll.n);
            assert(integer2pointer(i) == d);
            hashmap[i] = 0;

            /* 
             * make sure that the deleted entry is NOT in the list 
             * but ALL others are
             */
            for (j = START_INT; j <= END_INT; j++) {
                if (hashmap[j]) {
                    assert(linkedlist_search(&sll, integer2pointer(j), (void**)&s) == 0);
                    assert(j == pointer2integer(s));
                } else {
                    assert(linkedlist_search(&sll, integer2pointer(j), (void**)&s) == ENODATA);
                }
            }
        } else {
            fprintf(stderr, "linkedlist_delete for %d failed\n", i);
        }
    }
    assert(sll.n == 0);
    printf("all data verified, list is sane\n");

    return 0;
}


