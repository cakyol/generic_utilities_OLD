
#include "debug_framework.h"

int main (int argc, char *argv[])
{
    int m;

    for (m = 0; m < 25; m++) {
        if (m & 1) {
	    register_module_name(m, "odd module");
            set_module_debug_level(m, ERROR_LEVEL);
            REPORT_DEBUG(m, "m%d SHOULD NOT be reported", m);
            REPORT_INFORMATION(m, "m%d SHOULD NOT be reported", m);
            REPORT_WARNING(m, "m%d SHOULD NOT be reported", m);
            REPORT_ERROR(m, "m%d SHOULD be reported", m);
        } else {
            set_module_debug_level(m, DEBUG_LEVEL);
            REPORT_DEBUG(m, "m%d SHOULD be reported", m);
            REPORT_INFORMATION(m, "m%d SHOULD be reported", m);
            REPORT_WARNING(m, "m%d SHOULD be reported", m);
            REPORT_ERROR(m, "m%d SHOULD be reported", m);
        }
    }
    return 0;
}


