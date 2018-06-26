
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
** It ALWAYS is and WILL remain the property of Cihangir Metin Akyol.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "debug_framework.h"

/*
 * all modules' default error reporting level is set to ERROR_LEVEL
 */
unsigned char module_debug_levels [MAX_MODULES] = { ERROR_LEVEL };

/*
 * If user registers a module name, that gets reported instead of
 * just the module number (by default).
 */
static char *module_names [MAX_MODULES] = { 0 };

/*
 * debug level number to debug level name string lookup table.
 */
static char *debug_level_names [DEBUG_LEVEL_SPAN] = { "???" };

static void
default_debug_reporting_function (char *debug_string)
{
    fprintf(stderr, "%s", debug_string);
    fflush(stderr);
}

static
debug_reporting_function_t current_drf = &default_debug_reporting_function;

void
debug_framework_init (debug_reporting_function_t drf)
{
    int i;

    debug_level_names[DEBUG_LEVEL] = "DEBUG MESSAGE";
    debug_level_names[INFORMATION_LEVEL] = "INFORMATION MESSAGE";
    debug_level_names[WARNING_LEVEL] = "WARNING MESSAGE";
    debug_level_names[ERROR_LEVEL] = "ERROR MESSAGE";
    debug_level_names[FATAL_ERROR_LEVEL] = "FATAL ERROR MESSAGE";

    register_debug_reporting_function(drf);

    for (i = 0; i < MAX_MODULES; i++) {
	module_debug_levels[i] = ERROR_LEVEL;
	register_module_name(i, NULL);
    }
}

/*
 * set a single module's error/debug reporting thereshold
 */
int 
set_module_debug_level (int module, int level)
{
    /* check module validity */
    if ((module < 1) || (module >= MAX_MODULES)) return EINVAL;

    /* trim debug level to limits */
    if (level < DEBUG_LEVEL) 
        level = DEBUG_LEVEL;
    else if (level > FATAL_ERROR_LEVEL) 
        level = FATAL_ERROR_LEVEL;

    /* set it */
    module_debug_levels[module] = (unsigned char) level;

    /* done */
    return 0;
}

int
register_module_name (int module, char *module_name)
{
    /* check module limits */
    if ((module <= 0) || (module >= MAX_MODULES)) return EINVAL;

    /* free up older module name, if specified */
    if (module_names[module]) {
        free(module_names[module]);
        module_names[module] = NULL;
    }

    /* name is being cleared, so register its number form */
    if (NULL == module_name) {
        module_names[module] = malloc(16);
        if (NULL == module_names[module]) return ENOMEM;
	sprintf(module_names[module], "%d", module);
    /* name is specified, so record it */
    } else {
        module_names[module] = malloc(64);
        if (NULL == module_names[module]) return ENOMEM;
        strncpy(module_names[module], module_name, 63);
    }

    return 0;
}

void
register_debug_reporting_function (debug_reporting_function_t drf)
{
    current_drf = drf ? drf : &default_debug_reporting_function;
}

void
report_debug_message (int module, int level,
    const char *function_name, int line_number,
    char *fmt, ...)
{

#define DEBUG_MESSAGE_BUFFER_SIZE      512

    va_list args;
    char msg_buffer [DEBUG_MESSAGE_BUFFER_SIZE];
    int index, size_left, len;

    size_left = DEBUG_MESSAGE_BUFFER_SIZE;
    len = index = 0;

    /* module number 0 ALWAYS reports */
    if (0 == module) {
        len += snprintf(&msg_buffer[index], size_left,
                    "fn <%s> ln %d %s: ",
		    function_name,
		    line_number,
		    debug_level_names[level]);

    /* for others, check if module is allowed to report */
    } else if ((module > 0) && (module < MAX_MODULES)) {

#if 0
        /* if module name is not specified, create its number form */
        if (NULL == module_names[module]) {
            register_module_name(module, NULL);
        }
#endif

        len += snprintf(&msg_buffer[index], size_left,
                    "mod <%s> fn <%s> ln %d %s: ",
		    module_names[module],
		    function_name,
		    line_number,
                    debug_level_names[level]);

    /* invalid module number */
    } else {
        return;
    }
    
    size_left -= len;
    index += len;

    va_start(args, fmt);
    len += vsnprintf(&msg_buffer[index], size_left, fmt, args);
    va_end(args);

    /* add a newline and terminate with a null */
    msg_buffer[len] = '\n';
    msg_buffer[len + 1] = 0;

    /*
     * do the actual printing/reporting operation here using
     * the currently registered debug printing function
     */
    current_drf(msg_buffer);

    /* fatal error MUST ALWAYS crash the system */
    if (level >= FATAL_ERROR_LEVEL) assert(0);
}

#ifdef __cplusplus
} // extern C
#endif




