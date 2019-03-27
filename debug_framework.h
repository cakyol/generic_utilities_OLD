
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
** 'my_module' flog is set to the warning level, debugs and informations
** will not be reported.
**
** Note that fatal error call will crash the system with an assert call.
** It should be used only as a last resort.
**
** Note the word used as 'reporting' rather than 'printing' a message.  This
** is because the user can decide what to do exactly with the message.
** Typically that will be printing but it does not have to be.  It can be 
** redefined by the user.  By default however, the message is printed to
** stderr.
**
** To activate this in the source code, #include 'INCLUDE_ALL_DEBUGGING_CODE'.
** Otherwise, all debug statements will compile to nothing meaning they will
** NOT impose ANY overhead to the code.  There is an exception to this
** and that is, errors and fatal errors will ALWAYS be reported.
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

/*
 * This is a debug flag which contains the level for which debug
 * messages for it can be processed.  Every module can have its 
 * own debug flag.
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
 * User can redefine the reporting function by setting a new function
 * using this call.
 */
extern void
debugger_set_reporting_function (debug_reporting_function_pointer fn);

extern unsigned char function_trace_on;
extern unsigned int function_trace_indent;
extern const char *function_entered, *function_exited;
extern char function_trace_string [];
extern debug_reporting_function_pointer debug_reporter;

#define INCLUDE_ALL_DEBUGGING_CODE

#ifdef INCLUDE_ALL_DEBUGGING_CODE

    /* Turn off function entry/exit tracing */
    static inline void DISABLE_FUNCTION_TRACING (void)
    { function_trace_on = 0; }

    static inline void ENABLE_FUNCTION_TRACING (void)
    { function_trace_on = 1; }

    /* Turn off all debugging for this module */
    #define DEBUGGER_DISABLE_ALL(module_debug_flag) \
        (module_debug_flag = 0)

    /* Turn on all debugging from lowest level up */
    #define DEBUGGER_ENABLE_DEBUGS(module_debug_flag) \
        (module_debug_flag = DEBUG_LEVEL)

    /* Turn on all debug messages information level & up */
    #define DEBUGGER_ENABLE_INFOS(module_debug_flag) \
        (module_debug_flag = INFORMATION_LEVEL)

    /* Turn on all debug messages warning level & up */
    #define DEBUGGER_ENABLE_WARNINGS(module_debug_flag) \
        (module_debug_flag = WARNING_LEVEL)

    /* function entered notification */
    #define ENTER_FUNCTION() \
        do { \
            if (function_trace_on) { \
                sprintf(function_trace_string, "%*s%s%s (line %d)\n", \
                    function_trace_indent, " ", function_entered, \
                        __FUNCTION__, __LINE__); \
                function_trace_indent++; \
                debug_reporter(function_trace_string); \
            } \
        } while (0)

        /* function exit notification */
        #define EXIT_FUNCTION(value) \
        do { \
            if (function_trace_on) { \
                function_trace_indent--; \
                sprintf(function_trace_string, "%*s%s%s (line %d)\n", \
                    function_trace_indent, " ", function_exited, \
                        __FUNCTION__, __LINE__); \
                debug_reporter(function_trace_string); \
            } \
            return (value); \
        } while (0)

    #define DEBUG(module_name, module_debug_flag, fmt, args...) \
        do { \
            if (module_debug_flag & DEBUG_LEVEL_MASK) { \
                _process_debug_message_(module_name, debug_string, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
            } \
        } while (0)
    
    #define INFO(module_name, module_debug_flag, fmt, args...) \
        do { \
            if (module_debug_flag & INFORMATION_LEVEL_MASK) { \
                _process_debug_message_(module_name, info_string, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
            } \
        } while (0)
    
    #define WARNING(module_name, module_debug_flag, fmt, args...) \
        do { \
            if (module_debug_flag & WARNING_LEVEL_MASK) { \
                _process_debug_message_(module_name, warning_string, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
            } \
        } while (0)
    
#else /* ! INCLUDE_ALL_DEBUGGING_CODE */

    #define DISABLE_FUNCTION_TRACING()
    #define ENABLE_FUNCTION_TRACING()

    #define DEBUGGER_DISABLE_ALL(module_debug_flag)
    #define DEBUGGER_ENABLE_DEBUGS(module_debug_flag)
    #define DEBUGGER_ENABLE_INFOS(module_debug_flag)
    #define DEBUGGER_ENABLE_WARNINGS(module_debug_flag)
    
    #define ENTER_FUNCTION()
    #define EXIT_FUNCTION(value)   return(value)
 
    #define DEBUG(module_name, module_debug_flag, fmt, args...)
    #define INFO(module_name, module_debug_flag, fmt, args...)
    #define WARNING(module_name, module_debug_flag, fmt, args...)

#endif /* ! INCLUDE_ALL_DEBUGGING_CODE */

/* errors are ALWAYS reported */
#define ERROR(module_name, module_debug_flag, fmt, args...) \
    do { \
        _process_debug_message_(module_name, error_string, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
    } while (0)

/* fatal errors are ALWAYS reported */
#define FATAL_ERROR(module_name, fmt, args...) \
    do { \
        _process_debug_message_(module_name, fatal_error_string, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
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



