
#include <unistd.h>
#include "debug_framework.h"

int main (int argc, char *argv[])
{
    int m;
    char name [64];

    debug_init(NULL);
    for (m = 0; m < 10; m++) {
	REPORT_DEBUG(0, "m%d SHOULD be reported", 0);
	REPORT_INFORMATION(0, "m%d SHOULD be reported", 0);
	REPORT_WARNING(0, "m%d SHOULD be reported", 0);
	REPORT_ERROR(0, "m%d should be reported", 0);
    }

    for (m = 0; m < 25; m++) {
	sprintf(name, "MODULE%d", m);
	debug_module_name_set(m, name);
        if (m & 1) {
            debug_level_set(m, ERROR_LEVEL);
            REPORT_DEBUG(m, "m%d SHOULD NOT be reported", m);
            REPORT_INFORMATION(m, "m%d SHOULD NOT be reported", m);
            REPORT_WARNING(m, "m%d SHOULD NOT be reported", m);
            REPORT_ERROR(m, "m%d should be reported", m);
        } else {
            debug_level_set(m, DEBUG_LEVEL);
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


