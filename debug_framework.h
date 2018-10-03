
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
** are 0 -> MAX_MODULES-1.  
**
** Module names can also be 'registered' so that when the reporting is done,
** module names can be reported (rather than simply module numbers).
**
** Note that the error level 'FATAL_ERROR' will crash the system with an assert
** call.  It should be used only as a last resort.
**
** Note the word used as 'reporting' rather than 'printing' a message.  Every
** module can have its own specific reporting function that the user defines
** and that is what will be called by the framework.  The user can perform
** whatever they want with the message.  Typically that will be printing but
** it does not have to be.  It is defined by the user.  By default however,
** for every module, a function is registered which prints to stderr only.
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

/*
 * Define this if you want the debug infrastructure to be
 * included in your system.  Not defining this will save
 * a bit of code size and run slightly faster.
 * Note that regardless of this directive, ERRORS &
 * FATAL_ERRORS will ALWAYS be reported.
 */
// #undef DEBUGGING_ENABLED

#ifndef DEBUGGING_ENABLED
#define DEBUGGING_ENABLED
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

/*
 * Undefine this otherwise assert may not work properly.
 */
#undef NDEBUG
#include <assert.h>

/*
 * How many modules this debug infrastructure supports.
 * Redefine this as you wish.
 */
#define MAX_MODULES                     64

/*
 * Define your own modules here, like such.
 *
 * like:
 *
 * #define SWITCH_MODULE           0
 * #define ROUTER_MODULE           1
 * #define BUFFER_MODULE           7
 * etc.....
 *
 */

/*
 * max number of characters of a module name if the user defines
 * a verbose and printable name for a specific module.  Otherwise,
 * by default, the system will assign the name "mN" where N is
 * the module number.  For example, module 17's default name will
 * be "M_17".
 */
#define MODULE_NAME_SIZE                32

/*
 * levels of debugging
 */
#define LOWEST_DEBUG_LEVEL	    0
#define DEBUG_LEVEL                 LOWEST_DEBUG_LEVEL
#define INFORMATION_LEVEL           1
#define WARNING_LEVEL               2
#define ERROR_LEVEL                 3
#define FATAL_ERROR_LEVEL           4
#define HIGHEST_DEBUG_LEVEL	    FATAL_ERROR_LEVEL

/*
 * The definition of 'reporting' function.  User specified function of what
 * needs to be done with the message.  It can be printf'd, kprintf'd, 
 * written to a file, whatever.  The string is formatted, null terminated
 * and passed to this function.  Rest is up to the function itself to do 
 * whatever it wants with the passed string.
 *
 * User can re-define his/her own reporting function for a module which
 * should meet the sytax below.
 */
typedef void (*debug_reporting_function_t)(char*);

/*
 * Call this to initialize the debug framework.
 * For all modules, default levels, reporting functions
 * and module names will be used.
 *
 * The default reporting level is ERROR_LEVEL.
 * The default printing function will printf to stderr.
 * The default module name will be m_27 (27th module).
 */
extern void
debug_init (void);

/*
 * Functions below set various parameters for the module.
 * passing NULL as pointers will reset those values back
 * to the defaults.
 */
extern int
debug_module_set_name (int module, char *module_name);

extern int
debug_module_set_level (int module, int level);

extern int
debug_module_set_reporting_function (int module,
        debug_reporting_function_t drf);

/*
 * ***************************************************************************
 * These macros report (or not) depending on the debug level
 * threshold set for the specific module.
 */

#ifdef EXCLUDE_ALL_DEBUGGING_CODE

#define DEBUG(module, fmt, args...)
#define INFO(module, fmt, args...)
#define WARNING(module, fmt, args...)

#else /* include debugging */

#define DEBUG(module, fmt, args...) \
    do { \
	if (__module_can_report__(module, DEBUG_LEVEL)) { \
	    debug_message_process(module, DEBUG_LEVEL, \
		__FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
	} \
    } while (0)

#define INFO(module, fmt, args...) \
    do { \
	if (__module_can_report__(module, INFORMATION_LEVEL)) { \
	    debug_message_process(module, INFORMATION_LEVEL, \
		__FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
	} \
    } while (0)

#define WARNING(module, fmt, args...) \
    do { \
	if (__module_can_report__(module, WARNING_LEVEL)) { \
	    debug_message_process(module, WARNING_LEVEL, \
		__FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
	} \
    } while (0)

#endif /* EXCLUDE_ALL_DEBUGGING_CODE */

/*
 * Errors will ALWAYS be reported.
 * Furthermore, a Fatal error will crash the process (with an assert).
 */
#define ERROR(module, fmt, args...) \
    do { \
	if ((module >= 0) && (module < MAX_MODULES)) { \
	    debug_message_process(module, ERROR_LEVEL, \
		__FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
	} \
    } while (0)

#define FATAL_ERROR(module, fmt, args...) \
    do { \
	if ((module >= 0) && (module < MAX_MODULES)) { \
	    debug_message_process(module, FATAL_ERROR_LEVEL, \
		__FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
	} \
    } while (0)

/******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/* PRIVATE, DO NOT USE ANYTHING BELOW HERE DIRECTLY, ONLY USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW HERE DIRECTLY, ONLY USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW HERE DIRECTLY, ONLY USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW HERE DIRECTLY, ONLY USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW HERE DIRECTLY, ONLY USE THE MACROS ABOVE */
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/

/*
 * each module can have a level set below which the debug messages
 * do not get reported.  Also, each one does have its own 'reporting' 
 * function.  This helps report each module differently if the need be.
 * Some can write to the output, some into a file,. etc etc.  It is
 * totally user driven.
 *
 * By default however, the drf is simply writing to stderr.
 */
typedef struct debug_module_data_s {

        char name [MODULE_NAME_SIZE];
        debug_reporting_function_t drf;
        unsigned char level;

} debug_module_data_t;

extern void
default_debug_reporting_function (char *debug_message);

extern
debug_module_data_t module_levels [MAX_MODULES];

/*
 * Report if the current debug level set for this module is less than or
 * equal to the level specified in the user call.
 */

static inline int
__module_can_report__ (int module, int level) 
{
    return
	(module >= 0) && (module < MAX_MODULES) &&
	(level >= module_levels[module].level);
}

extern void
debug_message_process (int module, int level,
    const char *file_name, const char *function_name, int line_number,
    char *fmt, ...);

#ifdef __cplusplus
} // extern C
#endif

#endif // __DEBUG_FRAMEWORK_H__

