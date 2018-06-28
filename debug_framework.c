
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

debug_module_data_t modules [MAX_MODULES] = {
    {
	{ 0 },
	default_debug_reporting_function,
	ERROR_LEVEL
    }
};

/*
 * debug level number to debug level name string lookup table.
 */
static char *debug_level_names [DEBUG_LEVEL_SPAN] = { "???" };

void
default_debug_reporting_function (char *debug_string)
{
    fprintf(stderr, "%s", debug_string);
    fflush(stderr);
}

void
debug_init (void)
{
    int m;

    debug_level_names[DEBUG_LEVEL] = "DEBUG";
    debug_level_names[INFORMATION_LEVEL] = "INFO";
    debug_level_names[WARNING_LEVEL] = "WARNING";
    debug_level_names[ERROR_LEVEL] = "ERROR";
    debug_level_names[FATAL_ERROR_LEVEL] = "FATAL";

    for (m = 0; m < MAX_MODULES; m++) {
	sprintf(modules[m].name, "m%d", m);
	modules[m].level = ERROR_LEVEL;
	modules[m].drf = default_debug_reporting_function;
    }
}

#define BAIL_IF_BAD_MODULE_INDEX(m) \
    if (((m) < 1) || ((m) >= MAX_MODULES)) return EINVAL

int
debug_module_set_name (int module, char *module_name)
{
    BAIL_IF_BAD_MODULE_INDEX(module);

    if (NULL == module_name) {
	sprintf(modules[module].name, "m%d", module);
    } else {
        strncpy(modules[module].name, module_name, (MODULE_NAME_SIZE - 1));
    }

    return 0;
}

int 
debug_module_set_level (int module, int level)
{
    BAIL_IF_BAD_MODULE_INDEX(module);

    /* trim debug level to limits */
    if (level < DEBUG_LEVEL) 
        level = DEBUG_LEVEL;
    else if (level > FATAL_ERROR_LEVEL) 
        level = FATAL_ERROR_LEVEL;

    /* set it */
    modules[module].level = (unsigned char) level;

    /* done */
    return 0;
}

int
debug_module_set_reporting_function (int module,
	debug_reporting_function_t drf)
{
    BAIL_IF_BAD_MODULE_INDEX(module);
    modules[module].drf = drf ? drf : default_debug_reporting_function;
    return 0;
}

void
debug_message_process (int module, int level,
    const char *file_name, const char *function_name, int line_number,
    char *fmt, ...)
{

#define DEBUG_MESSAGE_BUFFER_SIZE      256

    va_list args;
    char msg_buffer [DEBUG_MESSAGE_BUFFER_SIZE];
    int index, size_left, len;

    size_left = DEBUG_MESSAGE_BUFFER_SIZE;
    len = index = 0;

    /* module number 0 ALWAYS reports */
    if (0 == module) {
        len += snprintf(&msg_buffer[index], size_left,
                    "%s <%s:%s:%d> ",
		    debug_level_names[level],
		    file_name,
		    function_name,
		    line_number);

    /* for others, check if module is allowed to report */
    } else if ((module > 0) && (module < MAX_MODULES)) {

        len += snprintf(&msg_buffer[index], size_left,
                    "%s <%s:%s:%s:%d> ",
		    debug_level_names[level],
		    modules[module].name,
		    file_name,
		    function_name,
		    line_number);

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
    modules[module].drf(msg_buffer);

    /* fatal error MUST ALWAYS crash the system */
    if (level >= FATAL_ERROR_LEVEL) assert(0);
}

#ifdef __cplusplus
} // extern C
#endif




