
#include "list.h"

#define MAX_VALUE   0xFFFFFF

int main (int argc, char *argv[])
{
    list_t list;
    int i, j, max;
    void *ptr;
    list_node_t *node, *nextnode;

    if (list_init(&list, false, true, 0, null, null)) {
        fprintf(stderr, "list_init failed\n");
        return -1;
    }

    /* insert in increasing order */
    fprintf(stderr, "inserting all nodes\n");
    for (i = 0; i <= MAX_VALUE; i++) {
        ptr = integer2pointer(i);
        if (list_append_data(&list, ptr)) {
            fprintf(stderr, "list_append_data for %p failed\n", ptr);
        }
    }

    /* verify */
    fprintf(stderr, "verifying all nodes\n");
    node = list.head;
    for (i = 0; i <= MAX_VALUE; i++) {
        if (node->data != integer2pointer(i)) {
            fprintf(stderr, "mismatch, expected %p, got %p\n",
                    integer2pointer(i), node->data);
        }
        node = node->next;
    }

    /* delete every odd node */
    fprintf(stderr, "deleting all ODD nodes\n");
    node = list.head;
    max = 0;
    for (i = 0; i <= MAX_VALUE; i++) {
        nextnode = node->next;
        if (pointer2integer(node->data) & 1) {
            list_remove_node(&list, node);
        } else {
            max++;
        }
        node = nextnode;
    }

    /* verify that only even numbers remain */
    fprintf(stderr, "verifying\n");
    node = list.head;
    j = 0;
    for (i = 0; i < max; i++) {
        if (node->data != integer2pointer(j)) {
            fprintf(stderr, "mismatch, expected %p, got %p\n",
                    node->data, integer2pointer(j));
        }
        j += 2;
        node = node->next;
    }

    /* now delete all even nodes and ensure nothing is left */
    fprintf(stderr, "deleting all even nodes\n");
    node = list.head;
    while (node) {
        nextnode = node->next;
        if ((pointer2integer(node->data) & 1) == 0) {
            list_remove_node(&list, node);
        }
        node = nextnode;
    }
    fprintf(stderr, "verifying\n");
    assert(list.n == 0);
    assert(list.head == null);
    assert(list.tail == null);
    list_destroy(&list);
    return 0;
}



