
#include "generic_object_database.h"
#include <sys/resource.h>

#define MAX_TYPES               3000
#define MAX_ATTRS               5
#define ITER                    1
#define MAX_AV_COUNT            2

char temp_buffer [64];

char *attribute_value_string (event_record_t *evrp)
{
    if (evrp->attribute_value_length == 0) {
        sprintf(temp_buffer, "%lld", evrp->attribute_value_data);
    } else {
        sprintf(temp_buffer, "%s", (char*) &evrp->attribute_value_data);
    }
    return temp_buffer;
}

void notify_event (event_record_t *evrp, void *arg2)
{
    object_database_t *dbp = (object_database_t*) arg2;
    int event = evrp->event_type;

    SUPPRESS_UNUSED_VARIABLE_WARNING(evrp);
    SUPPRESS_UNUSED_VARIABLE_WARNING(dbp);
    SUPPRESS_UNUSED_VARIABLE_WARNING(event);

    // return 0;

    if (event & OBJECT_CREATED) {
        printf("child (%d, %d) created for parent (%d, %d)\n",
            evrp->related_object_type, evrp->related_object_instance,
            evrp->object_type, evrp->object_instance);
        return;
    }
    if (event & ATTRIBUTE_INSTANCE_ADDED) {
        printf("attribute %d added to object (%d, %d)\n",
            evrp->attribute_id, evrp->object_type, evrp->object_instance);
        return;
    }
    if (event & ATTRIBUTE_VALUE_ADDED) {
        printf("attribute %d value <%s> added to object (%d, %d)\n",
            evrp->attribute_id, attribute_value_string(evrp),
            evrp->object_type, evrp->object_instance);
        return;
    }
    if (event & ATTRIBUTE_VALUE_DELETED) {
        printf("attribute %d value <%s> deleted from object (%d, %d)\n",
            evrp->attribute_id, attribute_value_string(evrp),
            evrp->object_type, evrp->object_instance);
        return;
    }
    if (event & ATTRIBUTE_INSTANCE_DELETED) {
        printf("attribute %d deleted from object (%d, %d)\n",
            evrp->attribute_id, evrp->object_type, evrp->object_instance);
        return;
    }
    if (event & OBJECT_DESTROYED) {
        printf("object (%d, %d) destroyed\n", 
            evrp->object_type, evrp->object_instance);
        return;
    }

    printf("UNKNOWN EVENT TYPE %d\n", event);
}

void add_del_attributes (object_database_t *obj_db, int type, int instance)
{
    int i, iter, count;
    char complex_value[50];
    int av;

    //printf("adding attributes to an object\n");
    count = 0;
    for (iter = 0; iter < ITER; iter++) {
        for (i = MAX_ATTRS; i > 1; i--) {
            for (av = 0; av < MAX_AV_COUNT; av++) {
                object_attribute_add_simple_value(obj_db, type, instance, i, av);
                sprintf(complex_value, "cav %d", av);
                object_attribute_add_complex_value(obj_db, type, instance, i,
                        (byte*) complex_value, strlen(complex_value) + 1);
                count++;
            }
            object_attribute_destroy(obj_db, type, instance, i);
        }
    }
}

int main (int argc, char *argv[])
{
    object_database_t db;
    int parent_type, parent_instance;
    int child_type, child_instance;
    int rc, count;

    printf("size of one object is %ld bytes\n", sizeof(object_t));

    database_initialize(&db, 1, 1, NULL);

    rc = database_register_for_object_events(&db, ALL_OBJECT_TYPES,
                notify_event, &db);
    if (rc) {
        fprintf(stderr, "database_register_for_object_events failed: %d\n", rc);
        return rc;
    }
    rc = database_register_for_attribute_events(&db, ALL_OBJECT_TYPES,
                notify_event, &db);
    if (rc) {
        fprintf(stderr, "database_register_for_attribute_events failed: %d\n", rc);
        return rc;
    }

    printf("creating objects\n");
    count = 0;

    // reverse ordering is WORST performance for the index object
    // but irrelevant to the avl tree object

    parent_type = parent_instance = 0;

    for (child_type = MAX_TYPES; child_type > 0; child_type--) {
        for (child_instance = child_type; child_instance > 0; child_instance--) {
            if (object_create(&db,
                            parent_type, parent_instance,
                            child_type, child_instance))
            {
                fprintf(stderr, "object %d,%d creation with parent %d,%d failed\n",
                        child_type, child_instance,
                        parent_type, parent_instance);
                continue;
            } 
            
            //printf("object %d,%d with parent %d,%d created\n",
                //child_type, child_instance,
                //parent_type, parent_instance);
            add_del_attributes(&db, child_type, child_instance);
            count++;

            parent_type = child_type;
            parent_instance = child_instance;
        }
    }
    printf("finished creating all %d objects\n", count);
    printf("now writing to file ... ");
    fflush(stdout);
    fflush(stdout);
    database_store(&db);
    printf("done\n");
    fflush(stdout);
    fflush(stdout);
    return 0;
}






