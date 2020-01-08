
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

#include "debug.h"

int _n_modules = 0;
const char **level_strings = 0;
module_debug_block_t *module_debug_blocks = 0;

static inline int
invalid_module_number (int m)
{
    return ((m < 0) || (m >= _n_modules));
}

static inline int
invalid_debug_level (int lev)
{
    return ((lev < MIN_DEBUG_LEVEL) || (lev > MAX_DEBUG_LEVEL));
}

int
set_module_name (int module, char *module_name)
{
    char *name;

    if (invalid_module_number(module))
        return EINVAL;
    if (module_name) {
        name = module_name;
    } else {
        char default_module_name [MODULE_NAME_LENGTH];
        sprintf(default_module_name, "module_%d", module);
        name = &default_module_name[0];
    }
    strncpy(&module_debug_blocks[module].module_name[0],
        name, MODULE_NAME_LENGTH);
    module_debug_blocks[module].module_name[MODULE_NAME_LENGTH - 1] = 0;
    return 0;
}

int
set_module_debug_level (int module, int level)
{
    if (invalid_module_number(module))
        return EINVAL;
    if (invalid_debug_level(level))
        return EINVAL;
    if (level < 0) level = ERROR_LEVEL;
    module_debug_blocks[module].level = level;
    return 0;
}

int
set_module_debug_reporting_function (int module,
    debug_reporting_function_pointer fptr)
{
    if (invalid_module_number(module))
        return EINVAL;
    if (0 == fptr)
        fptr = printf;
    module_debug_blocks[module].reporting_fn = fptr;
    return 0;
}

/*****************************************************************************/

extern int
debug_init (int n_modules)
{
    int m;

    if (n_modules <= 0)
        return EINVAL;

    level_strings = malloc(NUM_DEBUG_LEVELS * sizeof(char*));
    if (0 == level_strings)
        return ENOMEM;

    module_debug_blocks = malloc(n_modules * sizeof(module_debug_block_t));
    if (0 == module_debug_blocks) {
        free(level_strings);
        level_strings = 0;
        return ENOMEM;
    }
    
    /* set this now, things succeeded */
    _n_modules = n_modules;

    level_strings[ERROR_LEVEL] = "ERROR";
    level_strings[WARNING_LEVEL] = " WARNING";
    level_strings[INFORM_LEVEL] = "  INFORMATION";
    level_strings[TRACE_LEVEL] = "   TRACE";

    for (m = 0; m < n_modules; m++) {

        /* set to default module name */
        set_module_name(m, 0);

        /* set to error level */
        module_debug_blocks[m].level = ERROR_LEVEL;

        /* set to default printing function */
        set_module_debug_reporting_function(m, 0);
    }

    return 0;
}

