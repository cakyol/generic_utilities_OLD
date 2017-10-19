
#include "index_object.h"

#define MAX_SZ			(2048 * 2048)
#define ITER			(5)
#define BIG_ITER		(10000 * 4092)

/*
** some data structure we are interested in
*/
typedef struct Data
{
    int first;
    int second;
} Data, *DataPtr;

Data data [MAX_SZ];
Data lodata, hidata, searched;
timer_obj_t timr;

int compareData (datum_t p1, datum_t p2)
{
    Data *d1, *d2;
    
    d1 = (Data*) p1.pointer;
    d2 = (Data*) p2.pointer;
    int diff = d1->first - d2->first;
    if (diff) return diff;
    return d1->second - d2->second;
}

int main (int argc, char *argv[])
{
    register int i;
    index_obj_t index;
    datum_t ip1;
    datum_t exists;
    Data *datp;
    datum_t removed;
    int iter;

    /* create the index first */
    if (index_obj_init(&index, 
            true, compareData, MAX_SZ/4, 1000, NULL) != ok) {
	printf ("could not create index\n"); _exit (1);
    }
    
    printf ("\n\n\n");
printf("FILLING INITIAL DATA\n");
    start_timer(&timr);
    for (i=0; i<MAX_SZ; i++) {
	data [i].first = i;
	data [i].second = i;
	ip1.pointer = &data[i];
	if (index_obj_insert(&index, ip1, &exists)) {
	    fprintf(stderr, "cannot add entry %d %d to index %d\n",
		data[i].first, data[i].second, i);
	    return -1;
	} else {
	    // printf("inserted (%d, %d)\n", i, i);
	}
    }
    end_timer(&timr);
    printf("INITIAL DATA FILLED, object expanded %d times\n", index.expansion_count);
    report_timer(&timr, MAX_SZ);

    lodata.first = lodata.second = -5;
    hidata.first = hidata.second = MAX_SZ + 5;

    printf ("\n\n\n");
printf("SEARCHING DATA\n");
    start_timer(&timr);
    for (iter = 0; iter < ITER; iter++) {
	for (i = 0; i < MAX_SZ; i++) {
	    searched.first = searched.second = i;
	    ip1.pointer = &searched;
	    if (index_obj_search(&index, ip1, &exists) == OK) {
		datp = exists.pointer;
		if ((searched.first != datp->first) ||
		    (searched.second != datp->second)) {
			printf("searched (%d, %d), did NOT match found (%d, %d)\n",
			    searched.first, searched.second,
			    datp->first, datp->second);
		}
	    } else {
		printf("index_search could not find (%d, %d)\n",
		    searched.first, searched.second);
	    }
	}
    }
    end_timer(&timr);
    report_timer(&timr, ITER * MAX_SZ);

    printf ("\n\n\n");
printf ("BEST CASE INSERT/DELETE for %d entries\n", MAX_SZ);
    start_timer(&timr);
    ip1.pointer = &hidata;
    for (i = 0; i < BIG_ITER; i++) {
	if (index_obj_insert(&index, ip1, &exists) != OK) {
	    printf("could not insert hidata %d %d",
		hidata.first, hidata.second);
	}
	if (NULL != exists.pointer) {
	    fprintf(stderr, "hidata should NOT exist but it does\n");
	}
	if (index_obj_remove(&index, ip1, &removed) != OK) {
	    printf("could not remove hidata %d %d",
		hidata.first, hidata.second);
	}
	datp = removed.pointer;
	if ((datp->first != hidata.first) || (datp->second != hidata.second)) {
	    fprintf(stderr, "removed data does not match: (%d, %d) != (%d, %d)\n",
		    hidata.first, hidata.second, datp->first, datp->second);
	}
    }
    end_timer(&timr);
    report_timer(&timr, BIG_ITER * 2);

    printf ("\n\n\n");
printf ("WORST CASE INSERT/DELETE for %d entries\n", MAX_SZ);
    start_timer(&timr);
    ip1.pointer = &lodata;
    for (i = 0; i < ITER * 200; i++) {
	if (index_obj_insert(&index, ip1, &exists) != OK) {
	    printf("could not insert lodata %d %d",
		lodata.first, lodata.second);
	}
	if (index_obj_remove(&index, ip1, &removed) != OK) {
	    printf("could not remove hidata %d %d",
		lodata.first, lodata.second);
	}
    }
    end_timer(&timr);
    report_timer(&timr, ITER * 2 * 200);

    printf ("\n\n\n");
    return 0;
}

