
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
    ENTER_FUNCTION();
    ERROR(NULL, flag, "TeStInG %d", rcount);
    ERROR(NULL, flag, "TeStInG 2 %d", rcount);
    ERROR(NULL, flag, "TeStInG 3 %d", rcount);
    if (rcount++ < 30) recurse();
    ERROR(NULL, flag, "exiting current function");
    ERROR(NULL, flag, "exiting current function 2");
    ERROR(NULL, flag, "exiting current function 3");
    EXIT_FUNCTION(0);
}

int main (int argc, char *argv[])
{
    int i;

    debugger_set_reporting_function(NULL);
    //debugger_set_reporting_function(report);

    for (i = 0; i < 5; i++) {

        DISABLE_ALL_DEBUG_MESSAGES(flag);

        DEBUG(NULL, flag, "%s", incorrect);
        INFO(NULL, flag, "%s", incorrect);
        WARNING(NULL, flag, "%s", incorrect);
        ERROR(NULL, flag, "%s", correct);
        
        ENABLE_DEBUG_MESSAGES(flag);
        DEBUG(NULL, flag, "%s", correct);
        INFO(NULL, flag, "%s", correct);
        WARNING(NULL, flag, "%s", correct);
        ERROR(NULL, flag, "%s", correct);

        ENABLE_INFO_MESSAGES(flag);
        DEBUG(NULL, flag, "%s", incorrect);
        INFO(NULL, flag, "%s", correct);
        WARNING(NULL, flag, "%s", correct);
        ERROR(NULL, flag, "%s", correct);

        ENABLE_WARNING_MESSAGES(flag);
        DEBUG(NULL, flag, "%s", incorrect);
        INFO(NULL, flag, "%s", incorrect);
        WARNING(NULL, flag, "%s", correct);
        ERROR(NULL, flag, "%s", correct);

        DISABLE_ALL_DEBUG_MESSAGES(flag);
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




