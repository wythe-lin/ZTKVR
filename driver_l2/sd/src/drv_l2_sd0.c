#include "drv_l2_sd.h"

extern INT32U sd_card_rca;
extern INT32S drvl2_sd_bcr_ac_command_set(INT32U command, INT32U argument, INT32U response_type, INT32U *response);
INT16S drvl2_sdc_live_response2(void)
{
#if 0
	INT32U response;

    if (drvl2_sd_bcr_ac_command_set(C_SD_CMD13, sd_card_rca, C_RESP_R1, &response) == 0) {
    	// Card exist
    	return 0;
    } else {
    	// Card not exist
    	return -1;
    }
#else
	return gpio_read_io();
#endif
}