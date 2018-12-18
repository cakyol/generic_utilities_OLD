
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

debug_module_data_t module_levels [MAX_MODULES] = {
    {
        { 0 },
        default_debug_reporting_function,
        ERROR_LEVEL
    }
};


/*
 * debug level number to debug level name string lookup table.
 */
#define DEBUG_LEVEL_SPAN	(HIGHEST_DEBUG_LEVEL - LOWEST_DEBUG_LEVEL + 1)
static char *debug_level_names [DEBUG_LEVEL_SPAN] = { "???" };

void
default_debug_reporting_function (char *debug_string)
{
    fprintf(stderr, "%s", debug_string);
    fflush(stderr);
}

static void
set_default_module_name (int m)
{
    sprintf(module_levels[m].name, "M_%d", m);
}

void
debug_init (void)
{
    int m;

    debug_level_names[DEBUG_LEVEL] = "DEBUG";
    debug_level_names[NOTIFICATION_LEVEL] = "NOTIFICATION";
    debug_level_names[WARNING_LEVEL] = "WARNING";
    debug_level_names[ERROR_LEVEL] = "ERROR";
    debug_level_names[FATAL_ERROR_LEVEL] = "***** FATAL ERROR *****";
    for (m = 0; m < MAX_MODULES; m++) {
        set_default_module_name(m);
        module_levels[m].level = ERROR_LEVEL;
        module_levels[m].drf = default_debug_reporting_function;
    }
}

#define CHECK_MODULE_NUMBER(m)	\
    if ((m < 0) || (m >= MAX_MODULES)) return EINVAL;

int
debug_module_set_name (int module, char *module_name)
{
    CHECK_MODULE_NUMBER(module);

    if (NULL == module_name) {
        set_default_module_name(module);
    } else {
        strncpy(module_levels[module].name, module_name,
	    (MODULE_NAME_SIZE - 1));
    }

    return 0;
}

int 
debug_module_set_minimum_reporting_level (int module, int level)
{
    CHECK_MODULE_NUMBER(module);

    /* clip level to boundaries */
    if (level < DEBUG_LEVEL) 
        level = DEBUG_LEVEL;
    else if (level > FATAL_ERROR_LEVEL) 
        level = FATAL_ERROR_LEVEL;

    module_levels[module].level = (unsigned char) level;

    return 0;
}

int
debug_module_set_reporting_function (int module,
        debug_reporting_function_t drf)
{
    CHECK_MODULE_NUMBER(module);
    module_levels[module].drf = drf ? drf : default_debug_reporting_function;
    return 0;
}

/*
 * This function assumes 'module' is valid & within bounds.
 */
void
_process_debug_message_ (int module, int level,
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
		"%s: %s: <%s: %s: %d> ",
		module_levels[module].name,
		debug_level_names[level],
		file_name,
		function_name,
		line_number);

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
    module_levels[module].drf(msg_buffer);

    /* fatal error MUST ALWAYS crash the system */
    if (level >= FATAL_ERROR_LEVEL) assert(0);
}

#ifdef __cplusplus
} // extern C
#endif




