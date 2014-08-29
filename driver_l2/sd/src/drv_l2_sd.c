/*
* Purpose: Driver layer2 for controlling SD/MMC cards
*
* Author: Tristan
*
* Date: 2008/12/11
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.06
*/

#include "drv_l2_sd.h"

//#if (defined _DRV_L2_SD) && (_DRV_L2_SD == 1)

#if ((SD_POS == SDC_IOC4_IOC5_IOC6_IOC7_IOC8_IOC9)||(SD_DUAL_SUPPORT==1))
	#define SD_PIN_IDX	0
#else
	#define SD_PIN_IDX	1
#endif

#define SD_DEVICE_NUM	0

#define C_SD_L2_INIT_RETRY							2

// Timeout value(unit is ms). Maximum timeout value for card initialization is 1 second
#define C_SD_L2_CARD_INIT_TIMEOUT					400		// Some cards need 360 ms to response to ACMD41
#define C_SD_L2_CARD_INIT_RETRY_TIMEOUT				1000	// Some cards need 900 ms to initialize after receiving CMD0
#define C_SD_L2_CARD_PROGRAM_TIMEOUT				750		// 750 ms timeout value for writing data to SD card

// Timeout value(unit is 2.67us)
#define C_SD_L2_CARD_BUSY_TIMEOUT					2500
#define C_SD_L2_SINGLE_READ_CMD_COMPLETE_TIMEOUT	560
#define C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT		4300
#define C_SD_L2_SINGLE_WRITE_CMD_COMPLETE_TIMEOUT	100
#define C_SD_L2_MULTI_WRITE_CMD_COMPLETE_TIMEOUT	100
#define C_SD_L2_SINGLE_WRITE_DATA_COMPLETE_TIMEOUT	100
#define C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT	100
#define C_SD_L2_ADTC_DATA_FULL_TIMEOUT				3400
#define C_SD_L2_RESP_R136_FULL_TIMEOUT				2000
#define C_SD_L2_RESP_R2_FULL_TIMEOUT				180
#define C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT		3600
#define C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT		100

#ifndef __CS_COMPILER__
const INT8U DRVL2_SD[] = "GLB_GP-S2_0610L_SD-L2-ADS_2.0.10";
#else
const INT8U DRVL2_SD[] = "GLB_GP-S2_0610L_SD-L2-CS_2.0.10";
#endif


static SD_CARD_STATE_ENUM sd_card_state = SD_CARD_STATE_INACTIVE;
static INT32U sd_card_rca;			// 16 bits (31-16)
static INT32U sd_card_csd[4];		// 128 bits
static INT32U sd_card_scr[2];		// 64 bits
static INT32U sd_card_cid[4];		// 128 bits
static INT32U sd_card_ocr;			// 32 bits
static INT32U sd_card_total_sector;
static INT32U sd_card_speed;
static INT8U sd_card_type;
static INT8U sd_card_bus_width;
static INT8U sd_card_protect;

static INT32S drvl2_sd_bc_command_set(INT32U command, INT32U argument);
static INT32S drvl2_sd_bcr_ac_command_set(INT32U command, INT32U argument, INT32U response_type, INT32U *response);
static INT32S drvl2_sd_adtc_command_set(INT32U command, INT32U rca, INT32U count, INT32U *response);
static INT32S drvl2_v1_sd_card_initiate(INT32U timeout);
static INT32S drvl2_v2_sd_card_initiate(INT32U timeout);
static INT32S drvl2_mmc_card_initiate(INT32U timeout);
static INT32S drvl2_sdc_card_busy_wait(INT32U timeout);

extern INT32S SD_OS_Init(void) ;
extern void SD_IO_Request(INT32U idx) ;
extern void SD_IO_Release(void) ;

INT32S drvl2_sd_init(void)
{
	INT32S ret;
	INT32U response;
	INT32U count;
	INT32U timeout;

	//SD_OS_Init();
	SD_IO_Request(SD_PIN_IDX);
	if (sd_card_state == SD_CARD_STATE_TRANSFER) {
		drvl1_sdc_enable(SD_DEVICE_NUM);

		// If card exists and in transfer state, then just return
		if (drvl2_sd_bcr_ac_command_set(C_SD_CMD13, sd_card_rca, C_RESP_R1, &response) == 0) {
			if ((response & 0x1E00) == 0x800) {		// 0x800 is transfer state
				SD_IO_Release();
				return 0;
			}
		}
	}

	sd_card_rca = 0x0;
	sd_card_type = C_MEDIA_STANDARD_SD_CARD;
	sd_card_total_sector = 0;
	sd_card_speed = 400000;				// 400K Hz
	sd_card_bus_width = 1;
	sd_card_protect = 0;
	sd_card_state = SD_CARD_STATE_IDLE;
	
	drvl1_sdc_init(SD_DEVICE_NUM);
	drvl1_sdc_enable(SD_DEVICE_NUM);
	
	// Send 74 clock and then issue comand 0
	for (count=0; count<C_SD_L2_INIT_RETRY; count++) {
		drvl1_sdc_card_init_74_cycles(SD_DEVICE_NUM);
		drvl2_sd_bc_command_set(C_SD_CMD0, 0x0);
		drvl1_sdc_card_init_74_cycles(SD_DEVICE_NUM);
		drvl2_sd_bc_command_set(C_SD_CMD0, 0x0);
		drvl1_sdc_card_init_74_cycles(SD_DEVICE_NUM);

		if (count == 0) {
			timeout = C_SD_L2_CARD_INIT_TIMEOUT;
		} else {
			timeout = C_SD_L2_CARD_INIT_RETRY_TIMEOUT;
		}
		// Issue command 8
		ret = drvl2_sd_bcr_ac_command_set(C_SD_CMD8, 0x000001AA, C_RESP_R7, &response);
		// If the card responses to command 8
		if (ret == 0) {			// Ver 2.0 standard SD card or SDHC card
			if (!(response & 0x100)) {
				// If response is not valid, it is an unusable card
				sd_card_state = SD_CARD_STATE_INACTIVE;
				SD_IO_Release();
				return -1;
			}
			if (drvl2_v2_sd_card_initiate(timeout) == 0) {
				break;
			}

		} else {	// If the card doesn't response to command 8, it is a ver 1.x SD card or MMC
			if (drvl2_v1_sd_card_initiate(timeout)==0) {
				break;
			}
			if (sd_card_type == C_MEDIA_MMC_CARD) {
				if (drvl2_mmc_card_initiate(timeout) == 0) {
					break;
				}
			}
			// if card type is SDHC but fail to response to CMD8, we have to run the initialization process again
		}
	  #if _OPERATING_SYSTEM != _OS_NONE
	  	OSTimeDly(1);
	  #endif
	}
	if (count == C_SD_L2_INIT_RETRY) {
		SD_IO_Release();
		return -1;
	}
	sd_card_state = SD_CARD_STATE_READY;

	// Send command 2
	if (drvl2_sd_bcr_ac_command_set(C_SD_CMD2, 0x0, C_RESP_R2, &sd_card_cid[0])) {
		SD_IO_Release();
		return -1;
	}
	sd_card_state = SD_CARD_STATE_IDENTIFICATION;

	// Send command 3
	if ((sd_card_type != C_MEDIA_MMC_CARD)&&(sd_card_type != C_MEDIA_MMCHC_CARD)) {
		// Send CMD3 and read new RCA, SD will generate RCA itself
		if (drvl2_sd_bcr_ac_command_set(C_SD_CMD3, 0x0, C_RESP_R6, &response)) {
			SD_IO_Release();
			return -1;
		}
		sd_card_rca = response & 0xFFFF0000;
	} else {
		// Send CMD3 to set a new RCA to MMC card
		if (drvl2_sd_bcr_ac_command_set(C_SD_CMD3, 0xFFFF0000, C_RESP_R6, &response)) {
			SD_IO_Release();
			return -1;
		}
		sd_card_rca = 0xFFFF0000;
	}
	sd_card_state = SD_CARD_STATE_STANDBY;
	// Set bus width and clock speed
	ret = drvl2_sd_bus_clock_set(50000000);
	if(ret < 0) {
		ret = drvl2_sd_bus_clock_set(25000000);
	}
	
	SD_IO_Release();
	return ret;
}

INT16S drvl2_sdc_live_response0(void)
{
	INT32U response;

    if (drvl2_sd_bcr_ac_command_set(C_SD_CMD13, sd_card_rca, C_RESP_R1, &response) == 0) {
    	// Card exist
    	return 0;
    } else {
    	// Card not exist
    	return -1;
    }
}

static void drvl2_sd_set_speed_bus(void)
{
	drvl1_sdc_clock_set(SD_DEVICE_NUM, sd_card_speed);
	drvl1_sdc_bus_width_set(SD_DEVICE_NUM, sd_card_bus_width);			// Set bus width to 4 bits
}

static INT32S drvl2_sd_bc_command_set(INT32U command, INT32U argument)
{
	return drvl1_sdc_command_send(SD_DEVICE_NUM, command, argument);
}

static INT32S drvl2_sd_bcr_ac_command_set(INT32U command, INT32U argument, INT32U response_type, INT32U *response)
{
	INT8U i;

	if (drvl1_sdc_command_send(SD_DEVICE_NUM, command, argument)) {
		return -1;
	}

	switch (response_type) {
	case C_RESP_R1:
	case C_RESP_R1B:
	case C_RESP_R3:
	case C_RESP_R6:
		return drvl1_sdc_response_get(SD_DEVICE_NUM, response, C_SD_L2_RESP_R136_FULL_TIMEOUT);

	case C_RESP_R2:
		for (i=0; i<4; i++) {
			if (drvl1_sdc_response_get(SD_DEVICE_NUM, response, C_SD_L2_RESP_R2_FULL_TIMEOUT)) {
				return -1;
			}
			response++;
		}

	case C_RESP_R0:
	default:
		break;
	}

	return 0;
}

static INT32S drvl2_sd_adtc_command_set(INT32U command, INT32U rca, INT32U count, INT32U *response)
{
	// Clear SD RX data register before read command is issued
	drvl1_sdc_clear_rx_data_register(SD_DEVICE_NUM);

	if (drvl1_sdc_command_send(SD_DEVICE_NUM, command, rca)) {
		return -1;
	}

	while (count) {
		if (drvl1_sdc_data_get(SD_DEVICE_NUM, response, C_SD_L2_ADTC_DATA_FULL_TIMEOUT)) {
			return -1;
		}
		response++;
		count--;
	}

	return 0;
}

static INT32S drvl2_v1_sd_card_initiate(INT32U timeout)
{
	INT32S ret;
	INT32U response;
	INT32U timer;

  #if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	timer = (INT32U) sw_timer_get_counter_L();
  #elif _OPERATING_SYSTEM != _OS_NONE
  	timer = OSTimeGet();		// 10ms per tick
  #else
  	timer = 0;
  #endif
	do {
		// Send ACMD41 with HCS=0 until card is ready or timeout
		if (drvl2_sd_bcr_ac_command_set(C_SD_CMD55, 0x0, C_RESP_R1, &response)) {
			// If CMD55 timeout occurs, maybe it is a MMC card
			sd_card_type = C_MEDIA_MMC_CARD;

			return -1;
		}
		ret = drvl2_sd_bcr_ac_command_set(C_SD_ACMD41, 0x00200000, C_RESP_R3, &sd_card_ocr);
		if (ret) {
		  #if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
			if ((sw_timer_get_counter_L()-timer) > timeout) {
		  #elif _OPERATING_SYSTEM != _OS_NONE
		  	if ((OSTimeGet()-timer)*10 > timeout) {
		  #else
		  	if (++timer > timeout) {
		  #endif
		    	// If card timeout occurs, maybe it is a MMC card
		    	sd_card_type = C_MEDIA_MMC_CARD;

		    	return -1;
		    }

	    } else if (!(sd_card_ocr & 0x80000000)) {
		  #if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
			if ((sw_timer_get_counter_L()-timer) > timeout) {
		  #elif _OPERATING_SYSTEM != _OS_NONE
		  	if ((OSTimeGet()-timer)*10 > timeout)) {
		  #else
		  	if (++timer > timeout) {
		  #endif
		    	// If card busy timeout occurs, maybe it is a SDHC card
		    	sd_card_type = C_MEDIA_SDHC_CARD;

		    	return -1;
		    }

		} else {		// It is a Ver1.X standard SD card
			sd_card_type = C_MEDIA_STANDARD_SD_CARD;
			break;
		}
	} while (1) ;

	return 0;
}

static INT32S drvl2_v2_sd_card_initiate(INT32U timeout)
{
	INT32S ret;
	INT32U response;
	INT32U timer;

  #if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	timer = (INT32U) sw_timer_get_counter_L();
  #elif _OPERATING_SYSTEM != _OS_NONE
  	timer = OSTimeGet();		// 10ms per tick
  #else
  	timer = 0;
  #endif
	do {
		// Send ACMD41 with HCS=1 until card is ready or timeout
		if (drvl2_sd_bcr_ac_command_set(C_SD_CMD55, 0x0, C_RESP_R1, &response)) {
			return -1;
		}
		ret = drvl2_sd_bcr_ac_command_set(C_SD_ACMD41, 0x40200000, C_RESP_R3, &sd_card_ocr);
		if (ret || !(sd_card_ocr & 0x80000000)) {
			// Maximum timeout value for card initialization is 1 second
		  #if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
			if ((sw_timer_get_counter_L()-timer) > timeout) {
		  #elif _OPERATING_SYSTEM != _OS_NONE
		  	if ((OSTimeGet()-timer)*10 > timeout)) {
		  #else
		  	if (++timer > timeout) {
		  #endif
		    	// If card timeout occurs, it is an unusable card
		    	return -1;
		    }
		} else {
			if (sd_card_ocr & 0x40000000) {	// If CCS in sd_card_ocr is 1, it is a Ver2.0 SDHC card
				sd_card_type = C_MEDIA_SDHC_CARD;
			} else {		// If CCS in sd_card_ocr is 0, it is a Ver2.0 standard SD card
				sd_card_type = C_MEDIA_STANDARD_SD_CARD;
			}
			break;
		}
	} while (1) ;

	return 0;
}

static INT32S drvl2_mmc_card_initiate(INT32U timeout)
{
	INT32S ret;
	INT32U response;
	INT32U timer;

	// Start MMC initialization process
  #if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	timer = (INT32U) sw_timer_get_counter_L();
  #elif _OPERATING_SYSTEM != _OS_NONE
  	timer = OSTimeGet();		// 10ms per tick
  #else
  	timer = 0;
  #endif
	do {
		// Send CMD1 with voltage range set until card is ready or timeout
		ret = drvl2_sd_bcr_ac_command_set(C_SD_CMD1, 0x40FF8000, C_RESP_R3, &response);
		if (ret || !(response & 0x80000000)) {
		  #if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
			if ((sw_timer_get_counter_L()-timer) > timeout) {
		  #elif _OPERATING_SYSTEM != _OS_NONE
		  	if ((OSTimeGet()-timer)*10 > timeout) {
		  #else
		  	if (++timer > timeout) {
		  #endif
		    	// If card timeout occurs, it is an unusable card
		    	sd_card_state = SD_CARD_STATE_INACTIVE;

		    	return -1;
		    }

		} else {
			// Check whether voltage is acceptable
			if (!(response & 0x00FF8000)) {
				// If response is not valid, it is an unusable card
				sd_card_state = SD_CARD_STATE_INACTIVE;

				return -1;
			}
			sd_card_type = C_MEDIA_MMC_CARD;
			break;
		}
	} while (1) ;
	// MMC HC card
	if(response&0x40000000)	
		sd_card_type = C_MEDIA_MMCHC_CARD;
	return 0;
}

const INT8U speed_table[16] = {0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};
const INT32U tran_table[4] = {10000, 100000, 1000000, 10000000};

static INT32U drvl2_sd_despeed(void)
{
	INT32U time_value = (sd_card_csd[0] & 0x00000078) >> 3;
	INT32U tran_unit = sd_card_csd[0] & 0x00000007;
	
	if( ( time_value >= sizeof(speed_table) ) || ( tran_unit >= sizeof (tran_table) ) )
		return 0;
	else
		return speed_table[time_value] * tran_table[tran_unit] ;
}

static INT32S drvl2_sd_bus_clock_set_sdcard(INT32U limit_speed)
{
	INT32U response;
	INT32U c_size, mult, block_len;
	INT32U max_speed;
	INT32U switch_function_data[16];		// 512 bits
	
	if (drvl2_sd_bcr_ac_command_set(C_SD_CMD7, sd_card_rca, C_RESP_R1B, &response)) {
		return -1;
	}
	// Send ACMD51 to read 64-bits SD configuration register(SCR)	
	if (drvl1_sdc_block_len_set(SD_DEVICE_NUM, 8)) {
		return -1;
	}
    // Send ACMD51 to read 64-bits SD configuration register(SCR)
    if (drvl2_sd_bcr_ac_command_set(C_SD_CMD55, sd_card_rca, C_RESP_R1, &response) ||
    	drvl2_sd_adtc_command_set(C_SD_ACMD51, sd_card_rca, 2, &sd_card_scr[0])) {
		return -1;
	}
	// Reset block length to C_SD_SECTOR_SIZE bytes
	if (drvl1_sdc_block_len_set(SD_DEVICE_NUM, C_SD_SECTOR_SIZE)) {
		return -1;
	}
	// Check limit clock and SD spec version 
	if( (limit_speed>25000000) && (sd_card_scr[0]&0x0F)>1 )
	{
		// Set block length to 64 bytes so that we can read status data of CMD6
		drvl1_sdc_block_len_set(SD_DEVICE_NUM, 64);

		// Send ACMD6 to set bus width to 1 bit mode
		if (sd_card_bus_width == 4) {
			drvl2_sd_bcr_ac_command_set(C_SD_CMD55, sd_card_rca, C_RESP_R1, &response);
			drvl2_sd_bcr_ac_command_set(C_SD_ACMD6, 0x0, C_RESP_R1, &response);
			drvl1_sdc_bus_width_set(SD_DEVICE_NUM, 1);
		}

		if (drvl2_sd_adtc_command_set(C_SD_CMD6, 0x00FFFF01, 16, &switch_function_data[0]) == 0) {
			// Check whether card supports high-speed mode
			if ((switch_function_data[3] & 0x00000300) == 0x00000300) {
				drvl2_sd_adtc_command_set(C_SD_CMD6, 0x80FFFF01, 16, &switch_function_data[0]);
			}
		}
		// Restore sector length
		drvl1_sdc_block_len_set(SD_DEVICE_NUM, C_SD_SECTOR_SIZE);
	}
	if (sd_card_scr[0] & 0x00000400) {		// Check whether this SD card supports 4-bits bus width
		sd_card_bus_width = 4;

		// Send ACMD6 to set the card to 4-bit bus width
		if (drvl2_sd_bcr_ac_command_set(C_SD_CMD55, sd_card_rca, C_RESP_R1, &response) ||
			drvl2_sd_bcr_ac_command_set(C_SD_ACMD6, 0x2, C_RESP_R1, &response)) {
			return -1;
		}

		drvl1_sdc_bus_width_set(SD_DEVICE_NUM, 4);			// Set bus width to 4 bits
	}
	// Switch back to standby state
	drvl2_sd_bcr_ac_command_set(C_SD_CMD7, 0x0, C_RESP_R0, &response);
	// Send CMD9 and read CSD Register
	if (drvl2_sd_bcr_ac_command_set(C_SD_CMD9, sd_card_rca, C_RESP_R2, &sd_card_csd[0])) {
		return -1;
	}

	// Calculate Totoal Memory Size
	if (sd_card_type == C_MEDIA_SDHC_CARD) {
		c_size = ((sd_card_csd[2]>>16) & 0x0000FFFF)+((sd_card_csd[1]&0x0000003F)<<16);
		sd_card_total_sector = (c_size+1) << 10;
	} else {
		block_len = (sd_card_csd[1] >> 16) & 0xF;
		c_size = ((sd_card_csd[1] & 0x3FF) << 2) | ((sd_card_csd[2] >> 30) & 0x3);
		mult = (sd_card_csd[2] >> 15) & 0x7;
		sd_card_total_sector = (c_size + 1) << (mult + 2 + block_len - 9);
	}
	
	max_speed = drvl2_sd_despeed();
	if( max_speed == 0 )
		return -1;
	
  	if (max_speed > limit_speed) {
  		max_speed = limit_speed;
  	}
	sd_card_speed = max_speed;
	if (drvl1_sdc_clock_set(SD_DEVICE_NUM, max_speed)) {
		return -1;
	}

	// Change to transfer state
	if (drvl2_sd_bcr_ac_command_set(C_SD_CMD7, sd_card_rca, C_RESP_R1B, &response)) {
		// sdc speed back to 400k
		drvl1_sdc_clock_set(SD_DEVICE_NUM, 400000);
		return -1;
	}
	sd_card_state = SD_CARD_STATE_TRANSFER;
	return 0;
}

static INT32S drvl2_sd_bus_clock_set_mmc(INT32U limit_speed)
{
	INT32U response;
	INT32U c_size, mult, block_len;
	INT32U max_speed;
	INT32U SPEC_VAR;
	
	// Send CMD9 and read CSD Register
	if (drvl2_sd_bcr_ac_command_set(C_SD_CMD9, sd_card_rca, C_RESP_R2, &sd_card_csd[0])) {
		return -1;
	}
	
	SPEC_VAR = (sd_card_csd[0]&0x3C000000)>>26;
	max_speed = drvl2_sd_despeed();
	if( max_speed == 0 )
		return -1;
	
	// Change to transfer state
	if (drvl2_sd_bcr_ac_command_set(C_SD_CMD7, sd_card_rca, C_RESP_R1B, &response)) {
		// sdc speed back to 400k
		drvl1_sdc_clock_set(SD_DEVICE_NUM, 400000);
		return -1;
	}
	sd_card_state = SD_CARD_STATE_TRANSFER;
	// ----- Only spec. 4.0 or latter can support SWITCH function -----
	if( SPEC_VAR >= 4 )
	{
		//----- Setup MMC bus width 4 bit -----
		if(drvl2_sd_bcr_ac_command_set(C_MMC_CMD6, 0x03b70100, C_RESP_R1B, &response)){
			return -1;
		}
		if((response&0x08)==0)
		{
			sd_card_bus_width = 4;
			drvl1_sdc_bus_width_set(SD_DEVICE_NUM, 4);			// Set bus width to 4 bits
	    } 
	    //----- Setup MMC to high speed mode -----
	    if( limit_speed>25000000 )
	    {
			if( drvl2_sd_bcr_ac_command_set(C_MMC_CMD6, 0x03b90100, C_RESP_R1B, &response)){
				return -1;
			}
			if((response&0x08)==0)
			{
				max_speed = 52000000;
		    }
		}
	}
	
	if (max_speed > limit_speed) {
  		max_speed = limit_speed;
  	}
	
	sd_card_speed = max_speed;
	if (drvl1_sdc_clock_set(SD_DEVICE_NUM, max_speed)) {
		return -1;
	}
	
	if(sd_card_type == C_MEDIA_MMCHC_CARD)
	{
		INT32U buf[128]={0};

		if (drvl2_sd_adtc_command_set(C_SD_CMD8|C_SDC_CMD_WITH_DATA, 0, 128, buf)){	
			return -1;
		}
		sd_card_total_sector = buf[53];
	}
	else {
		block_len = (sd_card_csd[1] >> 16) & 0xF;
		c_size = ((sd_card_csd[1] & 0x3FF) << 2) | ((sd_card_csd[2] >> 30) & 0x3);
		mult = (sd_card_csd[2] >> 15) & 0x7;
		sd_card_total_sector = (c_size + 1) << (mult + 2 + block_len - 9);
	}
	return 0;
}


INT32S drvl2_sd_bus_clock_set(INT32U limit_speed)
{
	INT32U response;
	// Make sure that card is ready and in standby state
	if (drvl2_sd_bcr_ac_command_set(C_SD_CMD13, sd_card_rca, C_RESP_R1, &response)) {
		return -1;
	}
	response &= 0x1E00;
	
	if (response!=0x0600 && response!=0x0800) {		// 0x0600 is standby state, 0x800 is transfer state
		return -1;
	}
	
	if (response ==0x0800 ) {
		// Switch from transfer state to standby state
		drvl2_sd_bcr_ac_command_set(C_SD_CMD7, 0x0, C_RESP_R0, &response);
	}
	
	if (sd_card_type==C_MEDIA_SDHC_CARD || sd_card_type==C_MEDIA_STANDARD_SD_CARD)
		return 	drvl2_sd_bus_clock_set_sdcard(limit_speed);
	else
		return drvl2_sd_bus_clock_set_mmc(limit_speed);
}

void drvl2_sdc_card_info_get(SD_CARD_INFO_STRUCT *sd_info)
{
	INT8U i;

	sd_info->rca = (sd_card_rca >> 16) & 0xFFFF;
	for (i=0; i<4; i++) {
		sd_info->csd[i] = sd_card_csd[i];
	}
	for (i=0; i<2; i++) {
		sd_info->scr[i] = sd_card_scr[i];
	}
	for (i=0; i<4; i++) {
		sd_info->cid[i] = sd_card_cid[i];
	}
	sd_info->ocr = sd_card_ocr;
}

INT8U drvl2_sdc_card_protect_get(void)
{
	return sd_card_protect;
}

void drvl2_sdc_card_protect_set(INT8U value)
{
	sd_card_protect = value;
}

INT32U drvl2_sd_sector_number_get(void)
{
	return sd_card_total_sector;
}

static INT32S drvl2_sdc_card_busy_wait(INT32U timeout)
{
	INT32U timer;

	// Wait operation complete and back to transfer state
  #if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
	timer = (INT32U) sw_timer_get_counter_L();
  #elif _OPERATING_SYSTEM != _OS_NONE
  	timer = OSTimeGet();
  #else
  	timer = 0;
  #endif
	do {
		drvl1_sdc_card_init_74_cycles(SD_DEVICE_NUM);
		if (drvl1_sdc_card_busy_wait(SD_DEVICE_NUM, 0)) {
		  #if (defined _DRV_L1_TIMER) && (_DRV_L1_TIMER == 1)
			if ((sw_timer_get_counter_L()-timer) > (timeout)) {
		  #elif _OPERATING_SYSTEM != _OS_NONE
		  	if ((OSTimeGet()-timer) > (timeout/10)) {
		  #else
		  	if (++timer > timeout) {
		  #endif
				return -1;
		    }
		} else {
			break;
		}
	} while (1) ;

	return 0;
}

INT32S drvl2_sd_read(INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
	INT32U response;
	INT32S ret;

	if (sd_card_state != SD_CARD_STATE_TRANSFER) {
		return -1;
	}
	SD_IO_Request(SD_PIN_IDX);	
	drvl2_sd_set_speed_bus();	
	if (drvl2_sdc_card_busy_wait(C_SD_L2_CARD_BUSY_TIMEOUT)) {
		SD_IO_Release();
		return -1;
	}
#if 0
	if (drvl2_sd_bcr_ac_command_set(C_SD_CMD13, sd_card_rca, C_RESP_R1, &response)==0) {
		// Check whether card is in transfer state
		if (!(response & 0x100)) {
			SD_IO_Release();
			return -1;
		}
	}
#endif

	// Clear SD RX data register before read command is issued
	drvl1_sdc_clear_rx_data_register(SD_DEVICE_NUM);

	if (sector_count == 1) {
		if ((sd_card_type == C_MEDIA_SDHC_CARD)||(sd_card_type == C_MEDIA_MMCHC_CARD)) {
			if (drvl2_sd_adtc_command_set(C_SD_CMD17, start_sector, 0, &response)) {
				SD_IO_Release();
				return -1;
			}
		} else {
			if (drvl2_sd_adtc_command_set(C_SD_CMD17, start_sector<<9, 0, &response)) {
				SD_IO_Release();
				return -1;
			}
		}
		sd_card_state = SD_CARD_STATE_SENDING_DATA;

		ret = drvl1_sdc_read_data_by_dma(SD_DEVICE_NUM, buffer, sector_count, NULL);

		if (drvl1_sdc_data_crc_status_get(SD_DEVICE_NUM)) {
			ret = -1;
		}
		if (drvl1_sdc_command_complete_wait(SD_DEVICE_NUM, C_SD_L2_SINGLE_READ_CMD_COMPLETE_TIMEOUT)) {
			ret = -1;
		}

	} else {
		if ((sd_card_type == C_MEDIA_SDHC_CARD)||(sd_card_type == C_MEDIA_MMCHC_CARD)) {
			if (drvl2_sd_adtc_command_set(C_SD_CMD18, start_sector, 0, &response)) {
				SD_IO_Release();
				return -1;
			}
		} else {
			if (drvl2_sd_adtc_command_set(C_SD_CMD18, start_sector<<9, 0, &response)) {
				SD_IO_Release();
				return -1;
			}
		}
		sd_card_state = SD_CARD_STATE_SENDING_DATA;

		ret = drvl1_sdc_read_data_by_dma(SD_DEVICE_NUM, buffer, sector_count, NULL);

		if (drvl1_sdc_data_crc_status_get(SD_DEVICE_NUM)) {
			ret = -1;
		}
		if (drvl1_sdc_command_complete_wait(SD_DEVICE_NUM, C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT)) {
			ret = -1;
		}

		if (drvl1_sdc_stop_controller(SD_DEVICE_NUM, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT)) {
			ret = -1;
		}

		drvl2_sd_bcr_ac_command_set(C_SD_CMD12, 0x0, C_RESP_R1B, &response);
	}
	sd_card_state = SD_CARD_STATE_TRANSFER;
	SD_IO_Release();
	return ret;
}

INT32S drvl2_sd_write(INT32U start_sector, INT32U *buffer, INT32U sector_count)
{
	INT32U response;
	INT32S ret;

	if (sd_card_state != SD_CARD_STATE_TRANSFER) {
		return -1;
	}
	SD_IO_Request(SD_PIN_IDX);
	drvl2_sd_set_speed_bus();
	if (drvl2_sdc_card_protect_get()) {
		SD_IO_Release();
		return -1;
	}
	if (drvl2_sdc_card_busy_wait(C_SD_L2_CARD_BUSY_TIMEOUT)) {
		SD_IO_Release();
		return -1;
	}

	if (sector_count == 1) {
		if ((sd_card_type == C_MEDIA_SDHC_CARD)||(sd_card_type == C_MEDIA_MMCHC_CARD)) {
			if (drvl2_sd_adtc_command_set(C_SD_CMD24, start_sector, 0, &response)) {
				SD_IO_Release();
				return -1;
			}
		} else {
			if (drvl2_sd_adtc_command_set(C_SD_CMD24, start_sector<<9, 0, &response)) {
				SD_IO_Release();
				return -1;
			}
		}
		if (drvl1_sdc_command_complete_wait(SD_DEVICE_NUM, C_SD_L2_SINGLE_WRITE_CMD_COMPLETE_TIMEOUT)) {
			SD_IO_Release();
			return -1;
		}
		sd_card_state = SD_CARD_STATE_RECEIVE_DATA;

		ret = drvl1_sdc_write_data_by_dma(SD_DEVICE_NUM, buffer, sector_count, NULL);

		sd_card_state = SD_CARD_STATE_PROGRAMMING;
		if (drvl1_sdc_data_crc_status_get(SD_DEVICE_NUM)) {
			ret = -1;
		}
		if (drvl1_sdc_data_complete_wait(SD_DEVICE_NUM, C_SD_L2_SINGLE_WRITE_DATA_COMPLETE_TIMEOUT)) {
			ret = -1;
		}

	} else {
		if ((sd_card_type == C_MEDIA_SDHC_CARD)||(sd_card_type == C_MEDIA_MMCHC_CARD)) {
			if (drvl2_sd_adtc_command_set(C_SD_CMD25, start_sector, 0, &response)) {
				SD_IO_Release();
				return -1;
			}
		} else {
			if (drvl2_sd_adtc_command_set(C_SD_CMD25, start_sector<<9, 0, &response)) {
				SD_IO_Release();
				return -1;
			}
		}
		if (drvl1_sdc_command_complete_wait(SD_DEVICE_NUM, C_SD_L2_MULTI_WRITE_CMD_COMPLETE_TIMEOUT)) {
			SD_IO_Release();
			return -1;
		}
		sd_card_state = SD_CARD_STATE_RECEIVE_DATA;

		ret = drvl1_sdc_write_data_by_dma(SD_DEVICE_NUM, buffer, sector_count, NULL);

		if (drvl1_sdc_data_crc_status_get(SD_DEVICE_NUM)) {
			ret = -1;
		}
		if (drvl1_sdc_data_complete_wait(SD_DEVICE_NUM, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT)) {
			ret = -1;
		}
		if (drvl1_sdc_stop_controller(SD_DEVICE_NUM, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT)) {
			ret = -1;
		}

		if (drvl2_sd_bcr_ac_command_set(C_SD_CMD12, 0x0, C_RESP_R1B, &response)) {
			ret = -1;
		}
		sd_card_state = SD_CARD_STATE_PROGRAMMING;

	}

	// Wait operation complete and back to transfer state
	ret = drvl2_sdc_card_busy_wait(C_SD_L2_CARD_PROGRAM_TIMEOUT);
	sd_card_state = SD_CARD_STATE_TRANSFER;
	SD_IO_Release();
	return ret;
}

INT32S drvl2_sd_card_remove(void)
{
	INT32U response;
	SD_IO_Request(SD_PIN_IDX);
	// Reset card to idle state
	if (sd_card_state >= SD_CARD_STATE_TRANSFER) {
		if (drvl2_sd_bcr_ac_command_set(C_SD_CMD13, sd_card_rca, C_RESP_R1, &response)) {
			sd_card_state = SD_CARD_STATE_INACTIVE;
		} else {
			if ((response & 0x1E00) < 0x800) {
				// If card exists and not in transfer state, send it back to idle state
				drvl2_sd_bc_command_set(C_SD_CMD0, 0x0);
				drvl2_sd_bc_command_set(C_SD_CMD0, 0x0);
				drvl2_sd_bc_command_set(C_SD_CMD0, 0x0);
				sd_card_state = SD_CARD_STATE_IDLE;
			} else if ((response & 0x1E00) != 0x800) {
				// If card is in sending-data or receive data or programming state, send stop command
				drvl2_sd_bcr_ac_command_set(C_SD_CMD12, 0x0, C_RESP_R1B, &response);

				// Wait until card is not busy before turning off controller
				drvl2_sdc_card_busy_wait(C_SD_L2_CARD_PROGRAM_TIMEOUT);
			}
		}
	}

	drvl1_sdc_disable(SD_DEVICE_NUM);
	SD_IO_Release();
	return 0;
}

INT32S drvl2_sd_read_start(INT32U start_sector, INT32U sector_count)
{
	INT32U response;

	if (sd_card_state != SD_CARD_STATE_TRANSFER) {
		return -1;
	}
	SD_IO_Request(SD_PIN_IDX);
	drvl2_sd_set_speed_bus();
	if (drvl2_sdc_card_busy_wait(C_SD_L2_CARD_BUSY_TIMEOUT)) {
		SD_IO_Release();
		return -1;
	}

	// Clear SD RX data buffer before read command is issued
	drvl1_sdc_clear_rx_data_register(SD_DEVICE_NUM);

	if ((sd_card_type == C_MEDIA_SDHC_CARD)||(sd_card_type == C_MEDIA_MMCHC_CARD)) {
		if (drvl2_sd_adtc_command_set(C_SD_CMD18, start_sector, 0, &response)) {
			SD_IO_Release();
			return -1;
		}
	} else {
		if (drvl2_sd_adtc_command_set(C_SD_CMD18, start_sector<<9, 0, &response)) {
			SD_IO_Release();
			return -1;
		}
	}

	sd_card_state = SD_CARD_STATE_SENDING_DATA;

	return 0;
}

static volatile INT8S sd_read_sector_result;
INT32S drvl2_sd_read_sector(INT32U *buffer, INT32U sector_count, INT8U wait_flag)
{
	if (wait_flag == 0) {	// Start DMA and return immediately
		return drvl1_sdc_read_data_by_dma(SD_DEVICE_NUM, buffer, sector_count, (INT8S *) &sd_read_sector_result);
	} else if (wait_flag == 1) {	// Start DMA and wait until done
		INT32S ret = drvl1_sdc_read_data_by_dma(SD_DEVICE_NUM, buffer, sector_count, NULL);
		if(ret!=0)
			SD_IO_Release();
		return ret;
	}

	// Query status and return when done
	while (sd_read_sector_result == C_DMA_STATUS_WAITING) ;
	if (sd_read_sector_result != C_DMA_STATUS_DONE) {
		SD_IO_Release();
		return -1;
	}

	return 0;
}

INT32S drvl2_sd_read_stop(void)
{
	INT32U response;
	INT32S ret;

	ret = 0;
	if (drvl1_sdc_data_crc_status_get(SD_DEVICE_NUM)) {
		ret = -1;
	}
	if (drvl1_sdc_command_complete_wait(SD_DEVICE_NUM, C_SD_L2_MULTI_READ_CMD_COMPLETE_TIMEOUT)) {
		ret = -1;
	}
	if (drvl1_sdc_stop_controller(SD_DEVICE_NUM, C_SD_L2_READ_CONTROLLER_STOP_TIMEOUT)) {
		ret = -1;
	}

	drvl2_sd_bcr_ac_command_set(C_SD_CMD12, 0x0, C_RESP_R1B, &response);
	sd_card_state = SD_CARD_STATE_TRANSFER;
	SD_IO_Release();
	return ret;
}

INT32S drvl2_sd_write_start(INT32U start_sector, INT32U sector_count)
{
	INT32U response;

	if (sd_card_state != SD_CARD_STATE_TRANSFER) {
		return -1;
	}
	if (drvl2_sdc_card_protect_get()) {
		return -1;
	}
	SD_IO_Request(SD_PIN_IDX);
	drvl2_sd_set_speed_bus();
	if (drvl2_sdc_card_busy_wait(C_SD_L2_CARD_BUSY_TIMEOUT)) {
		SD_IO_Release();
		return -1;
	}

	if ((sd_card_type == C_MEDIA_SDHC_CARD)||(sd_card_type == C_MEDIA_MMCHC_CARD)) {
		if (drvl2_sd_adtc_command_set(C_SD_CMD25, start_sector, 0, &response)) {
			SD_IO_Release();
			return -1;
		}
	} else {
		if (drvl2_sd_adtc_command_set(C_SD_CMD25, start_sector<<9, 0, &response)) {
			SD_IO_Release();
			return -1;
		}
	}

	if (drvl1_sdc_command_complete_wait(SD_DEVICE_NUM, C_SD_L2_MULTI_WRITE_CMD_COMPLETE_TIMEOUT)) {
		SD_IO_Release();
		return -1;
	}
	sd_card_state = SD_CARD_STATE_RECEIVE_DATA;

	return 0;
}

static volatile INT8S sd_write_sector_result;
INT32S drvl2_sd_write_sector(INT32U *buffer, INT32U sector_count, INT8U wait_flag)
{
	if (wait_flag == 0) {	// Start DMA and return immediately
		return drvl1_sdc_write_data_by_dma(SD_DEVICE_NUM, buffer, sector_count, (INT8S *) &sd_write_sector_result);
	} else if (wait_flag == 1) {	// Start DMA and wait until done
		INT32S ret = drvl1_sdc_write_data_by_dma(SD_DEVICE_NUM, buffer, sector_count, NULL);
		if(ret!=0)
			SD_IO_Release();
		return ret;
	}

	// Query status and return when done
	while (sd_write_sector_result == C_DMA_STATUS_WAITING) ;
	if (sd_write_sector_result != C_DMA_STATUS_DONE) {
		SD_IO_Release();
		return -1;
	}

	return 0;
}

INT32S drvl2_sd_write_stop(void)
{
	INT32U response;
	INT32S ret;

	ret = 0;
	if (drvl1_sdc_data_complete_wait(SD_DEVICE_NUM, C_SD_L2_MULTI_WRITE_DATA_COMPLETE_TIMEOUT)) {
		ret = -1;
	}
	if (drvl1_sdc_data_crc_status_get(SD_DEVICE_NUM)) {
		ret = -1;
	}
	if (drvl1_sdc_stop_controller(SD_DEVICE_NUM, C_SD_L2_WRITE_CONTROLLER_STOP_TIMEOUT)) {
		ret = -1;
	}

	drvl2_sd_bcr_ac_command_set(C_SD_CMD12, 0x0, C_RESP_R1B, &response);
	sd_card_state = SD_CARD_STATE_PROGRAMMING;

	// Wait operation complete and back to transfer state
	ret = drvl2_sdc_card_busy_wait(C_SD_L2_CARD_PROGRAM_TIMEOUT);

	sd_card_state = SD_CARD_STATE_TRANSFER;
	SD_IO_Release();
	return ret;
}

INT32S drvl2_sd_set_clock_ext(INT32U bus_speed)
{
	if(bus_speed > 50000000)
		return -1;// remain unchanged
	sd_card_speed = bus_speed;
	return 0;
}

//#endif		// _DRV_L2_SD
