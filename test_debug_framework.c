
#include "debug_framework.h"

char *correct = "correct";
char *incorrect = "*** INCORRECT ***";

void report (const char *msg)
{
    static int xxx = 0;
    xxx++;
    printf("%d\n", xxx);
}

int rcount = 0;
module_debug_flag_t flag = 0;

int recurse (void)
{
    F_ENTER();
    ERROR(NULL, flag, "TeStInG %d", rcount);
    if (rcount++ < 30) recurse();
    ERROR(NULL, flag, "exiting current function");
    F_EXIT(0);
}

int main (int argc, char *argv[])
{
    int i;

    debugger_set_reporting_function(NULL);
    //debugger_set_reporting_function(report);

    for (i = 0; i < 5; i++) {

        DEBUGGER_DISABLE_ALL(flag);

        DEBUG(NULL, flag, "%s", incorrect);
        INFO(NULL, flag, "%s", incorrect);
        WARNING(NULL, flag, "%s", incorrect);
        ERROR(NULL, flag, "%s", correct);
        
        DEBUGGER_ENABLE_DEBUGS(flag);
        DEBUG(NULL, flag, "%s", correct);
        INFO(NULL, flag, "%s", correct);
        WARNING(NULL, flag, "%s", correct);
        ERROR(NULL, flag, "%s", correct);

        DEBUGGER_ENABLE_INFOS(flag);
        DEBUG(NULL, flag, "%s", incorrect);
        INFO(NULL, flag, "%s", correct);
        WARNING(NULL, flag, "%s", correct);
        ERROR(NULL, flag, "%s", correct);

        DEBUGGER_ENABLE_WARNINGS(flag);
        DEBUG(NULL, flag, "%s", incorrect);
        INFO(NULL, flag, "%s", incorrect);
        WARNING(NULL, flag, "%s", correct);
        ERROR(NULL, flag, "%s", correct);

        DEBUGGER_DISABLE_ALL(flag);
        DEBUG(NULL, flag, "%s", incorrect);
        INFO(NULL, flag, "%s", incorrect);
        WARNING(NULL, flag, "%s", incorrect);
        ERROR(NULL, flag, "%s", correct);
    }

    ENABLE_FUNCTION_TRACING();
    recurse();

    FATAL_ERROR("CRASH", "CRASHING DELIBERATELY %d", i);
    return 0;
}




