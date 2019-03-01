
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

int recurse (void)
{
    F_ENTER;
    if (rcount++ < 30) recurse();
    F_RETURN(0);
}

int main (int argc, char *argv[])
{
    int i, flag;

    debugger_set_reporting_function(NULL);
    //debugger_set_reporting_function(report);

    for (i = 0; i < 5; i++) {

        DEBUGGER_DISABLE_ALL(flag);

        DEBUG("module", flag, "%s", incorrect);
        INFO("module", flag, "%s", incorrect);
        WARNING("module", flag, "%s", incorrect);
        ERROR("module", flag, "%s", correct);
        
        DEBUGGER_ENABLE_DEBUGS(flag);
        DEBUG("module", flag, "%s", correct);
        INFO("module", flag, "%s", correct);
        WARNING("module", flag, "%s", correct);
        ERROR("module", flag, "%s", correct);

        DEBUGGER_ENABLE_INFOS(flag);
        DEBUG("module", flag, "%s", incorrect);
        INFO("module", flag, "%s", correct);
        WARNING("module", flag, "%s", correct);
        ERROR("module", flag, "%s", correct);

        DEBUGGER_ENABLE_WARNINGS(flag);
        DEBUG("module", flag, "%s", incorrect);
        INFO("module", flag, "%s", incorrect);
        WARNING("module", flag, "%s", correct);
        ERROR("module", flag, "%s", correct);

        DEBUGGER_DISABLE_ALL(flag);
        DEBUG("module", flag, "%s", incorrect);
        INFO("module", flag, "%s", incorrect);
        WARNING("module", flag, "%s", incorrect);
        ERROR("module", flag, "%s", correct);
    }

    ENABLE_FUNCTION_TRACING();
    recurse();

    FATAL_ERROR("CRASH", "CRASHING DELIBERATELY %d", i);
    return 0;
}




