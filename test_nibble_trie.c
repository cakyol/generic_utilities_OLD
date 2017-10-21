
#include "trie_object.h"

#define SIZE	5
#define MAX_REF	4

char string [SIZE + 1];
ntrie_t ntrie_obj;

#define FOR_ALL_POSSIBLE_STRINGS \
    for (i = 'a'; i <= 'z'; i++) \
	for (j = 'a'; j <= 'z'; j++) \
	    for (k = 'a'; k <= 'z'; k++) \
		for (l = 'a'; l <= 'z'; l++) \
		    for (m = 'a'; m <= 'z'; m++) { \
			string [0] = m; string [1] = l; string [2] = k; \
			string [3] = j; string [4] = i; \
			string [5] = 0; \

#define MAX_BYTE 	255
#define MAX_SIZE	(MAX_BYTE + 1)

int char_map [MAX_SIZE];
timer_obj_t timr;

int main (argc, argv)
int argc;
char *argv [];
{
    int i, j, k, l, m;
    uint64 mem;
    int count = 0, total = 0;
    double megabytes;

    printf("\nPOPULATING TRIE\n");
    ntrie_init(&ntrie_obj, NULL);
    printf("\nSIZE OF NTRIE NODE = %lu BYTES\n", sizeof(ntrie_node_t));
    start_timer(&timr);
    FOR_ALL_POSSIBLE_STRINGS
	total++;
        if (ntrie_insert(&ntrie_obj, string, SIZE, (void*) 1, NULL) == ok) {
	    count++;
	}
    }
    end_timer(&timr);
    report_timer(&timr, total);

    printf("successfully added %d of %d strings to trie (nodes %d)\n", 
	count, total, ntrie_obj.node_count);
    OBJECT_MEMORY_USAGE(&ntrie_obj, mem, megabytes);
    printf("total memory used is %llu bytes (%lf Mbytes)\n",
	mem, megabytes);

    printf("\nSEARCHING ALL AVAILABLE STRINGS\n");
    count = total = 0;
    start_timer(&timr);
    FOR_ALL_POSSIBLE_STRINGS
        total++;
	if (ntrie_search(&ntrie_obj, string, SIZE, NULL) == ok) {
            count++;
        }
    }
    end_timer(&timr);
    printf("found %d strings of %d strings\n", count, total);
    report_timer(&timr, total);

    return 0;
}

