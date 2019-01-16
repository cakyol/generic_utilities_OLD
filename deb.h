
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

#ifndef __DEBUG__H__
#define __DEBUG__H__

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

extern void
debugger_set_reporting_function (debug_reporting_function_pointer fn);

#define INCLUDE_ALL_DEBUGGING_CODE

#ifdef INCLUDE_ALL_DEBUGGING_CODE

    /* Turn off all debugging for this module */
    #define DISABLE_ALL_DEBUGS(module_debug_flag) \
        (module_debug_flag = 0)

    /* Turn on all debugging from lowest level up */
    #define ENABLE_DEBUG_LEVEL(module_debug_flag) \
        (module_debug_flag = DEBUG_LEVEL)

    /* Turn on all debug messages information level & up */
    #define ENABLE_INFORMATION_LEVEL(module_debug_flag) \
        (module_debug_flag = INFORMATION_LEVEL)

    /* Turn on all debug messages warning level & up */
    #define ENABLE_WARNING_LEVEL(module_debug_flag) \
        (module_debug_flag = WARNING_LEVEL)

    /* Turn on all debug messages error level & up */
    #define ENABLE_ERROR_LEVEL(module_debug_flag) \
        (module_debug_flag = ERROR_LEVEL)

    #define DEBUG(module_name, module_debug_flag, fmt, args...) \
        do { \
            if (module_debug_flag & DEBUG_LEVEL_MASK) { \
                _process_debug_message_(module_name, debug_string, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
            } \
        } while (0)
    
    #define INFORMATION(module_name, module_debug_flag, fmt, args...) \
        do { \
            if (module_debug_flag & INFORMATION_LEVEL_MASK) { \
                _process_debug_message_(module_name, information_string, \
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
    
    #define ERROR(module_name, module_debug_flag, fmt, args...) \
        do { \
            if (module_debug_flag & ERROR_LEVEL_MASK) { \
                _process_debug_message_(module_name, error_string, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
            } \
        } while (0)

    #define FATAL(module_name, fmt, args...) \
        do { \
            _process_debug_message_(module_name, fatal_error_string, \
                __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
            assert(0); \
        } while (0)
    
#else /* ! INCLUDE_ALL_DEBUGGING_CODE */
 
    #define DISABLE_ALL_DEBUGS(module_debug_flag)
    #define ENABLE_DEBUG_LEVEL(module_debug_flag)
    #define ENABLE_INFORMATION_LEVEL(module_debug_flag)
    #define ENABLE_WARNING_LEVEL(module_debug_flag)
    #define ENABLE_ERROR_LEVEL(module_debug_flag)

    #define DEBUG(module_name, module_debug_flag, fmt, args...)
    #define INFORMATION(module_name, module_debug_flag, fmt, args...)
    #define WARNING(module_name, module_debug_flag, fmt, args...)
    #define ERROR(module_name, module_debug_flag, fmt, args...)
    #define FATAL_ERROR(module_name, fmt, args...)

#endif /* ! INCLUDE_ALL_DEBUGGING_CODE */

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

#define ERROR_LEVEL             (0b00001000)
#define ERROR_LEVEL_MASK        (0b00001111)

#define FATAL_ERROR_LEVEL       (0b00010000)

extern const char *debug_string;
extern const char *information_string;
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

#endif /* __DEBUG__H__ */



