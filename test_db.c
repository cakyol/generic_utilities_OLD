
#include "generic_object_database.h"

#define MAX_TYPES		10
#define MAX_ATTRS		10
#define ITER			1
#define MAX_AV_COUNT		3

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

void notify_event (event_record_t *evrp)
{
    int event = evrp->event;
    return;

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
	printf("attribute %d value <%s> added for object (%d, %d)\n",
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

void add_del_attributes (object_database_t *obj_db, object_t *obj)
{
    int i, iter, count;
    char complex_value[50];
    attribute_instance_t *aitp;
    int av;

    printf("adding & deleting attributes to an object\n");
    count = 0;
    for (iter = 0; iter < ITER; iter++) {
	for (i = MAX_ATTRS; i > 1; i--) {
	    aitp = object_attribute_instance_add(obj, i);
	    if (NULL == aitp) {
		fprintf(stderr, "object (%d, %d) attribute %d add failed\n",
			obj->object_type, obj->object_instance, i);
	    }
	    for (av = 0; av < MAX_AV_COUNT; av++) {
		object_attribute_add_simple_value(obj, i, av);
		sprintf(complex_value, "cav %d", av);
		object_attribute_add_complex_value(obj, i,
			(byte*) complex_value, strlen(complex_value) + 1);
		count++;
	    }

#if 0
	    // deletes all values and sets it
	    object_attribute_set_simple_value(obj, i, i);
	    //printf("deleting attribute instance %d from object (%d, %d)\n",
		    //i, obj->object_type, obj->object_instance);
	    object_attribute_instance_destroy(obj, i);
#endif
	}
#if 0
	for (i = MAX_ATTRS; i > 1; i--) {
	    if (object_attribute_instance_destroy(obj, i) != ok) {
		printf("object (%d, %d) attribute %d delete failed\n",
			obj->object_type, obj->object_instance, i);
		count++;
	    }
	}
#endif

    }
}

int main (int argc, char *argv[])
{
    object_database_t db;
    int type, instance;
    object_t *obj, *parent;
    int count;

    database_initialize(&db, 1, notify_event, NULL);

    printf("creating objects\n");
    count = 0;
    // reverse ordering is WORST performance for the index object
    // but irrelevant to the avl tree object

    parent = &db.root_object;
    for (type = MAX_TYPES; type > 0; type--) {
	for (instance = type; instance > 0; instance--) {
	    obj = object_create(parent, type, instance);
	    if (obj) {
		printf("object type %d, instance %d created\n", type, instance);
		add_del_attributes(&db, obj);
		//object_destroy(obj);
		//printf("object destroyed\n");
		count++;
	    } else {
                fprintf(stderr, "object %d,%d creation failed\n",
                        type, instance);
            }
            parent = obj;
	}
    }
    printf("finished creating all objects\n");
    database_store(&db);
    printf("\n");
    return 0;
}






