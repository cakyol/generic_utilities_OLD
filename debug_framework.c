
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
static char *module_names [MAX_MODULES] = { 0 };
static char *debug_level_names [DEBUG_LEVEL_SPAN] =
    { "DEBUG", "INFO", "WARNING", "ERROR", "FATAL_ERROR" };

static void
default_debug_reporting_function (char *debug_string)
{
    fprintf(stderr, "%s", debug_string);
    fflush(stderr);
}

static
debug_reporting_function_t current_drf = &default_debug_reporting_function;

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
    if ((module > 0) && (module < MAX_MODULES)) {
        module_names[module] = module_name;
        return 0;
    }
    return -1;
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
    index = 0;

    if (0 == module_names[module]) {

    }

    /* write module number in case grep on module is needed */
    len = snprintf(&msg_buffer[index], size_left,
                "%s: mod: %d, fn: %s, ln: %d >> ",
                debug_level_names[level], module, function_name, line_number);
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
    if (level >= FATAL_ERROR_LEVEL) assert(1);
}

#ifdef __cplusplus
} // extern C
#endif




