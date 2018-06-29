
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
** messages WILL be reported for module 0.  Module names can also be 'registered'
** so that when the reporting is done, module names can be reported (rather
** than simply module numbers).
**
** Note that the error level 'FATAL_ERROR' will crash the system with an assert
** call.  It should be used only as a last resort.
**
** Note the word used as 'reporting' rather than 'printing' a message.  Every
** module can have its own specific reporting function that the user defines
** and that is what will be called by the framework.  The user can perform
** whatever they want with the message.  Typically that will be printing but
** it does not have to be.  It is defined by the user.  By default, initially
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
 * Redefine this if more needs to be supported.
 */
#define MAX_MODULES			128

/*
 * max number of characters of a module name if the user defines
 * a verbose and printable name for a specific module.  Otherwise,
 * by default, the system will assign the name "mN" where N is
 * the module number.  For example, module 17's default name will
 * be "m17".
 */
#define MODULE_NAME_SIZE		32

/*
 * Default string 'reporting' function.  User specified function of what
 * needs to be done with the message.  It can be printf'd, kprintf'd, 
 * written to a file, whatever.  The string is formatted, null terminated
 * and passed to the user registered function.  Rest is up to the function
 * itself to do whatever it wants with the passed string.
 *
 * User can re-define his/her own reporting function for a module which
 * should meet the definition below.
 */
typedef void (*debug_reporting_function_t)(char*);

/*
 * Call this to initialize the debug framework.
 * For all modules, default levels & reporting functions will be used.
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

#define MODULE_DEBUG(module, fmt, args...) \
    if (debug_module_can_report(module, DEBUG_LEVEL)) \
        debug_message_process(module, DEBUG_LEVEL, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

#define MODULE_INFO(module, fmt, args...) \
    if (debug_module_can_report(module, INFORMATION_LEVEL)) \
        debug_message_process(module, INFORMATION_LEVEL, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

#define MODULE_WARNING(module, fmt, args...) \
    if (debug_module_can_report(module, WARNING_LEVEL)) \
        debug_message_process(module, WARNING_LEVEL, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

#define MODULE_ERROR(module, fmt, args...) \
    if (debug_module_can_report(module, ERROR_LEVEL)) \
        debug_message_process(module, ERROR_LEVEL, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

/*
 * ***************************************************************************
 * These macros ALWAYS report, regardless of module or level
 */

#define REPORT_DEBUG(fmt, args...) \
    debug_message_process(0, DEBUG_LEVEL, \
	    __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_INFO(fmt, args...) \
    debug_message_process(0, INFORMATION_LEVEL, \
	    __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_WARNING(fmt, args...) \
    debug_message_process(0, WARNING_LEVEL, \
	    __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

#define REPORT_ERROR(fmt, args...) \
    debug_message_process(0, ERROR_LEVEL, \
	    __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

/*
 * ***************************************************************************
 * fatal error ALWAYS report AND crash, regardless of module
 */
#define FATAL_ERROR(fmt, args...) \
    debug_message_process(0, FATAL_ERROR_LEVEL, \
        __FILE__, __FUNCTION__, __LINE__, fmt, ## args)

/* PRIVATE, DO NOT USE ANYTHING BELOW THIS LINE DIRECTLY, USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW THIS LINE DIRECTLY, USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW THIS LINE DIRECTLY, USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW THIS LINE DIRECTLY, USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW THIS LINE DIRECTLY, USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW THIS LINE DIRECTLY, USE THE MACROS ABOVE */
/* PRIVATE, DO NOT USE ANYTHING BELOW THIS LINE DIRECTLY, USE THE MACROS ABOVE */

#define DEBUG_LEVEL                 0
#define INFORMATION_LEVEL           1
#define WARNING_LEVEL               2
#define ERROR_LEVEL                 3
#define FATAL_ERROR_LEVEL           4
#define DEBUG_LEVEL_SPAN            (FATAL_ERROR_LEVEL - DEBUG_LEVEL + 1)
#if MAX_DEBUG_LEVEL > 255
    #error "max debug level must be < 256"
#endif

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
debug_module_data_t modules [MAX_MODULES];

/*
 * module 0 ALWAYS gets printed, wildcard.
 * Otherwise, the level must be allowed for that particular module to report.
 */
static inline int
debug_module_can_report (int module, int level) 
{
    return 
        (module == 0) ||
        ((module > 0) && (module < MAX_MODULES) && 
            (level >= modules[module].level));
}

extern void
debug_message_process (int module, int level,
    const char *file_name, const char *function_name, int line_number,
    char *fmt, ...);

#ifdef __cplusplus
} // extern C
#endif

#endif // __DEBUG_FRAMEWORK_H__

