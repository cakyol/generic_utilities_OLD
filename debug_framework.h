
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

#define DEBUG_LEVEL                 0
#define INFORMATION_LEVEL           1
#define WARNING_LEVEL               2
#define ERROR_LEVEL                 3
#define FATAL_ERROR_LEVEL           4
#define DEBUG_LEVEL_SPAN            (FATAL_ERROR_LEVEL - DEBUG_LEVEL + 1)

/*
 * This is a function which takes a printable string and 'reports' it.
 * The 'reporting' is up to the user, it can be a printf, kernel print,
 * write to a file, etc, etc....
 */
typedef void (*debug_reporting_function_t)(char*);

extern int
set_module_debug_level (int module, int level);

extern int
register_module_name (int module, char *module_name);

/*
 * Register your own debug reporting function to be called.  If NULL is passed,
 * the default reporting function will be used, which is printing to the 
 * standard error file.
 */
extern void
register_debug_reporting_function (debug_reporting_function_t drf);

#define REPORT_DEBUG(module, fmt, args...) \
    if (module_can_report(module, DEBUG_LEVEL)) \
        report_debug_message(module, DEBUG_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_INFORMATION(module, fmt, args...) \
    if (module_can_report(module, INFORMATION_LEVEL)) \
        report_debug_message(module, INFORMATION_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_WARNING(module, fmt, args...) \
    if (module_can_report(module, WARNING_LEVEL)) \
        report_debug_message(module, WARNING_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_ERROR(module, fmt, args...) \
    if (module_can_report(module, ERROR_LEVEL)) \
        report_debug_message(module, ERROR_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args)

/*
 * fatal errors ALWAYS report, no need to check
 */
#define REPORT_FATAL_ERROR(module, fmt, args...) \
    report_debug_message(module, FATAL_ERROR_LEVEL, \
        __FUNCTION__, __LINE__, fmt, ## args)

/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/

#if MAX_DEBUG_LEVEL > 255
    #error  "max debug level must be < 256"
#endif

extern unsigned char 
module_debug_levels [MAX_MODULES];

/*
 * module 0 ALWAYS gets printed, wildcard.
 * Otherwise, the level must be allowed for that particular module to print.
 */
static inline int
module_can_report (int module, int level) 
{
    return 
        (module == 0) ||
        ((module > 0) && (module < MAX_MODULES) && 
            (level >= module_debug_levels[module]));
}

extern void
report_debug_message (int module, int level,
    const char *function_name, int line_number,
    char *fmt, ...);

#ifdef __cplusplus
} // extern C
#endif

#endif // __DEBUG_FRAMEWORK_H__

