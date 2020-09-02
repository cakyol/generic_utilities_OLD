
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
** module, lower level debugs will not be reported.  The exception to this
** is errors (ERROR, FATAL_ERROR) which will ALWAYS be reported.
** Note that fatal error call will crash the system with an assert call.
** It should be used only as a last resort.
**
** This framework can be made thread safe.  This means a report/message 
** will be completely reported and finished before another thread can be
** executed which may have the potential to interfere in the reporting of
** the message.  This feature can be turned on at initialization time.
**
** Note the word used as 'reporting' rather than 'printing' a message.  This
** is because the user can override the default printing function (which is
** simply writing to stderr) by a user specified one.  It is expected that
** such a user defined function will typically do some sort of printing but it 
** can be anything defined by the user.  Note that another debug/error
** cannot be called inside the user defined function or deadlock will
** result.
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
 * Debug levels, the higher the number, the higher the priority.
 * (Must fit into an unsigned byte).
 * ERROR_LEVEL & FATAL_ERROR will always gets reported.
 * Critical errors will also always report AND crash the system.
 */
#define MIN_DEBUG_LEVEL                 0
#define TRACE_DEBUG_LEVEL               MIN_DEBUG_LEVEL
#define INFORM_DEBUG_LEVEL              1
#define WARNING_DEBUG_LEVEL             2
#define ERROR_DEBUG_LEVEL               3
#define FATAL_ERROR_DEBUG_LEVEL         4
#define MAX_DEBUG_LEVEL                 FATAL_ERROR_DEBUG_LEVEL

#define MODULE_NAME_LEN                 48
typedef struct module_name_s {
    char module_name [MODULE_NAME_LEN];
} module_name_t;

/* user will completely handle everything */
typedef void (*debug_reporting_function)
    (int module, int level,
     const char *file_name, const char *function_name, const int line_number,
     char *fmt, va_list args);

/* how many modules being debugged, initialized in debug_initialize */
extern int n_modules;

/* debug level per module, null if INCLUDE_DEBUGGING_CODE is false */
extern byte *module_debug_levels;

/* serialize multiple thread access to debug printing, if needed */
extern lock_obj_t *p_debugger_lock;

/*
 * Do we really need these, is the programmer really so incompetent
 * to supply an out of bounds number.  Omitting this increases
 * speed.  Besides, it can always be turned on later by changing
 * the 'if 0' below to 'if 1'.
 */
#if 0

    static inline
    int invalid_debug_level (int l)
    { return  (l < MIN_DEBUG_LEVEL) || (l > MAX_DEBUG_LEVEL); }

    static inline
    int invalid_module_number (int m)
    { return (m < 0) || (m >= n_modules); }

#else

    #define invalid_debug_level(l)      0
    #define invalid_module_number(m)    0

#endif

/*
 * If you do NOT want debugging code to be generated/executed,
 * then UN define this.
 */
#define INCLUDE_DEBUGGING_CODE

#ifdef INCLUDE_DEBUGGING_CODE

    static inline
    void debug_set_module_level (int m, byte l)
    {
        if (invalid_module_number(m)) return;
        if (invalid_debug_level(l)) return;
        module_debug_levels[m] = l;
    }

    #define TRACE(m, fmt, args...) \
        do { \
            if (invalid_module_number(m)) break; \
            if (module_debug_levels[m] > TRACE_DEBUG_LEVEL) break; \
            _process_debug_message_(m, TRACE_DEBUG_LEVEL, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        } while (0)
    
    #define INFORMATION(m, fmt, args...) \
        do { \
            if (invalid_module_number(m)) break; \
            if (module_debug_levels[m] > INFORM_DEBUG_LEVEL) break; \
            _process_debug_message_(m, INFORM_DEBUG_LEVEL, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        } while (0)
    
    #define WARNING(m, fmt, args...) \
        do { \
            if (invalid_module_number(m)) break; \
            if (module_debug_levels[m] > WARNING_DEBUG_LEVEL) break; \
            _process_debug_message_(m, WARNING_DEBUG_LEVEL, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        } while (0)
    
#else /* ! INCLUDE_DEBUGGING_CODE */

    #define debug_set_module_level(m, l)
    #define TRACE(m, fmt, args...)
    #define INFORMATION(m, fmt, args...)
    #define WARNING(m, fmt, args...)

#endif /* ! INCLUDE_DEBUGGING_CODE */

/*
 * These are always needed regardless since ERROR & FATAL_ERROR
 * will need some sort of debug infra support.
 */

/* errors are ALWAYS reported */
#define ERROR(m, fmt, args...) \
    do { \
        if (invalid_module_number(m)) break; \
        _process_debug_message_(m, ERROR_DEBUG_LEVEL, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
    } while (0)

/* fatal errors are ALWAYS reported */
#define FATAL_ERROR(m, fmt, args...) \
    do { \
        if (invalid_module_number(m)) break; \
        _process_debug_message_(m, FATAL_ERROR_DEBUG_LEVEL, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        assert(0); \
    } while (0)

extern int
debug_initialize (int make_it_thread_safe,
    debug_reporting_function fn, int num_modules);

extern void 
debug_set_reporting_function (debug_reporting_function fn);

extern int
debug_set_module_name (int module, char *name);

/**************************************************************************
 *
 * PRIVATE, DO NOT USE.  DEFINED ONLY TO PASS COMPILATIONS.
 *
 */
extern void
_process_debug_message_ (int module, int level,
    const char *file_name, const char *function_name, int line_number,
    char *fmt, ...);

#ifdef __cplusplus
} // extern C
#endif

#endif /* __DEBUG_FRAMEWORK_H__ */



