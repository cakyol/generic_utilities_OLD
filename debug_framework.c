
#define MAX_MODULES             256

#define MIN_DEBUG_LEVEL         0
#define DEBUG                   (MIN_DEBUG_LEVEL)
#define INFO                    (DEBUG + 1)
#define WARN                    (INFO + 1)
#define ERROR                   (WARN + 1)
#define FATAL                   (ERROR + 1)
#define MAX_DEBUG_LEVEL         FATAL

/*
 * by default, all errors and higher levels are enabled
 */
static module_debug_levels [MAX_MODULES] = { ERROR };

int set_module_debug_level (int module, int level)
{
    /* check module validity */
    if ((module < 0) || (module >= MAX_MODULES)) return EINVAL;

    /* trim debug level to limits */
    if (level < MIN_DEBUG_LEVEL) 
        level = MIN_DEBUG_LEVEL;
    else if (level > MAX_DEBUG_LEVEL) 
        level = MAX_DEBUG_LEVEL;

    /* set it */
    module_debug_levels[module] = level;

    /* done */
    return 0;
}

int
print_debug_message (int module, int level,
    char *filename, int line_number,
    char *fmt, ...)
{

#define STATIC_MESSAGE_BUFFER_SIZE      512

    va_list args;
    char msg_buffer [STATIC_MESSAGE_BUFFER_SIZE];
    int index, size_left, len;

    /* invalid module */
    if ((module < 0) || (module >= MAX_MODULES)) return EINVAL;

    /* not allowed, level threshold is higher than requested level */
    if (level < module_debug_levels[module]) return EPERM;

    size_left = STATIC_MESSAGE_BUFFER_SIZE;
    index = 0;

    /* write module number in case grep on module is needed */
    len = vsnprintf(&msg_buffer[index], size_left,
                "M: %d, F: %s, L: %d >> ", module, filename, line_number);
    size_left -= len;
    index += len;

    va_start(args, fmt);
    vsnprintf(&msg_buffer[index], size_left, fmt, args);
    va_end(args);

    /* print here */

    /* done */
    return 0;
}




