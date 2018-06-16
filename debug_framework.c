
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
 * all modules' default error reporting level is set to ERROR_DEBUG_LEVEL
 */
unsigned char module_debug_levels [MAX_MODULES] = { ERROR_DEBUG_LEVEL };

/*
 * set a single module's error/debug reporting thereshold
 */
int 
set_module_debug_level (int module, int level)
{
    /* check module validity */
    if ((module < 0) || (module >= MAX_MODULES)) return EINVAL;

    /* trim debug level to limits */
    if (level < MIN_DEBUG_LEVEL) 
        level = MIN_DEBUG_LEVEL;
    else if (level > MAX_DEBUG_LEVEL) 
        level = MAX_DEBUG_LEVEL;

    /* set it */
    module_debug_levels[module] = (unsigned char) level;

    /* done */
    return 0;
}

void
print_debug_message (int module, int level,
    char *function_name, int line_number,
    char *fmt, ...)
{

#define DEBUG_MESSAGE_BUFFER_SIZE      512

    va_list args;
    char msg_buffer [DEBUG_MESSAGE_BUFFER_SIZE];
    int index, size_left, len;

    size_left = DEBUG_MESSAGE_BUFFER_SIZE;
    index = 0;

    /* write module number in case grep on module is needed */
    len = snprintf(&msg_buffer[index], size_left,
                "M: %d, F: %s, L: %d >> ", module, function_name, line_number);
    size_left -= len;
    index += len;

    va_start(args, fmt);
    vsnprintf(&msg_buffer[index], size_left, fmt, args);
    va_end(args);

    /* do the actual printing operation here */

    /* fatal error MUST ALWAYS crash the system */
    if (level >= FATAL_DEBUG_LEVEL) assert(1);
}

#ifdef __cplusplus
} // extern C
#endif




