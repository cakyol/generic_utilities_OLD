
#include <unistd.h>
#include "debug_framework.h"

int main (int argc, char *argv[])
{
    int m;
    char name [64];

    debug_init();
    fprintf(stderr, "\n\nsizeof debg framework for %d modules is %lu bytes\n\n",
	    MAX_MODULES, sizeof(module_levels));
    for (m = 0; m < 10; m++) {
	MODULE_DEBUG(0, "m%d SHOULD be reported", 0);
	MODULE_INFO(0, "m%d SHOULD be reported", 0);
	MODULE_WARNING(0, "m%d SHOULD be reported", 0);
	MODULE_ERROR(0, "m%d should be reported", 0);
    }

    for (m = 0; m < 25; m++) {
        if (m & 1) {
	    sprintf(name, "MODULE%d", m);
	    debug_module_set_name(m, name);
            debug_module_set_level(m, ERROR_LEVEL);
            MODULE_DEBUG(m, "m%d SHOULD NOT be reported", m);
            MODULE_INFO(m, "m%d SHOULD NOT be reported", m);
            MODULE_WARNING(m, "m%d SHOULD NOT be reported", m);
            MODULE_ERROR(m, "m%d should be reported", m);
        } else {
	    debug_module_set_name(m, NULL);
            debug_module_set_level(m, DEBUG_LEVEL);
            MODULE_DEBUG(m, "m%d should be reported", m);
            MODULE_INFO(m, "m%d should be reported", m);
            MODULE_WARNING(m, "m%d should be reported", m);
            MODULE_ERROR(m, "m%d should be reported", m);
        }
    }

    /* try & break using an invalid module number */
    for (m = MAX_MODULES; m < (MAX_MODULES + 10); m++) {
	MODULE_DEBUG(m, "m%d SHOULD NOT be reported", m);
	MODULE_INFO(m, "m%d SHOULD NOT be reported", m);
	MODULE_WARNING(m, "m%d SHOULD NOT be reported", m);
	MODULE_ERROR(m, "m%d SHOULD NOT be reported", m);
    }

    FATAL_ERROR("finishing module %d with a fatal error", m);
    while (1);
    return 0;
}


