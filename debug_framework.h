
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

#ifndef __DEBUG_FRAMEWORK_H__
#define __DEBUG_FRAMEWORK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

/*
 * How many modules this debug infrastructure supports
 */
#define MAX_MODULES                 1024

#define MIN_DEBUG_LEVEL             0
#define DEBUG_DEBUG_LEVEL           MIN_DEBUG_LEVEL
#define INFORMATION_DEBUG_LEVEL     10
#define WARNING_DEBUG_LEVEL         20
#define ERROR_DEBUG_LEVEL           30
#define FATAL_DEBUG_LEVEL           40
#define MAX_DEBUG_LEVEL             FATAL_DEBUG_LEVEL

#if MAX_DEBUG_LEVEL > 255
    #error  "max debug level must be < 256"
#endif

extern int
set_module_debug_level (int module, int level);

#define REPORT_DEBUG(module, fmt, args...) \
    print_debug_message(module, DEBUG_DEBUG_LEVEL, \
        __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_INFORMATION(module, fmt, args...) \
    print_debug_message(module, INFORMATION_DEBUG_LEVEL, \
        __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_WARNING(module, fmt, args...) \
    print_debug_message(module, WARNING_DEBUG_LEVEL, \
        __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_ERROR(module, fmt, args...) \
    print_debug_message(module, ERROR_DEBUG_LEVEL, \
        __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_FATAL_ERROR(module, fmt, args...) \
    print_debug_message(module, FATAL_DEBUG_LEVEL, \
        __FUNCTION__, __LINE__, fmt, ## args)

/*
 * Do not use this directly, always use the macros above instead
 */
extern int
print_debug_message (int module, int level,
    char *function_name, int line_number,
    char *fmt, ...);

#ifdef __cplusplus
} // extern C
#endif

#endif // __DEBUG_FRAMEWORK_H__

