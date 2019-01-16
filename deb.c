
#include "deb.h"

const char *debug_string = "DEBUG";
const char *information_string = "INFORMATION";
const char *warning_string = "WARNING";
const char *error_string = "ERROR";
const char *fatal_error_string = "FATAL_ERROR";

static void
default_debug_reporting_function (const char *msg)
{
    fprintf(stderr, "%s", msg);
}

static 
debug_reporting_function_pointer debug_reporter = default_debug_reporting_function;

void
debugger_set_reporting_function (debug_reporting_function_pointer fn)
{
    if (fn) {
        debug_reporter = fn;
    } else {
        debug_reporter = default_debug_reporting_function;
    }
}

void
_process_debug_message_ (char *module_name, const char *level,
    const char *file_name, const char *function_name, int line_number,
    char *fmt, ...)
{

#define DEBUG_MESSAGE_BUFFER_SIZE      256

    va_list args;
    char msg_buffer [DEBUG_MESSAGE_BUFFER_SIZE];
    int index, size_left, len;

    size_left = DEBUG_MESSAGE_BUFFER_SIZE - 1;
    len = index = 0;
    len += snprintf(&msg_buffer[index], size_left,
		"%s: module %s: <%s: %s(%d)> ",
		level, module_name, file_name, function_name, line_number);

    size_left -= len;
    index += len;

    va_start(args, fmt);
    len += vsnprintf(&msg_buffer[index], size_left, fmt, args);
    va_end(args);

    /* add a newline and terminate with a null */
    msg_buffer[len] = '\n';
    msg_buffer[len + 1] = 0;

    /* precaution */
    msg_buffer[DEBUG_MESSAGE_BUFFER_SIZE - 1] = 0;

    /*
     * do the actual printing/reporting operation here using
     * the currently registered debug printing function
     */
    debug_reporter(msg_buffer);
}

char *correct = "correct";
char *incorrect = "*** INCORRECT ***";

void report (char *msg)
{
    static int xxx = 0;
    xxx++;
    printf("%d\n", xxx);
}

int main (int argc, char *argv[])
{
    int i, flag;

    debugger_set_reporting_function(NULL);

    for (i = 0; i < 5; i++) {

        DISABLE_ALL_DEBUGS(flag);

        DEBUG("module", flag, "%s", incorrect);
        INFORMATION("module", flag, "%s", incorrect);
        WARNING("module", flag, "%s", incorrect);
        ERROR("module", flag, "%s", incorrect);
        
        ENABLE_DEBUG_LEVEL(flag);
        DEBUG("module", flag, "%s", correct);
        INFORMATION("module", flag, "%s", correct);
        WARNING("module", flag, "%s", correct);
        ERROR("module", flag, "%s", correct);

        ENABLE_INFORMATION_LEVEL(flag);
        DEBUG("module", flag, "%s", incorrect);
        INFORMATION("module", flag, "%s", correct);
        WARNING("module", flag, "%s", correct);
        ERROR("module", flag, "%s", correct);

        ENABLE_WARNING_LEVEL(flag);
        DEBUG("module", flag, "%s", incorrect);
        INFORMATION("module", flag, "%s", incorrect);
        WARNING("module", flag, "%s", correct);
        ERROR("module", flag, "%s", correct);

        ENABLE_ERROR_LEVEL(flag);
        DEBUG("module", flag, "%s", incorrect);
        INFORMATION("module", flag, "%s", incorrect);
        WARNING("module", flag, "%s", incorrect);
        ERROR("module", flag, "%s", correct);
    }

    FATAL_ERROR("CRASH", "CRASHING DELIBERATELY %d", i);
    return 0;
}




