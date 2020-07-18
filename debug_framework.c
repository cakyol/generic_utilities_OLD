
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

int n_modules = 0;
lock_obj_t debugger_lock = { 0 };
byte *module_debug_levels = 0;

static debug_reporting_function user_specified_drf = 0;
static module_name_t *module_names = 0;

static const char *level_strings [] = { 
    [TRACE_DEBUG_LEVEL] = "TRACE",
    [INFORM_DEBUG_LEVEL] = "INFORMATION",
    [WARNING_DEBUG_LEVEL] = "WARNING",
    [ERROR_DEBUG_LEVEL] = "ERROR",
    [FATAL_ERROR_DEBUG_LEVEL] = "FATAL_ERROR" };

/*
 * We use a global static message buffer for sprintf
 * when a user specified printing function is required.
 * This can be global even for multi threaded apps since
 * the debug macros are protected by locks.
 */
#define DEBUG_MESSAGE_BUFFER_SIZE      2048
static char *static_msg_buffer = 0;

static void
set_default_module_name (int m)
{ sprintf(&module_names[m].module_name[0], "MODULE_%d", m); }

int
debug_framework_initialize (debug_reporting_function fn, int n_m)
{
    int m;

    lock_obj_init(&debugger_lock);
    set_debug_reporting_function(fn);
    if (n_m <= 0) return EINVAL;
    
    n_modules = n_m;
    module_names = malloc(n_modules * sizeof(module_name_t));
    if (0 == module_names) {
        n_modules = 0;
        return ENOMEM;
    }
    for (m = 0; m < n_modules; m++) set_default_module_name(m);

#ifdef INCLUDE_DEBUGGING_CODE
    module_debug_levels = malloc(n_modules * sizeof(byte));
    if (0 == module_debug_levels) {
        free(module_names);
        module_names = 0;
        n_modules = 0;
        return ENOMEM;
    }
    for (m = 0; m < n_modules; m++)
        module_debug_levels[m] = ERROR_DEBUG_LEVEL;
#endif /* INCLUDE_DEBUGGING_CODE */

    return 0;

}

/*
 * If 'fn' is null, the default function is used.
 * The static message buffer is used only when a user specified
 * non null debug printing function is used.  Otherwise it is not
 * needed and released to save memory.
 */
void
set_debug_reporting_function (debug_reporting_function fn)
{
    if (fn) {

        /*
         * allocate static message buffer if never allocated before.
         * If allocation fails, force it back to the default.
         */
        if (0 == static_msg_buffer) {
            static_msg_buffer = malloc(DEBUG_MESSAGE_BUFFER_SIZE);
            if (0 == static_msg_buffer) fn = 0;
        }

    } else {

        /*
         * we are going back to the default print function.  We dont
         * need the static message buffer if it was allocated before.
         */
        if (static_msg_buffer) {
            free(static_msg_buffer);
            static_msg_buffer = 0;
        }
    }

    user_specified_drf = fn;
}

int
set_module_name (int m, char *name)
{
    if (invalid_module_number(m)) return EINVAL;
    if (name && name[0]) {
        strncpy(&module_names[m].module_name[0], name, MODULE_NAME_LEN-2);
    } else {
        set_default_module_name(m);
    }
    return 0;
}

/*
 * When this function is called, both the module number
 * and level are already verified (thru the macros calling it)
 * so range check does not need to be done again.
 */
void
_process_debug_message_ (int module, int level,
    const char *file_name, const char *function_name,
    int line_number,
    char *fmt, ...)
{
    va_list args;
    int index, size_left, len;

    va_start(args, fmt);

    /*
     * if an external reporting function has not been 
     * defined, then simply use fprintf(stderr, ...);
     */
    if (0 == user_specified_drf) {
        fprintf(stderr, "%s: %s: %s(%d): %s: ",
            level_strings[level], module_names[module].module_name,
            file_name, line_number, function_name);
        vfprintf(stderr, fmt, args);
        fflush(stderr);
        return;
    }

    size_left = DEBUG_MESSAGE_BUFFER_SIZE - 1;
    len = index = 0;

    len += snprintf(&static_msg_buffer[index], size_left,
                "%s: %s: %s(%d): %s: ",
        level_strings[level], module_names[module].module_name,
        file_name, line_number, function_name);

    size_left -= len;
    index += len;

    len += vsnprintf(&static_msg_buffer[index], size_left, fmt, args);
    va_end(args);

    /* terminate with a null */
    static_msg_buffer[len] = 0;

    /* precaution */
    static_msg_buffer[DEBUG_MESSAGE_BUFFER_SIZE - 1] = 0;

    /*
     * do the actual printing/reporting operation here using
     * the currently registered debug printing function
     */
    user_specified_drf(static_msg_buffer);
}

#ifdef __cplusplus
} // extern C
#endif



