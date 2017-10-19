
#include "utils.h"

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
    "hello",
    "hello",
    "hello",
    "goodbye",
    "uninitialized",
    "hello",
    "abc",
    "ab",
    "bc",
    "goodbye"
};

char *not_there1 = "initi";
char *not_there2 = "hel";

#define MAX_STRINGS	(sizeof(strings) / sizeof(char*))

int print_key (byte *vkey, int len, void *user_data, int refc)
{
    char buffer [100];

    memcpy(buffer, vkey, len);
    buffer[len] = 0;
    printf("%s: %d\n", buffer, refc);
    return ok;
}

char *extra = "wildcard";

int main (argc, argv)
int argc;
char *argv [];
{
    ntrie_t ntrie_obj;
    int i, rref;
    int rc;

    printf("\nPOPULATING FASTMAP\n");
    ntrie_init(&ntrie_obj, true);
    printf("\nSIZE OF FASTMAP NODE = %d BYTES\n", (int) sizeof(ntrie_node_t));
    for (i = 0; i < MAX_STRINGS; i++) {
        ntrie_insert(&ntrie_obj, strings[i], strlen(strings[i]), NULL, &rref);
    }

    /* these should NOT be found */
    printf("searching %s in ntrie, it should NOT be there\n", not_there1);
    rc = ntrie_search(&ntrie_obj, not_there1, strlen(not_there1), NULL, &rref);
    if ((rc == ok) || (rref != 0)) {
        fprintf(stderr, "string %s should NOT be found but did (%d)",
                not_there1, rref);
    } else {
        printf("passed\n");
    }

    printf("searching %s in ntrie, it should NOT be there\n", not_there2);
    rc = ntrie_search(&ntrie_obj, not_there2, strlen(not_there2), NULL, &rref);
    if ((rc == ok) || (rref != 0)) {
        fprintf(stderr, "string %s should NOT be found but did (%d)",
                not_there2, rref);
    } else {
        printf("passed\n");
    }
    fflush(stdout);

    /* print out contents */
    ntrie_traverse(&ntrie_obj, print_key);

    return 0;
}

