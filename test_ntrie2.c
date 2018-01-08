
#include <stdio.h>
#include "ntrie_object.h"

#define MAX_REF		10
#define MAX_BYTE 	255
#define MAX_SIZE	(MAX_BYTE + 1)

char *strings[] = {
    "bc",
    "uninitialized",
    "initialized",
    "hello",
    // "1_invalid_string",
    "hello",
    "hello2",
    "hello4",
    "hello6",
    "goodbye",
    "uninitialized3",
    "hello10",
    "abc",
    "ab",
    "bc",
    "goodbye5"
};

char *not_there1 = "initi";
char *not_there2 = "hel";

#define MAX_STRINGS	(sizeof(strings) / sizeof(char*))

int print_key (void *trie_object, void *trie_node, void *data,
    void *key, void *key_length, void *u1, void *u2)
{
    char buffer [100];
    int len = pointer2integer(key_length);

    memcpy(buffer, key, len);
    buffer[len] = 0;
    printf("key %s: data: 0x%p\n", buffer, data);
    return 0;
}

char *extra = "wildcard";

int main (argc, argv)
int argc;
char *argv [];
{
    ntrie_t ntrie_obj;
    int i, rc;
    void *found;

    printf("\nPOPULATING FASTMAP\n");
    ntrie_init(&ntrie_obj, 0, NULL);
    printf("\nSIZE OF FASTMAP NODE = %d BYTES\n", (int) sizeof(ntrie_node_t));
    for (i = 0; i < (int) MAX_STRINGS; i++) {
        ntrie_insert(&ntrie_obj, strings[i], strlen(strings[i]),
	    strings[i], &found);
    }

    /* these should NOT be found */
    printf("searching %s in ntrie, it should NOT be there\n", not_there1);
    rc = ntrie_search(&ntrie_obj, not_there1, strlen(not_there1), &found);
    if ((rc == 0) || found) {
        fprintf(stderr, "string %s should NOT be found but was\n", not_there1);
    } else {
        printf("passed\n");
    }

    printf("searching %s in ntrie, it should NOT be there\n", not_there2);
    rc = ntrie_search(&ntrie_obj, not_there2, strlen(not_there2), &found);
    if ((rc == 0) || found) {
        fprintf(stderr, "string %s should NOT be found but was\n", not_there2);
    } else {
        printf("passed\n");
    }
    fflush(stdout);

    /* print out contents */
    ntrie_traverse(&ntrie_obj, print_key, NULL, NULL);

    return 0;
}

