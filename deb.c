
#include <stdio.h>

#define DEFINE_DEBUG_MODULE(module) \
    int module ## _debug_ ## integer

#define DEBUG_LEVEL		0x1
#define INFO_LEVEL		0x3
#define WARNING_LEVEL		0x7
#define ERROR_LEVEL		0xF

#define DEBUG_LEVEL_MASK	0x1
#define INFO_LEVEL_MASK		0x2
#define WARNING_LEVEL_MASK	0x4
#define ERROR_LEVEL_MASK	0x8

#define TURN_DEBUG_OFF(module) \
    module ## _debug_ ## integer = 0;

#define SET_DEBUG_LEVEL(module) \
    module ## _debug_ ## integer = DEBUG_LEVEL
#define CAN_REPORT_DEBUG(module) \
    (module ## _debug_ ## integer & DEBUG_LEVEL_MASK)

#define SET_INFO_LEVEL(module) \
    module ## _debug_ ## integer = INFO_LEVEL
#define CAN_REPORT_INFO(module) \
    (module ## _debug_ ## integer & INFO_LEVEL_MASK)

#define SET_WARNING_LEVEL(module) \
    module ## _debug_ ## integer = WARNING_LEVEL
#define CAN_REPORT_WARNING(module) \
    (module ## _debug_ ## integer & WARNING_LEVEL_MASK)

#define SET_ERROR_LEVEL(module) \
    module ## _debug_ ## integer = ERROR_LEVEL
#define CAN_REPORT_ERROR(module) \
    (module ## _debug_ ## integer & ERROR_LEVEL_MASK)

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

	SET_DEBUG_LEVEL(mmm);
	if (CAN_REPORT_DEBUG(mmm)) printf("correct\n");
	if (CAN_REPORT_INFO(mmm)) printf("INCORRECT\n");
	if (CAN_REPORT_WARNING(mmm)) printf("INCORRECT\n");
	if (CAN_REPORT_ERROR(mmm)) printf("INCORRECT\n");

	SET_INFO_LEVEL(mmm);
	if (CAN_REPORT_DEBUG(mmm)) printf("correct\n");
	if (CAN_REPORT_INFO(mmm)) printf("correct\n");
	if (CAN_REPORT_WARNING(mmm)) printf("INCORRECT\n");
	if (CAN_REPORT_ERROR(mmm)) printf("INCORRECT\n");

	SET_WARNING_LEVEL(mmm);
	if (CAN_REPORT_DEBUG(mmm)) printf("correct\n");
	if (CAN_REPORT_INFO(mmm)) printf("correct\n");
	if (CAN_REPORT_WARNING(mmm)) printf("correct\n");
	if (CAN_REPORT_ERROR(mmm)) printf("INCORRECT\n");

	SET_ERROR_LEVEL(mmm);
	if (CAN_REPORT_DEBUG(mmm)) printf("correct\n");
	if (CAN_REPORT_INFO(mmm)) printf("correct\n");
	if (CAN_REPORT_WARNING(mmm)) printf("correct\n");
	if (CAN_REPORT_ERROR(mmm)) printf("correct\n");
    }
    return 0;
}

