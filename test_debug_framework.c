
#include <unistd.h>
#include "debug_framework.h"

int main (int argc, char *argv[])
{
    int m;
    char name [64];

    debug_framework_init(NULL);
    for (m = 0; m < 25; m++) {
	sprintf(name, "MODULE%d", m);
	register_module_name(m, name);
        if (m & 1) {
            set_module_debug_level(m, ERROR_LEVEL);
            REPORT_DEBUG(m, "m%d SHOULD NOT be reported", m);
            REPORT_INFORMATION(m, "m%d SHOULD NOT be reported", m);
            REPORT_WARNING(m, "m%d SHOULD NOT be reported", m);
            REPORT_ERROR(m, "m%d should be reported", m);
        } else {
            set_module_debug_level(m, DEBUG_LEVEL);
            REPORT_DEBUG(m, "m%d should be reported", m);
            REPORT_INFORMATION(m, "m%d should be reported", m);
            REPORT_WARNING(m, "m%d should be reported", m);
            REPORT_ERROR(m, "m%d should be reported", m);
        }
    }
    REPORT_FATAL_ERROR(m, "finishing module %d with a fatal error", m);
    while (1);
    return 0;
}


