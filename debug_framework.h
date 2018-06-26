
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

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** This is a very generic debug framework which 'reports' an error for a module.
** There are about 5 levels of debugging defined and every 'module' can have
** its own individual level set.  Modules are defined by the user.  Modules
** are 1 -> MAX_MODULES.  0 is a special modue number and ALL levels of
** messages will be reported for module 0.  Module names can also be 'registered'
** so that when the reporting is done, module names can be reported (rather
** than simply module numbers).
**
** Note that the error level 'FATAL_ERROR' will crash the system with an assert
** call.  It should be used only as a last resort.
**
** Note the word used as 'reporting' rather than 'printing' a message.  The
** framework can be registered with a reporting function that the user defines
** and that is what will be called by the framework.  The user can perform
** whatever they want with the message.  Typically that will be printing but
** it does not have to be.  It is defined by the user.  By default, a function
** is registered which prints to stderr only.
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

/*
 * Undefine this otherwise assrt does not do anything
 */
#undef NDEBUG
#include <assert.h>

/******* ONLY USE THESE, REST IS PRIVATE AND SHOULD NOT BE USED **************/
/******* ONLY USE THESE, REST IS PRIVATE AND SHOULD NOT BE USED **************/
/******* ONLY USE THESE, REST IS PRIVATE AND SHOULD NOT BE USED **************/
/******* ONLY USE THESE, REST IS PRIVATE AND SHOULD NOT BE USED **************/
/******* ONLY USE THESE, REST IS PRIVATE AND SHOULD NOT BE USED **************/
/******* ONLY USE THESE, REST IS PRIVATE AND SHOULD NOT BE USED **************/
/******* ONLY USE THESE, REST IS PRIVATE AND SHOULD NOT BE USED **************/

/*
 * How many modules this debug infrastructure supports.
 * Redefine this if more needs to be supported.
 */
#define MAX_MODULES                 256

/*
 * Generic string 'reporting' function.  User specified function of what
 * needs to be done with the message.  It can be printf'd, kprintf'd, 
 * written to a file, whatever.
 */
typedef void (*debug_reporting_function_t)(char*);

/*
 * call this to initialize the debug framework.  If drf is NULL,
 * the default reporting funtion will be used.  The default
 * reporting function writes to stderr.
 */
extern void
debug_framework_init (debug_reporting_function_t drf);

/*
 * This function sets the debug level for a module such that reporting
 * requests lower than this value will not be reported.  By default,
 * all modules are at error level, meaning messages ONLY >= error level
 * will be 'reported'.  Note that module 0 is special in the sense that
 * ALL messages for level 0 will always be reported.  This function will
 * ignore it if 0 is passed as the module.
 */
extern int
set_module_debug_level (int module, int level);

/*
 * This function updates the module name of a module so it can be 
 * reported more verbosely.  If NULL is passed, the name is cancelled
 * and the numeric form of the module will be reported. Valid modules
 * are 1 to MAX_MODULES.  Note that module 0 is special and matches
 * 'all modules' and no name will be printed for module 0.
 */
extern int
register_module_name (int module, char *module_name);

/*
 * This is a function which registers a function to be called when a
 * message needs to be reported.
 *
 * The 'reporting' is up to the user, it can be a printf, kernel print,
 * write to a file, etc, etc.... The user can do anything with that
 * printable string.  
 *
 * By default, the reporting function prints to stderr.  Setting 
 * this to NULL also defaults the behaviour.
 */
extern void
register_debug_reporting_function (debug_reporting_function_t drf);

/*
 * passing 0 for 'module' below will ALWAYS report the message.
 */
#define REPORT_DEBUG(module, fmt, args...) \
    if (module_can_report(module, DEBUG_LEVEL)) \
        process_debug_message(module, DEBUG_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_INFORMATION(module, fmt, args...) \
    if (module_can_report(module, INFORMATION_LEVEL)) \
        process_debug_message(module, INFORMATION_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_WARNING(module, fmt, args...) \
    if (module_can_report(module, WARNING_LEVEL)) \
        process_debug_message(module, WARNING_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_ERROR(module, fmt, args...) \
    if (module_can_report(module, ERROR_LEVEL)) \
        process_debug_message(module, ERROR_LEVEL, \
            __FUNCTION__, __LINE__, fmt, ## args)

/*
 * fatal errors ALWAYS report, no need to check
 */
#define REPORT_FATAL_ERROR(module, fmt, args...) \
    process_debug_message(module, FATAL_ERROR_LEVEL, \
        __FUNCTION__, __LINE__, fmt, ## args)

/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/
/******* PRIVATE, DO NOT USE; PRIVATE, DO NOT USE; PRIVATE, DO NOT USE *******/

#define DEBUG_LEVEL                 0
#define INFORMATION_LEVEL           1
#define WARNING_LEVEL               2
#define ERROR_LEVEL                 3
#define FATAL_ERROR_LEVEL           4
#define DEBUG_LEVEL_SPAN            (FATAL_ERROR_LEVEL - DEBUG_LEVEL + 1)

#if MAX_DEBUG_LEVEL > 255
    #error  "max debug level must be < 256"
#endif

extern unsigned char 
module_debug_levels [MAX_MODULES];

/*
 * module 0 ALWAYS gets printed, wildcard.
 * Otherwise, the level must be allowed for that particular module to report.
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
process_debug_message (int module, int level,
    const char *function_name, int line_number,
    char *fmt, ...);

#ifdef __cplusplus
} // extern C
#endif

#endif // __DEBUG_FRAMEWORK_H__

