
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
byte *module_debug_levels = 0;
lock_obj_t *p_debugger_lock = 0;

static debug_reporting_function user_specified_drf = 0;
static lock_obj_t debugger_lock = { 0 };
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
 * the debug macros are protected by locks.  If a user
 * defined reporting function is not defined, then this is
 * not needed since by default, printing to stderr is used.
 */
#define DEBUG_MESSAGE_BUFFER_SIZE      2048

static void
debug_set_default_module_name (int m)
{ sprintf(&module_names[m].module_name[0], "MODULE_%d", m); }

int
debug_initialize (int make_it_thread_safe,
    debug_reporting_function fn, int n_m)
{
    int m;

    lock_obj_init(&debugger_lock);
    if (make_it_thread_safe) {
        p_debugger_lock = &debugger_lock;
    } else {
        p_debugger_lock = 0;
    }
    debug_set_reporting_function(fn);
    if (n_m <= 0) return EINVAL;
    
    n_modules = n_m;
    module_names = malloc(n_modules * sizeof(module_name_t));
    if (0 == module_names) {
        n_modules = 0;
        return ENOMEM;
    }
    for (m = 0; m < n_modules; m++) debug_set_default_module_name(m);

/*
 * saves memory if not defined
 */
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
debug_set_reporting_function (debug_reporting_function fn)
{
    user_specified_drf = fn;
}

int
debug_set_module_name (int m, char *name)
{
    if (invalid_module_number(m)) return EINVAL;
    if (name && name[0]) {
        strncpy(&module_names[m].module_name[0], name, MODULE_NAME_LEN-2);
    } else {
        debug_set_default_module_name(m);
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
    const char *file_name, const char *function_name, const int line_number,
    char *fmt, ...)
{
    va_list args;

    SAFE_WRITE_LOCK(p_debugger_lock);
    va_start(args, fmt);
    if (user_specified_drf) {
        user_specified_drf(module, level, file_name, function_name,
            line_number, fmt, args);
    } else {
        fprintf(stderr, "%s: %s: %s(%d): <%s>: ",
            level_strings[level], module_names[module].module_name,
            file_name, line_number, function_name);
        vfprintf(stderr, fmt, args);
        fflush(stderr);
    }

#if 0
    int index, size_left, len;

    size_left = DEBUG_MESSAGE_BUFFER_SIZE - 1;
    len = index = 0;

    len += snprintf(&static_msg_buffer[index], size_left,
                "%s: %s: %s(%d): <%s>: ",
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
#endif /* 0 */

    SAFE_WRITE_UNLOCK(p_debugger_lock);
}

#ifdef __cplusplus
} // extern C
#endif



