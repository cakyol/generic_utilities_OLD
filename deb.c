
#include <stdio.h>

/*
 * Declare a byte for module <module_name>.
 * This will contain the highest level of debug
 * messages that module is allowed to process.
 * The higher the bit, the higher the level of
 * messages it can report.
 */
#define DEFINE_DEBUG_MODULE(module_name) \
    char debug_variable_for_module_ ## module_name

#define DEBUG_LEVEL		(0b1)
#define INFO_LEVEL		(0b11)
#define WARNING_LEVEL		(0b111)
#define ERROR_LEVEL		(0b1111)

#define DEBUG_LEVEL_MASK	(0b1)
#define INFO_LEVEL_MASK		(0b10)
#define WARNING_LEVEL_MASK	(0b100)
#define ERROR_LEVEL_MASK	(0b1000)

/*
 * Turn off ALL debugging for module <module_name>
 */
#define TURN_DEBUG_OFF(module_name) \
    debug_variable_for_module_ ## module_name = 0;

/*
 * Set the level of debugging for module <module_name>
 * to DEBUG level (lowest level)
 */
#define SET_DEBUG_LEVEL_TO_DEBUG(module_name) \
    debug_variable_for_module_ ## module_name = DEBUG_LEVEL

/*
 * Set the level of debugging for module <module_name>
 * to INFO level (one level more than DEBUG)
 */
#define SET_DEBUG_LEVEL_TO_INFO(module_name) \
    debug_variable_for_module_ ## module_name = INFO_LEVEL

/*
 * Set the level of debugging for module <module_name>
 * to WARNING level (two levels more than DEBUG)
 */
#define SET_DEBUG_LEVEL_TO_WARNING(module_name) \
    debug_variable_for_module_ ## module_name = WARNING_LEVEL

/*
 * Set the level of debugging for module <module_name>
 * to ERROR level (highest level)
 */
#define SET_DEBUG_LEVEL_TO_ERROR(module_name) \
    debug_variable_for_module_ ## module_name = ERROR_LEVEL

#define CAN_REPORT_DEBUG(module_name) \
    (debug_variable_for_module_ ## module_name & DEBUG_LEVEL_MASK)
#define CAN_REPORT_INFO(module_name) \
    (debug_variable_for_module_ ## module_name & INFO_LEVEL_MASK)
#define CAN_REPORT_WARNING(module_name) \
    (debug_variable_for_module_ ## module_name & WARNING_LEVEL_MASK)
#define CAN_REPORT_ERROR(module_name) \
    (debug_variable_for_module_ ## module_name & ERROR_LEVEL_MASK)

DEFINE_DEBUG_MODULE(mmm);
DEFINE_DEBUG_MODULE(mmm);
DEFINE_DEBUG_MODULE(mmm);

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

