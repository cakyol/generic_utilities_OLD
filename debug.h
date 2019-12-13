
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

#ifndef __DEBUG_H__
#define __DEBUG_H__

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
 * Debug levels, lowest number is the highest priority
 */
#define ERROR_DEBUG_LEVEL           0   /* *ALWAYS* gets reported */
#define WARNING_DEBUG_LEVEL         1
#define INFORM_DEBUG_LEVEL          2
#define TRACE_DEBUG_LEVEL           3
#define NUM_DEBUG_LEVELS            (TRACE_DEBUG_LEVEL + 1)

/* like printf */
typedef int (*debug_reporting_function_pointer)(const char *format, ...);

#define MODULE_NAME_LENGTH          48
typedef struct module_debug_block_s {

    /* name of the module to print if needed */
    char module_name [MODULE_NAME_LENGTH];

    /* current allowed debug level for this module */    
    int level;

    /* function which reports the message */
    debug_reporting_function_pointer reporting_fn;

} module_debug_block_t;

extern const char **level_strings;
extern module_debug_block_t *module_debug_blocks;

/*
 * Generic debug/reporting call.
 * Note that ERROR_DEBUG_LEVEL will ALWAYS
 * be reported regardless, and it should be.
 */
#define REPORT(m, l, file, line, args...) \
    if (l <= module_debug_blocks[m].level) { \
        module_debug_block_t *mdb = &module_debug_blocks[m]; \
        mdb->reporting_fn("%s in %s (%s:%d): ", \
            level_strings[l], mdb->module_name, file, line); \
        mdb->reporting_fn(args); \
    }

#define ERROR(module, args...) \
    REPORT(module, ERROR_DEBUG_LEVEL, __FILE__, __LINE__, ## args)

#define WARNING(module, args...) \
    REPORT(module, WARNING_DEBUG_LEVEL, __FILE__, __LINE__, ## args)

#define INFORM(module, args...) \
    REPORT(module, INFORM_DEBUG_LEVEL, __FILE__, __LINE__, ## args)

#define TRACE(module, args...) \
    REPORT(module, TRACE_DEBUG_LEVEL, __FILE__, __LINE__, ## args)

/*****************************************************************************/

/* initialize the debug framework.  This MUST be called first */
extern int debug_init(int n_modules);

/* set a printable name for the specified module */
extern void set_module_name(int module, char *module_name);

/* set the reporting level for the specified module */
extern void set_module_debug_level(int module, int level);

/* set the 'reporting function' for the specified module */
extern void set_module_debug_reporting_function(int module,
    debug_reporting_function_pointer fptr);

#ifdef __cplusplus
} // extern C
#endif

#endif /* __DEBUG_H__ */


