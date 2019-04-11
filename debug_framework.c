
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol, gee.akyol@gmail.com, gee_akyol@yahoo.com
** Copyright: Cihangir Metin Akyol, April 2014 -> ....
**
** All this code has been personally developed by and belongs to 
** Mr. Cihangir Metin Akyol.  It has been developed in his own 
** personal time using his own personal resources.  Therefore,
** it is NOT owned by any establishment, group, company or 
** consortium.  It is the sole property and work of the named
** individual.
**
** It CAN be used by ANYONE or ANY company for ANY purpose as long 
** as ownership and/or patent claims are NOT made to it by ANYONE
** or ANY ENTITY.
**
** It ALWAYS is and WILL remain the sole property of Cihangir Metin Akyol.
**
** For proper indentation/viewing, regardless of which editor is being used,
** no tabs are used, ONLY spaces are used and the width of lines never
** exceed 80 characters.  This way, every text editor/terminal should
** display the code properly.  If modifying, please stick to this
** convention.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#include "debug_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned char function_trace_on = 0;
unsigned int function_trace_indent = 0;
const char *function_entered = "ENTERED ";
const char *function_exited = "EXITED ";
lock_obj_t debugger_lock;
char function_trace_string [128] = { 0 };

const char *debug_string = "DEBUG";
const char *info_string = "INFO";
const char *warning_string = "WARNING";
const char *error_string = "ERROR";
const char *fatal_error_string = "FATAL";

static void
default_debug_reporting_function (const char *msg)
{
    fprintf(stderr, "%s", msg);
}

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
debugger_initialize (debug_reporting_function_pointer fn)
{
    lock_obj_init(&debugger_lock);
    debugger_set_reporting_function(fn);
}

/*
 * We use a global message buffer to save stack space and protect
 * it with a lock for multi threaded applications.
 */
#define DEBUG_MESSAGE_BUFFER_SIZE      1024
static char msg_buffer [DEBUG_MESSAGE_BUFFER_SIZE];

void
_process_debug_message_ (char *module_name, const char *level,
    const char *file_name, const char *function_name, int line_number,
    char *fmt, ...)
{
    va_list args;
    int index, size_left, len;

    size_left = DEBUG_MESSAGE_BUFFER_SIZE - 1;
    len = index = 0;

    /*
     * align the error messages to be
     * printed also with the proper indentation.
     */
    len += snprintf(&msg_buffer[index], size_left,
		"%*s%s: %s<%s: %s(%d)> ",
        function_trace_indent, " ", level,
        module_name ? module_name : "",
        file_name, function_name, line_number);

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

#ifdef __cplusplus
} // extern C
#endif



