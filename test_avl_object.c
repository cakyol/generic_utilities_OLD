
#include "avl_tree_object.h"

#define MAX_SZ		(50 * 1024 * 1024)
#define EXTRA           (10 * MEGA)

int max_value_reached = 0;
int data [MAX_SZ];
avl_tree_t avlt;
timer_obj_t timr;

int int_compare (datum_t p1, datum_t p2)
{ 
    int *i1 = p1.pointer;
    int *i2 = p2.pointer;
    return *i1 - *i2;
}

void perform_avl_tree_test (avl_tree_t *avlt, boolean use_odd_numbers)
{
    int i, lo, hi, d, fail_search;
    boolean rv;
    int fine, not_fine;
    char *oddness = use_odd_numbers ? "odd" : "even";
    char *reverse = use_odd_numbers ? "even" : "odd";
    uint64 bytes_used;
    double megabytes_used;
    datum_t fwdata, searched, found, removed;

    printf("size of ONE avl node is: %lu bytes\n",
            sizeof(avl_node_t));

    /* 
    ** Fill opposing ends of array, converge in middle.
    ** This gives some sort of randomness to data.
    */
    printf("filling array of size %d with %s number data\n", 
	MAX_SZ, oddness);
    d = use_odd_numbers ? 1 : 0;
    lo = 0; 
    hi = MAX_SZ - 1;
    while (TRUE) {
	data[lo++] = d;
	d += 2;
	data[hi--] = d;
	d += 2;
	if (lo > hi) break;
    }

    if (max_value_reached < d) {
	max_value_reached = d + 10;
	printf("max value recorded so far is %d\n", max_value_reached);
    }

    avl_tree_init(avlt, true, int_compare, NULL);

    /* enter all array data into avl tree */
    printf("now entering all %s number data into the avl tree\n", oddness);
    start_timer(&timr);
    for (i = 0; i < MAX_SZ; i++) {
	fwdata.pointer = &data[i];
	rv = avl_tree_insert(avlt, fwdata, &found);
	if (rv != ok) {
	    printf("populate_data: avl_tree_insert error: %d failed\n", i);
	}
    }
    end_timer(&timr);
    report_timer(&timr, MAX_SZ);
    OBJECT_MEMORY_USAGE(avlt, bytes_used, megabytes_used);
    printf("total memory used by the avl tree of %d nodes: %llu bytes (%lf Mbytes)\n",
	avlt->n, bytes_used, megabytes_used);

    printf("searching for non existant data\n");
    fine = not_fine = 0;
    start_timer(&timr);
    for (i = max_value_reached; i < (max_value_reached + EXTRA); i++) {
	searched.pointer = &i;
	rv = avl_tree_search(avlt, searched, &found);
	if ((rv == ok) || found.pointer) {
	    not_fine++;
	} else {
	    fine++;
	}
    }
    end_timer(&timr);
    report_timer(&timr, EXTRA);
    printf("expected %d, NOT expected %d\n", fine, not_fine);

    /* now search all data that should be found (all of them) */
    printf("now searching all %s numbers in the avl tree\n", oddness);
    fine = not_fine = 0;
    start_timer(&timr);
    for (i = 0; i < MAX_SZ; i++) {
	searched.pointer = &data[i];
	rv = avl_tree_search(avlt, searched, &found);

	if ((rv != ok) || (data[i] != *((int*) found.pointer))) {
	    not_fine++;
	} else {
	    fine++;
	}
    }
    end_timer(&timr);
    report_timer(&timr, MAX_SZ);
    printf("found %d as expected and %d as NOT expected\n", fine, not_fine);

    /* now search for all entries that should NOT be in the tree */
    printf("now searching for all %s numbers in the avl tree\n", reverse);
    fine = not_fine = 0;
    d = use_odd_numbers ? 0 : 1;
    start_timer(&timr);
    for (i = 0; i < MAX_SZ; i++) {
	searched.pointer = &d;
	rv = avl_tree_search(avlt, searched, &found);
	if ((rv == ok) || found.pointer) {
	    not_fine++;
	} else {
	    fine++;
	}
	d += 2;
    }
    end_timer(&timr);
    report_timer(&timr, MAX_SZ);
    printf("%d as expected and %d as NOT expected\n", fine, not_fine);

#if 0

int tree_nodes = avlt->n;
printf("now deleting the whole tree (%d nodes)\n", tree_nodes);
start_timer(&timr);
avl_tree_destroy(avlt);
end_timer(&timr);
report_timer(&timr, tree_nodes);
return;

#endif // 0

    /* now delete one entry at a time and search it (should not be there) */
    printf("now deleting and searching\n");
    fine = not_fine = fail_search = 0;
    start_timer(&timr);
    for (i = 0; i < MAX_SZ; i++) {
	searched.pointer =  &data[i];
	rv = avl_tree_remove(avlt, searched, &removed);
	if ((rv != 0) || (data[i] != *((int*) removed.pointer))) {
	    not_fine++;
	} else {
	    fine++;
	}
	rv = avl_tree_search(avlt, searched, &found);
	if ((rv == ok) || found.pointer) {
	    fail_search++;
	}
    }
    end_timer(&timr);
    report_timer(&timr, MAX_SZ*2);
    printf("deleted %d, could NOT delete %d, erroneous search %d\n",
	fine, not_fine, fail_search);

    OBJECT_MEMORY_USAGE(avlt, bytes_used, megabytes_used);
    printf("total memory used by the avl tree (%d nodes) after deletes: "
	"%llu bytes (%f Mbytes)\n",
        avlt->n, bytes_used, megabytes_used);

    avl_tree_destroy(avlt);
}

int main (argc, argv)
int argc;
char *argv [];
{
    avl_tree_t avlt;

    perform_avl_tree_test(&avlt, TRUE);
    printf("\n\n");
    perform_avl_tree_test(&avlt, FALSE);
    
    return 0;
}

