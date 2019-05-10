
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "linkedlist_object.h"

#define START_INT   1
#define END_INT     300
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

static int delete_count = 0;

void destroy_fn (void *user_data, void *arg)
{
    delete_count++;
}

int main (int argc, char *argv[])
{
    int failed, failures;
    int result;
    int i, j, count;
    linkedlist_t sll;
    linkedlist_node_t *node;
    void *value;

    failed = linkedlist_init(&sll, 1, compare_pointers, NULL);
    if (failed) {
        fprintf(stderr, "linkedlist_init failed: %d\n", failed);
        return failed;
    } else {
        printf("list initialized\n");
    }
    flush();

    failures = 0;
    for (i = START_INT; i <= END_INT ; i++) {
        value = integer2pointer(i);

        /* FIRST time, it should succeed */
        failed = linkedlist_add_once(&sll, value, NULL);
        if (failed) {
            fprintf(stderr, "adding %d failed: %d\n", i, failed);
        }
        hashmap[i] = 1;

        /* consecutive ones should fail */
        failed = linkedlist_add_once(&sll, value, NULL);
        if (!failed) failures++;
        failed = linkedlist_add_once(&sll, value, NULL);
        if (!failed) failures++;
    }
    printf("all %d items added to list\n", sll.n);
    assert(sll.n == (END_INT - START_INT + 1));
    if (failures) {
        fprintf(stderr, "%d failures during repetitive inserts\n", failures);
    }
    flush();

    printf("checking that the list is ordered .. ");
    node = sll.head;
    result = START_INT - 10;
    while (not_endof_linkedlist(node)) {
        //printf("%lld ", pointer2integer(node->user_data));
        if (pointer2integer(node->user_data) < result) {
            fprintf(stderr, "list order incorrect, current: %lld expected: >= %d\n",
                pointer2integer(node->user_data), result);
            result = pointer2integer(node->user_data);
        }
        node = node->next;
    }
    printf("ok\n");
    fflush(stdout);
    fflush(stdout);

    /* now delete one at a time and make sure everything is consistent */
    printf("now deleting and verifying one at a time\n");
    flush();

    count = sll.n;
    int *s, *d;
    for (i = END_INT; i >= START_INT; i--) {

        failed = linkedlist_delete(&sll, integer2pointer(i), (void**) &d);
        if (failed == 0) {
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

    delete_count = 0;
    linkedlist_destroy(&sll, destroy_fn, NULL);
    printf("delete_count %d (should be 0)\n", delete_count);

    return 0;
}


