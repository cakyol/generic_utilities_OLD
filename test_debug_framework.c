
#include <unistd.h>
#include "debug_framework.h"

void function1 (void)
{
    F_ENTER;
    F_EXIT;
}

int function2 (void)
{
    F_ENTER;
    function1();
    F_EXIT_V(0);
}

int function3 (void)
{
    F_ENTER;
    function2();
    F_EXIT_V(0);
}

int function4 (void)
{
    F_ENTER;
    function3();
    F_EXIT_V(0);
}

int main (int argc, char *argv[])
{
    int m;
    char name [64];

    debug_init();

    F_ENTER;
    fprintf(stderr, "\n\nsizeof debg framework for %d modules is %lu bytes\n\n",
            MAX_MODULES, sizeof(module_levels));
    for (m = 0; m < 10; m++) {
        DEBUG(0, "module %d SHOULD be reported", 0);
        NOTIFY(0, "module %d SHOULD be reported", 0);
        WARNING(0, "module %d SHOULD be reported", 0);
        ERROR(0, "module %d should be reported", 0);
    }

    for (m = 0; m < 25; m++) {
        if (m & 1) {
            sprintf(name, "MODULE NUMBER %d", m);
            debug_module_set_name(m, name);
            debug_module_set_minimum_reporting_level(m, ERROR_LEVEL);
            DEBUG(m, "module %d SHOULD NOT be reported", m);
            NOTIFY(m, "module %d SHOULD NOT be reported", m);
            WARNING(m, "module %d SHOULD NOT be reported", m);
            ERROR(m, "module %d should be reported", m);
        } else {
            debug_module_set_name(m, NULL);
            debug_module_set_minimum_reporting_level(m, DEBUG_LEVEL);
            DEBUG(m, "module %d should be reported", m);
            NOTIFY(m, "module %d should be reported", m);
            WARNING(m, "module %d should be reported", m);
            ERROR(m, "module %d should be reported", m);
        }
    }

    /* try & break using an invalid module number */
    for (m = MAX_MODULES; m < (MAX_MODULES + 10); m++) {
        DEBUG(m, "module %d SHOULD NOT be reported", m);
        NOTIFY(m, "module %d SHOULD NOT be reported", m);
        WARNING(m, "module %d SHOULD NOT be reported", m);
        ERROR(m, "module %d SHOULD NOT be reported", m);
    }

    function4();

    F_EXIT_V(0); 
}


