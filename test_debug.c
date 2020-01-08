
#include "debug.h"

char *correct = "correct\n";
char *incorrect = "*** INCORRECT ***\n";

int main (int argc, char *argv[])
{
    int i;

    debug_init(1);
    for (i = 0; i < 5; i++) {

        set_module_debug_level(0, ERROR_LEVEL);
        TRACE(0, "%s", incorrect);
        INFORM(0, "%s", incorrect);
        WARN(0, "%s", incorrect);
        ERROR(0, "%s", correct);
        
        set_module_debug_level(0, TRACE_LEVEL);
        TRACE(0, "%s", correct);
        INFORM(0, "%s", correct);
        WARN(0, "%s", correct);
        ERROR(0, "%s", correct);

        set_module_debug_level(0, INFORM_LEVEL);
        TRACE(0, "%s", incorrect);
        INFORM(0, "%s", correct);
        WARN(0, "%s", correct);
        ERROR(0, "%s", correct);

        set_module_debug_level(0, WARNING_LEVEL);
        TRACE(0, "%s", incorrect);
        INFORM(0, "%s", incorrect);
        WARN(0, "%s", correct);
        ERROR(0, "%s", correct);

        set_module_debug_level(0, ERROR_LEVEL);
        TRACE(0, "%s", incorrect);
        INFORM(0, "%s", incorrect);
        WARN(0, "%s", incorrect);
        ERROR(0, "%s", correct);
    }
}




