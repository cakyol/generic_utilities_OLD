
#include "debug_framework.h"
#include "event_manager.h"

#define LOW_OBJECT      15
#define HI_OBJECT       1000

#define EVENT_MGR_MODULE        1

event_manager_t em;

int process_event (void *evp, void *arg)
{
    return 0;
}

int main (int argc, char *argv[])
{
    int i;

    debug_init();
    debug_module_set_name(EVENT_MGR_MODULE, "EventManagerModule");

    if (event_manager_init(&em, 0, 0)) {
        FATAL_ERROR("event_manager_init failed");
    }

    for (i = LOW_OBJECT; i <= HI_OBJECT; i++) {
        if (register_for_object_events(&em, i, process_event, NULL)) {
            MODULE_ERROR(EVENT_MGR_MODULE,
                "register_for_object_events failed for object %d", i);
        }
        if (register_for_attribute_events(&em, i, process_event, NULL)) {
            MODULE_ERROR(EVENT_MGR_MODULE,
                "register_for_attribute_events failed for object %d", i);
        }
    }

    /* now check */
    for (i = LOW_OBJECT-10; i <= HI_OBJECT+10; i++) {

        /* should be registered */
        if ((i >= LOW_OBJECT) && (i <= HI_OBJECT)) {

            if (!already_registered(&em, OBJECT_EVENTS, i, process_event, NULL)) {
                MODULE_ERROR(EVENT_MGR_MODULE,
                    "object %d NOT registered for object events", i);
            }

            if (!already_registered(&em, ATTRIBUTE_EVENTS, i, process_event, NULL)) {
                MODULE_ERROR(EVENT_MGR_MODULE,
                    "object %d NOT registered for attribute events", i);
            }

        /* should NOT be registered */
        } else {
            if (already_registered(&em, OBJECT_EVENTS, i, process_event, NULL)) {
                MODULE_ERROR(EVENT_MGR_MODULE,
                    "object %d registered for object events", i);
            }

            if (already_registered(&em, ATTRIBUTE_EVENTS, i, process_event, NULL)) {
                MODULE_ERROR(EVENT_MGR_MODULE,
                    "object %d registered for attribute events", i);
            }
        }
    }

    printf("if no errors have been output so far, event manager is correct.\n");

    return 0;
}




