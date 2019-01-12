
#include <stdio.h>

typedef unsigned char debug_flag_t;

#define DEBUG_LEVEL             (0b00000001)
#define DEBUG_LEVEL_MASK        (0b11111111)

#define INFORMATION_LEVEL       (0b00000010)
#define INFORMATION_LEVEL_MASK  (0b11111110)

#define WARNING_LEVEL           (0b00000100)
#define WARNING_LEVEL_MASK      (0b11111100)

#define ERROR_LEVEL             (0b00001000)
#define ERROR_LEVEL_MASK        (0b11111000)

#ifdef INCLUDE_ALL_DEBUGGING_CODE

#define ENABLE_DEBUG(module)         (module = DEBUG_LEVEL)
#define ENABLE_INFORMATION(module)   (module = INFORMATION_LEVEL)
#define ENABLE_WARNING(module)       (module = WARNING_LEVEL)
#define ENABLE_ERROR(module)         (module = ERROR_LEVEL)

#define DEBUG(module, fmt, args...) \
    do { \
        if (module & DEBUG_LEVEL_MASK) { \
            _process_debug_message_(module, DEBUG_LEVEL, \
                __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        } \
    } while (0)

#define INFORMATION(module, fmt, args...) \
    do { \
        if (module & INFORMATION_LEVEL_MASK) { \
            _process_debug_message_(module, INFORMATION_LEVEL, \
                __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        } \
    } while (0)

#define WARNING(module, fmt, args...) \
    do { \
        if (module & WARNING_LEVEL_MASK) { \
            _process_debug_message_(module, WARNING_LEVEL, \
                __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        } \
    } while (0)

/*
 * Should this always report regardless ?
 */
#define ERROR(module, fmt, args...) \
    do { \
        if (module & ERROR_LEVEL_MASK) { \
            _process_debug_message_(module, ERROR_LEVEL, \
                __FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
        } \
    } while (0)

#else /* ! INCLUDE_ALL_DEBUGGING_CODE */

    #define ENABLE_DEBUG(module)
    #define ENABLE_INFORMATION(module)
    #define ENABLE_WARNING(module)
    #define ENABLE_ERROR(module)

    #define DEBUG(module, fmt, args...)
    #define INFORMATION(module, fmt, args...)
    #define WARNING(module, fmt, args...)
    #define ERROR(module, fmt, args...) \

#endif /* ! INCLUDE_ALL_DEBUGGING_CODE */

int main (int argc, char *argv[])
{
    int i;

    for (i = 0; i < 5; i++) {

    TURN_DEBUG_OFF(mmm);
    if (CAN_REPORT_DEBUG(mmm)) printf("INCORRECT\n");
    if (CAN_REPORT_INFO(mmm)) printf("INCORRECT\n");
    if (CAN_REPORT_WARNING(mmm)) printf("INCORRECT\n");
    if (CAN_REPORT_ERROR(mmm)) printf("INCORRECT\n");

    SET_DEBUG_LEVEL_TO_DEBUG(mmm);
    if (CAN_REPORT_DEBUG(mmm)) printf("correct\n");
    if (CAN_REPORT_INFO(mmm)) printf("INCORRECT\n");
    if (CAN_REPORT_WARNING(mmm)) printf("INCORRECT\n");
    if (CAN_REPORT_ERROR(mmm)) printf("INCORRECT\n");

    SET_DEBUG_LEVEL_TO_INFO(mmm);
    if (CAN_REPORT_DEBUG(mmm)) printf("correct\n");
    if (CAN_REPORT_INFO(mmm)) printf("correct\n");
    if (CAN_REPORT_WARNING(mmm)) printf("INCORRECT\n");
    if (CAN_REPORT_ERROR(mmm)) printf("INCORRECT\n");

    SET_DEBUG_LEVEL_TO_WARNING(mmm);
    if (CAN_REPORT_DEBUG(mmm)) printf("correct\n");
    if (CAN_REPORT_INFO(mmm)) printf("correct\n");
    if (CAN_REPORT_WARNING(mmm)) printf("correct\n");
    if (CAN_REPORT_ERROR(mmm)) printf("INCORRECT\n");

    SET_DEBUG_LEVEL_TO_ERROR(mmm);
    if (CAN_REPORT_DEBUG(mmm)) printf("correct\n");
    if (CAN_REPORT_INFO(mmm)) printf("correct\n");
    if (CAN_REPORT_WARNING(mmm)) printf("correct\n");
    if (CAN_REPORT_ERROR(mmm)) printf("correct\n");
    }
    return 0;
}

