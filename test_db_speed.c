
#include "generic_object_database.h"

// #define BY_NAME

#define MAX_TYPES		2000
#define MAX_ATTRS		100
#define ITER			4
#define MAX_AV_COUNT		10

object_database_t db;
timer_obj_t timr;

int main (int argc, char *argv[])
{
    int ptype, pinstance;
    int type, instance;
    int num_elements;
    int64 count;
    int i;
    uint64 bytes_used;
    double megabytes_used;

    database_initialize(&db, 1, NULL, NULL);

    /* create objects */
    count = 0;
    start_timer(&timr);
    printf("creating objects\n");
    ptype = pinstance = 0;
    for (type = MAX_TYPES; type > 0; type--) {
	for (instance = MAX_TYPES; instance > 0; instance--) {

	    if (SUCCEEDED(object_create(&db, ptype, pinstance, type, instance))) {
                count++;
            } else {
		fprintf(stderr, "creating (%d, %d) failed\n",
			type, instance);
	    }
	}
    }
    end_timer(&timr);
    report_timer(&timr, count);
    printf("\n");

    OBJECT_MEMORY_USAGE(&db, bytes_used, megabytes_used);
    num_elements = database_object_count(&db);
    printf("database has %d elements (%llu bytes %f Megabytes)\n",
            num_elements, bytes_used, megabytes_used);
    printf("   approx %d bytes per element\n", (int) (bytes_used/num_elements));

    /* search objects */
    printf("searching objects\n");
    count = 0;
    start_timer(&timr);
    for (i = 0; i < ITER; i++) {
	for (type = 1; type <= MAX_TYPES; type++) {
	    for (instance = 1; instance <= MAX_TYPES; instance++) {

		if (!object_exists(&db, type, instance)) {
		    fprintf(stderr, "could not find object %d, %d\n",
                            type, instance);
		}
		count++;
	    }
	}
    }
    end_timer(&timr);
    report_timer(&timr, count);
    printf("\n");

    /* delete objects */
    count = 0;
    start_timer(&timr);
    printf("deleting objects\n");
    for (type = MAX_TYPES; type > 0; type--) {
	for (instance = MAX_TYPES; instance > 0; instance--) {
	    if (FAILED(object_destroy(&db, type, instance))) {
		fprintf(stderr, "deleting (%d, %d) failed\n",
			type, instance);
	    }
	    count++;
	}
    }
    end_timer(&timr);
    report_timer(&timr, count);
    printf("\n");

    return 0;
}






