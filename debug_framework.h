
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

/******* ONLY USE THESE, REST IS PRIVATE AND SHOULD NOT BE USED **************/

/*
 * How many modules this debug infrastructure supports
 */
#define MAX_MODULES                 1024

#define REPORT_DEBUG(module, fmt, args...) \
    if (module_can_report(module, DEBUG_DEBUG_LEVEL) \
        print_debug_message(module, DEBUG_DEBUG_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args))

#define REPORT_INFORMATION(module, fmt, args...) \
    if (module_can_report(module, INFORMATION_DEBUG_LEVEL) \
        print_debug_message(module, INFORMATION_DEBUG_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args))

#define REPORT_WARNING(module, fmt, args...) \
    if (module_can_report(module, WARNING_DEBUG_LEVEL) \
        print_debug_message(module, WARNING_DEBUG_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_ERROR(module, fmt, args...) \
    if (module_can_report(module, ERROR_DEBUG_LEVEL) \
        print_debug_message(module, ERROR_DEBUG_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args))

#define REPORT_FATAL_ERROR(module, fmt, args...) \
    print_debug_message(module, FATAL_DEBUG_LEVEL, \
        __FUNCTION__, __LINE__, fmt, ## args)

extern int
set_module_debug_level (int module, int level);

/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/

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

extern unsigned char 
module_debug_levels [MAX_MODULES];

static inline int
module_can_report (int module, int level) 
{
    return 
        (module >= 0) && (module < MAX_MODULES) &&
        (level >= module_debug_levels[level]);

}

extern void
print_debug_message (int module, int level,
    char *function_name, int line_number,
    char *fmt, ...);

#ifdef __cplusplus
} // extern C
#endif

#endif // __DEBUG_FRAMEWORK_H__

