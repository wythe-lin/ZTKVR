/*
* Description: This file provides APIs to upgrade firmware from SD card to SPI
*
* Author: Tristan Yang
*
* Date: 2008/02/17
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.00
*/
#include "ap_state_firmware_upgrade.h"
#include "stdio.h"

#define UPGRADE_POS_X 10
#define UPGRADE_POS_Y 100

void ap_state_firmware_upgrade(void)
{
  
	INT32U i, j, k, total_size, complete_size;
	INT32U *firmware_buffer;
	INT32U display_buffer;
	INT32U buff_size, *buff_ptr;
	INT16U *ptr; 
	struct stat_t statetest;
	INT16S fd;
	STRING_ASCII_INFO ascii_str;
	INT8S  prog_str[6];
	INT8U retry=0;
	
	if (storage_sd_upgrade_file_flag_get() != 2) {
		return;
	}
	
	fd = open("C:\\ztkvr_upgrade.bin", O_RDONLY);
	if (fd < 0) {
		return;
	}

	if (fstat(fd, &statetest)) {
		close(fd);
		return;
	}
	total_size = statetest.st_size;

	firmware_buffer = (INT32U *) gp_malloc(C_UPGRADE_BUFFER_SIZE);
	if (!firmware_buffer) {
		close(fd);
		return;
	}
	
	buff_size = TFT_WIDTH * TFT_HEIGHT * 2;
	display_buffer = (INT32U) gp_malloc_align(buff_size, 64);
	if (!display_buffer) {
		close(fd);
		DBG_PRINT("firmware upgrade allocate display buffer fail\r\n");
		return;
	}
	
	buff_ptr = (INT32U*) display_buffer;
	buff_size >>= 2;
	for (i=0;i<buff_size;i++) {
		*buff_ptr++ = 0;
	}
	
	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);
	
	ascii_str.font_color = 0xFFFF;
	ascii_str.font_type = 0;
	ascii_str.buff_w = TFT_WIDTH;
	ascii_str.buff_h = TFT_HEIGHT;
	ascii_str.pos_x = UPGRADE_POS_X;
	ascii_str.pos_y = UPGRADE_POS_Y;
	ascii_str.str_ptr = "Upgrading firmware...";
	ap_state_resource_string_ascii_draw((INT16U *)display_buffer, &ascii_str);
	
	ascii_str.pos_x = UPGRADE_POS_X;
	ascii_str.pos_y = UPGRADE_POS_Y+20;
	ascii_str.str_ptr = "Do not power off now";
	ap_state_resource_string_ascii_draw((INT16U *)display_buffer, &ascii_str);
	
	OSQPost(DisplayTaskQ, (void *) (display_buffer|MSG_DISPLAY_TASK_JPEG_DRAW));
	OSTimeDly(5);
	
	complete_size = 0;
	while (complete_size < total_size) {
		INT32U buffer_left;

		if (read(fd, (INT32U) firmware_buffer, C_UPGRADE_BUFFER_SIZE) <= 0) {
			break;
		}
		buffer_left = (total_size - complete_size + (C_UPGRADE_SPI_BLOCK_SIZE-1)) & ~(C_UPGRADE_SPI_BLOCK_SIZE-1);
		if (buffer_left > C_UPGRADE_BUFFER_SIZE) {
			buffer_left = C_UPGRADE_BUFFER_SIZE;
		}
		while (buffer_left && retry<C_UPGRADE_FAIL_RETRY) {
			complete_size &= ~(C_UPGRADE_SPI_BLOCK_SIZE-1);
			if (SPI_Flash_erase_block(complete_size)) {
				retry++;
				continue;
			}
			for (j=C_UPGRADE_SPI_BLOCK_SIZE; j; j-=C_UPGRADE_SPI_WRITE_SIZE) {
				if (SPI_Flash_write_page(complete_size, (INT8U *) (firmware_buffer + ((complete_size & (C_UPGRADE_BUFFER_SIZE-1))>>2)))) {
					break;
				}
				complete_size += C_UPGRADE_SPI_WRITE_SIZE;
			}
			if (j == 0) {
				buffer_left -= C_UPGRADE_SPI_BLOCK_SIZE;

				if (complete_size < total_size) {
					j = complete_size*100/total_size;
				} else {
					j = 100;
				}
				for (i=0;i<50;i++) {
					ptr = (INT16U *) (display_buffer+((UPGRADE_POS_Y+40+i)*TFT_WIDTH+UPGRADE_POS_X)*2);
					for(k=0;k<50;k++) {
						*ptr++ = 0x0;
					}
				}
				ascii_str.pos_x = UPGRADE_POS_X;
				ascii_str.pos_y = UPGRADE_POS_Y+40;
				sprintf((CHAR*)prog_str,"%d%c",j,'%');
				ascii_str.str_ptr = (CHAR *)prog_str;
				ap_state_resource_string_ascii_draw((INT16U *)display_buffer, &ascii_str);
				OSQPost(DisplayTaskQ, (void *) (display_buffer|MSG_DISPLAY_TASK_JPEG_DRAW));
			} else {
				retry++;
			}
		}
		if (retry == C_UPGRADE_FAIL_RETRY) {
			break;
		}
	}
	OSTimeDly(5);
	buff_ptr = (INT32U*) display_buffer;
	for (i=0;i<buff_size;i++) {
		*buff_ptr++ = 0;
	}
	OSTimeDly(150);
	
	if (retry != C_UPGRADE_FAIL_RETRY) {
		ascii_str.pos_x = UPGRADE_POS_X;
		ascii_str.pos_y = UPGRADE_POS_Y;
		ascii_str.str_ptr = "Remove SD card and";
		ap_state_resource_string_ascii_draw((INT16U *)display_buffer, &ascii_str);
		
		ascii_str.pos_x = UPGRADE_POS_X;
		ascii_str.pos_y = UPGRADE_POS_Y+20;
		ascii_str.str_ptr = "restart now";
		ap_state_resource_string_ascii_draw((INT16U *)display_buffer, &ascii_str);
		
		ascii_str.pos_x = UPGRADE_POS_X;
		ascii_str.pos_y = UPGRADE_POS_Y+40;
		ascii_str.str_ptr = "100%";
		ap_state_resource_string_ascii_draw((INT16U *)display_buffer, &ascii_str);
	}
	else {
		ascii_str.pos_x = UPGRADE_POS_X+10;
		ascii_str.pos_y = UPGRADE_POS_Y;
		ascii_str.str_ptr = "Upgrade fail";
		ap_state_resource_string_ascii_draw((INT16U *)display_buffer, &ascii_str);
	}
	
	OSQPost(DisplayTaskQ, (void *) (display_buffer|MSG_DISPLAY_TASK_JPEG_DRAW));
	
	i=0;
	while(1) {
	  #if KEY_ACTIVE	//wwj add
		if (gpio_read_io(PW_KEY)) {
	  #else
		if (!gpio_read_io(PW_KEY)) {
	  #endif
			i++;
		}
		else {
			i=0;
		}
		if (i >= 3) {
		  #if KEY_ACTIVE	//wwj add
			while(gpio_read_io(PW_KEY));
		  #else
			while(!gpio_read_io(PW_KEY));
		  #endif
  
			gpio_write_io(POWER_EN,0);

		}
		OSTimeDly(5);
	}
}
