
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
** will not be reported.  The EXCEPTION to this is that errors & fatal
** errors will ALWAYS be reported.
**
** Note that fatal error call will crash the system with an assert call.
** It should be used only as a last resort.
**
** This framework is thread safe.  This means an error report will be
** completely printed and finished before another thread can be
** executed which may have the potential to screw up the writing of
** the message.
**
** Note the word used as 'reporting' rather than 'printing' a message.  This
** is because the user can override the default printing function (which is
** simply writing to stderr) by a specified one and can do whatever in that
** specific function.  Typically that will be some sort of printing but it 
** can be anything defined by the user.  Note that another debug/error
** cannot be called inside the user defined function or deadlock will
** result.
**
** To activate this in the source code, #include 'INCLUDE_DEBUGGING_CODE'.
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
 * Debug levels, the higher the number, the higher the priority.
 * (Must fit into an unsigned byte).
 * ERROR_LEVEL always gets reported.
 * Critical errors will also always report AND crash the system.
 */
#define TRACE_DEBUG_LEVEL               0
#define INFORM_DEBUG_LEVEL              1
#define WARNING_DEBUG_LEVEL             2
#define ERROR_DEBUG_LEVEL               3
#define FATAL_ERROR_DEBUG_LEVEL         4
#define MAX_DEBUG_LEVEL                 FATAL_ERROR_DEBUG_LEVEL

#define MODULE_NAME_LEN                 48
typedef struct module_name_s {
    char module_name [MODULE_NAME_LEN];
} module_name_t;

/* like printf */
typedef int (*debug_reporting_function)(const char *format, ...);

extern int n_modules;
extern byte *module_debug_levels;
extern lock_obj_t *p_debugger_lock;

static inline
int invalid_module_number (int m)
{ return (m < 0) || (m >= n_modules); }

static inline 
void GRAB_WRITE_LOCK (lock_obj_t *lck)
{ if (lck) grab_write_lock(lck); }

static inline void
RELEASE_WRITE_LOCK (lock_obj_t *lck)
{ if (lck) release_write_lock(lck); }

/*
 * If you do NOT want debugging code to be generated/executed,
 * then UN define this.
 */
#define INCLUDE_DEBUGGING_CODE

#ifdef INCLUDE_DEBUGGING_CODE

    static inline
    int invalid_debug_level (int l)
    { return  (l < TRACE_DEBUG_LEVEL) || (l > MAX_DEBUG_LEVEL); }

    static inline void
    debug_set_module_level (int m, byte l)
    {
        if (invalid_module_number(m)) return;
        if (invalid_debug_level(l)) return;
        module_debug_levels[m] = l;
    }

    #define TRACE(m, fmt, args...) \
        do { \
            if (invalid_module_number(m)) break; \
            if (module_debug_levels[m] > TRACE_DEBUG_LEVEL) break; \
            GRAB_WRITE_LOCK(p_debugger_lock); \
            _process_debug_message_(m, TRACE_DEBUG_LEVEL, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
            RELEASE_WRITE_LOCK(p_debugger_lock); \
        } while (0)
    
    #define INFORMATION(m, fmt, args...) \
        do { \
            if (invalid_module_number(m)) break; \
            if (module_debug_levels[m] > INFORM_DEBUG_LEVEL) break; \
            GRAB_WRITE_LOCK(p_debugger_lock); \
            _process_debug_message_(m, INFORM_DEBUG_LEVEL, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
            RELEASE_WRITE_LOCK(p_debugger_lock); \
        } while (0)
    
    #define WARNING(m, fmt, args...) \
        do { \
            if (invalid_module_number(m)) break; \
            if (module_debug_levels[m] > WARNING_DEBUG_LEVEL) break; \
            GRAB_WRITE_LOCK(p_debugger_lock); \
            _process_debug_message_(m, WARNING_DEBUG_LEVEL, \
                    __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
            RELEASE_WRITE_LOCK(p_debugger_lock); \
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
        GRAB_WRITE_LOCK(p_debugger_lock); \
        _process_debug_message_(m, ERROR_DEBUG_LEVEL, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        RELEASE_WRITE_LOCK(p_debugger_lock); \
    } while (0)

/* fatal errors are ALWAYS reported */
#define FATAL_ERROR(m, fmt, args...) \
    do { \
        if (invalid_module_number(m)) break; \
        GRAB_WRITE_LOCK(p_debugger_lock); \
        _process_debug_message_(m, FATAL_ERROR_DEBUG_LEVEL, \
            __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        RELEASE_WRITE_LOCK(p_debugger_lock); \
        assert(0); \
    } while (0)

extern int
debug_initialize (int make_it_thread_safe,
    debug_reporting_function fn, int num_modules);

extern void 
debug_set_reporting_function (debug_reporting_function fn);

extern int
debug_set_module_name (int module, char *name);

/**************************************************************************/

/*
 * PRIVATE, DO NOT USE.  DEFINED ONLY TO PASS COMPILATIONS.
 */
extern void
_process_debug_message_ (int module, int level,
    const char *file_name, const char *function_name, int line_number,
    char *fmt, ...);

#ifdef __cplusplus
} // extern C
#endif

#endif /* __DEBUG_FRAMEWORK_H__ */



