
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#undef NDEBUG
#include <assert.h>

#define INCLUDE_ALL_DEBUGGING_CODE

typedef unsigned char module_debug_flag_t;

#ifdef INCLUDE_ALL_DEBUGGING_CODE

    #define DISABLE_ALL_DEBUGS(module_debug_flag) \
        (module_debug_flag = 0)

    #define ENABLE_DEBUG_LEVEL(module_debug_flag) \
        (module_debug_flag = DEBUG_LEVEL)

    #define ENABLE_INFORMATION_LEVEL(module_debug_flag) \
        (module_debug_flag = INFORMATION_LEVEL)

    #define ENABLE_WARNING_LEVEL(module_debug_flag) \
        (module_debug_flag = WARNING_LEVEL)

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

    #define FATAL_ERROR(module_name, fmt, args...) \
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




