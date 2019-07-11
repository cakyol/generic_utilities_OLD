
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
** It ALWAYS is and WILL remain the sole property of Cihangir Metin Akyol.
**
** For proper indentation/viewing, regardless of which editor is being used,
** no tabs are used, ONLY spaces are used and the width of lines never
** exceed 80 characters.  This way, every text editor/terminal should
** display the code properly.  If modifying, please stick to this
** convention.
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
** This is a generic debug framework which 'reports' bugs/errors based on
** the level allowed for a module.
**
** There are 5 levels of debugging: debug, information, warning, error and
** fatal error in that specific order.  If a specific level is set for a
** module, lower level debugs will not be reported.  For example, if module
** 'my_module' flag is set to the warning level, debugs and informations
** will not be reported.  The exception to this is that errors & fatal
** errors will ALWAYS be reported.
**
** Note that fatal error call will crash the system with an assert call.
** It should be used only as a last resort.
**
** Note the word used as 'reporting' rather than 'printing' a message.  This
** is because the user can decide what to do exactly with the message.
** Typically that will be printing but it does not have to be.  It can be 
** redefined by the user.  By default however, the message is printed to
** stderr.  If reporting function is redefined by the user, it should NOT
** call any debug/error messages itelf otherwise a deadlock will result.
**
** To activate this in the source code, #include 'INCLUDE_ALL_DEBUGGING_CODE'.
** Otherwise, all debug statements will compile to nothing meaning they will
** NOT impose ANY overhead to the code.
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
#include <assert.h>

#include "common.h"
#include "lock_object.h"

/*
 * If you do NOT want debugging code to be generated/executed,
 * then UN define this.
 */
#define INCLUDE_ALL_DEBUGGING_CODE

/*
 * This is a debug flag which contains the level for which debug
 * messages for it can be processed.  Every module can have its 
 * own debug flag.  The user defines his/her modules.
 */
typedef unsigned char module_debug_flag_t;

/*
 * By default, all outputs from this debug framework go to stderr.
 * If you want to perform different operations, define your own
 * handling function.
 *
 * The function takes one char* parameter, which is the fully 
 * formatted, newline & NULL terminated string.  It returns
 * nothing.
 *
 * Your function should NOT change the string.
 */
typedef void (*debug_reporting_function_pointer)(const char *msg);

/*
 * Always call this to initialize the debugger subsystem
 */
extern void
debugger_initialize (debug_reporting_function_pointer fn);

/*
 * User can redefine the reporting function by setting a new function
 * using this call.
 */
extern void
debugger_set_reporting_function (debug_reporting_function_pointer fn);

#ifdef INCLUDE_ALL_DEBUGGING_CODE

    /* Turn off function entry/exit tracing */
    #define DISABLE_FUNCTION_TRACING() \
        (function_trace_on = 0)

    /* Turn on function entry/exit tracing */
    #define ENABLE_FUNCTION_TRACING() \
        (function_trace_on = 1)

    /* Turn off all debugging for this module */
    #define DISABLE_ALL_DEBUG_MESSAGES(module_debug_flag) \
        (module_debug_flag = 0)

    /* Turn on all debugging from lowest level up */
    #define ENABLE_DEBUG_MESSAGES(module_debug_flag) \
        (module_debug_flag = DEBUG_LEVEL)

    /* Turn on all debug messages information level & up */
    #define ENABLE_INFO_MESSAGES(module_debug_flag) \
        (module_debug_flag = INFORMATION_LEVEL)

    /* Turn on all debug messages warning level & up */
    #define ENABLE_WARNING_MESSAGES(module_debug_flag) \
        (module_debug_flag = WARNING_LEVEL)

    /* function entered notification */
    #define ENTER_FUNCTION() \
        do { \
            if (function_trace_on) { \
                grab_write_lock(&debugger_lock); \
                sprintf(function_trace_string, "%*s(%d) %s (line %d)\n", \
                    function_trace_indent+7, function_entered, \
                    function_trace_indent, __FUNCTION__, __LINE__); \
                function_trace_indent++; \
                debug_reporter(function_trace_string); \
                release_write_lock(&debugger_lock); \
            } \
        } while (0)

    /* function exit notification */
    #define EXIT_FUNCTION(value) \
        do { \
            if (function_trace_on) { \
                grab_write_lock(&debugger_lock); \
                function_trace_indent--; \
                sprintf(function_trace_string, "%*s(%d) %s (line %d)\n", \
                    function_trace_indent+6, function_exited, \
                    function_trace_indent, __FUNCTION__, __LINE__); \
                debug_reporter(function_trace_string); \
                release_write_lock(&debugger_lock); \
            } \
            return (value); \
        } while (0)

    #define DEBUG(module_name, module_debug_flag, fmt, args...) \
        do { \
            if (module_debug_flag & DEBUG_LEVEL_MASK) { \
                grab_write_lock(&debugger_lock); \
                _process_debug_message_(module_name, debug_string, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
                release_write_lock(&debugger_lock); \
            } \
        } while (0)
    
    #define INFO(module_name, module_debug_flag, fmt, args...) \
        do { \
            if (module_debug_flag & INFORMATION_LEVEL_MASK) { \
                grab_write_lock(&debugger_lock); \
                _process_debug_message_(module_name, info_string, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
                release_write_lock(&debugger_lock); \
            } \
        } while (0)
    
    #define WARNING(module_name, module_debug_flag, fmt, args...) \
        do { \
            if (module_debug_flag & WARNING_LEVEL_MASK) { \
                grab_write_lock(&debugger_lock); \
                _process_debug_message_(module_name, warning_string, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
                release_write_lock(&debugger_lock); \
            } \
        } while (0)
    
#else /* ! INCLUDE_ALL_DEBUGGING_CODE */

    #define DISABLE_FUNCTION_TRACING()
    #define ENABLE_FUNCTION_TRACING()

    #define DISABLE_ALL_DEBUG_MESSAGES(module_debug_flag)
    #define ENABLE_DEBUG_MESSAGES(module_debug_flag)
    #define ENABLE_INFO_MESSAGES(module_debug_flag)
    #define ENABLE_WARNING_MESSAGES(module_debug_flag)
    
    #define ENTER_FUNCTION()
    #define EXIT_FUNCTION(value)   return(value)
 
    #define DEBUG(module_name, module_debug_flag, fmt, args...)
    #define INFO(module_name, module_debug_flag, fmt, args...)
    #define WARNING(module_name, module_debug_flag, fmt, args...)

#endif /* ! INCLUDE_ALL_DEBUGGING_CODE */

/* errors are ALWAYS reported */
#define ERROR(module_name, fmt, args...) \
    do { \
        grab_write_lock(&debugger_lock); \
        _process_debug_message_(module_name, error_string, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        release_write_lock(&debugger_lock); \
    } while (0)

/* fatal errors are ALWAYS reported */
#define FATAL_ERROR(module_name, fmt, args...) \
    do { \
        grab_write_lock(&debugger_lock); \
        _process_debug_message_(module_name, fatal_error_string, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        release_write_lock(&debugger_lock); \
        assert(0); \
    } while (0)

/*****************************************************************************
 *****************************************************************************
 *****************************************************************************
 *****************************************************************************
 *
 * DO NOT TOUCH OR USE ANYTHING BELOW HERE
 *
 *****************************************************************************
 *****************************************************************************
 *****************************************************************************
 ****************************************************************************/

extern lock_obj_t debugger_lock;
extern unsigned char function_trace_on;
extern unsigned int function_trace_indent;
extern const char *function_entered, *function_exited;
extern char function_trace_string [];
extern debug_reporting_function_pointer debug_reporter;

#define DEBUG_LEVEL             (0b00000001)
#define DEBUG_LEVEL_MASK        (0b00000001)

#define INFORMATION_LEVEL       (0b00000010)
#define INFORMATION_LEVEL_MASK  (0b00000011)

#define WARNING_LEVEL           (0b00000100)
#define WARNING_LEVEL_MASK      (0b00000111)

extern const char *debug_string;
extern const char *info_string;
extern const char *warning_string;
extern const char *error_string;
extern const char *fatal_error_string;

extern void
_process_debug_message_ (char *module_name, const char *level,
    const char *file_name, const char *function_name, int line_number,
    char *fmt, ...);

#ifdef __cplusplus
} // extern C
#endif

#endif /* __DEBUG_FRAMEWORK_H__ */



