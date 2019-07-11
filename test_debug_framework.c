
#include "debug_framework.h"

char *correct = "correct";
char *incorrect = "*** INCORRECT ***";
char *M = "TestModule";

int rcount = 0;
module_debug_flag_t flag = 0;

int recurse (void)
{
    ENTER_FUNCTION();
    ERROR(M, "TeStInG %d", rcount);
    ERROR(M, "TeStInG 2 %d", rcount);
    ERROR(M, "TeStInG 3 %d", rcount);
    if (rcount++ < 30) recurse();
    ERROR(M, "exiting current function");
    ERROR(M, "exiting current function 2");
    ERROR(M, "exiting current function 3");
    EXIT_FUNCTION(0);
}

int main (int argc, char *argv[])
{
    int i;

    debugger_initialize(NULL);
    debugger_set_reporting_function(NULL);
    DISABLE_FUNCTION_TRACING();

    for (i = 0; i < 5; i++) {

        DISABLE_ALL_DEBUG_MESSAGES(flag);

        DEBUG(M, flag, "%s", incorrect);
        INFO(M, flag, "%s", incorrect);
        WARNING(M, flag, "%s", incorrect);
        ERROR(M, "%s", correct);
        
        ENABLE_DEBUG_MESSAGES(flag);
        DEBUG(M, flag, "%s", correct);
        INFO(M, flag, "%s", correct);
        WARNING(M, flag, "%s", correct);
        ERROR(M, "%s", correct);

        ENABLE_INFO_MESSAGES(flag);
        DEBUG(M, flag, "%s", incorrect);
        INFO(M, flag, "%s", correct);
        WARNING(M, flag, "%s", correct);
        ERROR(M, "%s", correct);

        ENABLE_WARNING_MESSAGES(flag);
        DEBUG(M, flag, "%s", incorrect);
        INFO(M, flag, "%s", incorrect);
        WARNING(M, flag, "%s", correct);
        ERROR(M, "%s", correct);

        DISABLE_ALL_DEBUG_MESSAGES(flag);
        DEBUG(M, flag, "%s", incorrect);
        INFO(M, flag, "%s", incorrect);
        WARNING(M, flag, "%s", incorrect);
        ERROR(M, "%s", correct);
    }

    ENABLE_FUNCTION_TRACING();
    recurse();

    FATAL_ERROR("CRASH", "CRASHING DELIBERATELY %d", i);
    return 0;
}




