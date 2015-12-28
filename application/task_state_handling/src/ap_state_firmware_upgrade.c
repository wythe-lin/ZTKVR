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
#include "gplib.h"
#include "zt31xx.h"

#define msg(x)					print_string x

/* for debug */
#define DEBUG_AP_STATE_FIRMWARE_UPGRADE		1
#if DEBUG_AP_STATE_FIRMWARE_UPGRADE
    #define _dmsg(x)				print_string x
#else
    #define _dmsg(x)
#endif

/* constants */
#define UPGRADE_POS_X		10
#define UPGRADE_POS_Y		100


/*
 *****************************************************************************
 * utility function
 *
 *
 *
 *****************************************************************************
 */
/* detect power key */
static void fwup_system_hold(void)
{
	int	i = 0;

	msg((RED "[m]: system hold ...\r\n" NONE));
	while (1) {
#if KEY_ACTIVE	//wwj add
		if (gpio_read_io(PW_KEY)) {
#else
		if (!gpio_read_io(PW_KEY)) {
#endif
			i++;
		} else {
			i=0;
		}

		if (i >= 3) {
#if KEY_ACTIVE	//wwj add
			while (gpio_read_io(PW_KEY));
#else
			while (!gpio_read_io(PW_KEY));
#endif

			gpio_write_io(POWER_EN, 0);

		}
		OSTimeDly(5);
	}
}

/* clear display buffer */
static void fwup_clear_dispbuf(INT32U addr, INT32U size)
{
	INT32U	i;
	INT32U	*ptr;

	ptr = (INT32U *) addr;
	size >>= 2;
	for (i=0; i<size; i++) {
		*ptr++ = 0;
	}
}

/* draw ascii string in display buffer */
static void fwup_ascii_string_init(STRING_ASCII_INFO *ascii, INT16U color, INT16U type, INT16U width, INT16U height)
{
	ascii->font_color = color;
	ascii->font_type  = type;
	ascii->buff_w	  = width;
	ascii->buff_h	  = height;
}

static void fwup_ascii_string_draw(STRING_ASCII_INFO *ascii, INT32U addr, INT16S x, INT16S y, char *str)
{
	ascii->pos_x   = x;
	ascii->pos_y   = y;
	ascii->str_ptr = str;
	ap_state_resource_string_ascii_draw((INT16U *) addr, ascii);
}


/*
 *****************************************************************************
 * upgrade ZTKVR firmware
 *
 *
 *
 *****************************************************************************
 */
/*  for ztkvr_upgrade.bin */
static void fwup_upgrade_ztkvr(void)
{
	INT32U i, j, k, total_size, complete_size;
	INT32U *firmware_buffer;
	INT32U display_buffer;
	INT32U buff_size;
	INT16U *ptr;
	struct stat_t statetest;
	INT16S fd;
	STRING_ASCII_INFO ascii_str;
	INT8S  prog_str[6];
	INT8U retry=0;

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

	fwup_clear_dispbuf(display_buffer, buff_size);
	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);

	fwup_ascii_string_init(&ascii_str, 0xFFFF, 0, TFT_WIDTH, TFT_HEIGHT);
	fwup_ascii_string_draw(&ascii_str, display_buffer, UPGRADE_POS_X, UPGRADE_POS_Y,    "Upgrading ZTKVR firmware...");
	fwup_ascii_string_draw(&ascii_str, display_buffer, UPGRADE_POS_X, UPGRADE_POS_Y+20, "Do not power off now");
	OSQPost(DisplayTaskQ, (void *) (display_buffer | MSG_DISPLAY_TASK_JPEG_DRAW));

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
				ascii_str.str_ptr = (CHAR *) prog_str;
				ap_state_resource_string_ascii_draw((INT16U *) display_buffer, &ascii_str);
				OSQPost(DisplayTaskQ, (void *) (display_buffer | MSG_DISPLAY_TASK_JPEG_DRAW));
			} else {
				retry++;
			}
		}
		if (retry == C_UPGRADE_FAIL_RETRY) {
			break;
		}
	}
	OSTimeDly(5);

	fwup_clear_dispbuf(display_buffer, buff_size);
	OSTimeDly(150);

	if (retry != C_UPGRADE_FAIL_RETRY) {
		fwup_ascii_string_draw(&ascii_str, display_buffer, UPGRADE_POS_X, UPGRADE_POS_Y,    "Remove SD card and");
		fwup_ascii_string_draw(&ascii_str, display_buffer, UPGRADE_POS_X, UPGRADE_POS_Y+20, "restart now");
		fwup_ascii_string_draw(&ascii_str, display_buffer, UPGRADE_POS_X, UPGRADE_POS_Y+40, "100%");
	} else {
		fwup_ascii_string_draw(&ascii_str, display_buffer, UPGRADE_POS_X, UPGRADE_POS_Y+10, "Upgrade fail");
	}
	OSQPost(DisplayTaskQ, (void *) (display_buffer | MSG_DISPLAY_TASK_JPEG_DRAW));

	// system hold ...
	fwup_system_hold();
}


/*
 *****************************************************************************
 * upgrade ZT31XX firmware
 *
 *
 *
 *****************************************************************************
 */
#define SZ_64KB			(64*1024)

/* for zt3150_upgrade.bin/zt3120_upgrade.bin */
static void fwup_upgrade_zt31xx(unsigned char n)
{
	STRING_ASCII_INFO	ascii_str;
	struct stat_t		statetest;
	INT8U			retry = 0;
	INT16S			fd;
	INT32U			disp_buff, buff_size;
	INT32U			*firmware;
	INT32U			filelen, memlen;

	_dmsg((BROWN "[S]: upgrade_zt31xx()\r\n" NONE));

	// read bin file to sdram
	if (n == 0) {
		fd = open("C:\\zt3150_upgrade.bin", O_RDONLY);
	} else {
		fd = open("C:\\zt3120_upgrade.bin", O_RDONLY);
	}
	if (fd < 0) {
		msg(("[m]: cannot open file\r\n"));
		return;
	}
	if (fstat(fd, &statetest)) {
		msg(("[m]: get file size fail\r\n"));
		close(fd);
		return;
	}
	filelen = statetest.st_size;
	if (filelen <= SZ_64KB) {
		memlen = SZ_64KB;
	} else {
		memlen = ((filelen + (SZ_64KB - 1)) / SZ_64KB) * SZ_64KB;
	}
	msg(("[m]: %s file length=0x%08x\r\n", (n==0) ? "zt3150_upgrade.bin" : "zt3120_upgrade.bin", filelen));

	// 
	firmware = (INT32U *) gp_malloc(memlen);
	if (!firmware) {
		msg(("[m]: bin file buffer allocte fail\r\n"));
		close(fd);
		return;
	}
	msg(("[m]: allocate buffer size=0x%08x\r\n", memlen));

	// initial display buffer
	buff_size = TFT_WIDTH * TFT_HEIGHT * 2;
	disp_buff = (INT32U) gp_malloc_align(buff_size, 64);
	if (!disp_buff) {
		close(fd);
		msg(("[m]: display buffer allocate fail\r\n"));
		return;
	}
	fwup_clear_dispbuf(disp_buff, buff_size);
	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);

	fwup_ascii_string_init(&ascii_str, 0xFFFF, 0, TFT_WIDTH, TFT_HEIGHT);
	fwup_ascii_string_draw(&ascii_str, disp_buff, UPGRADE_POS_X, UPGRADE_POS_Y, (n == 0) ? "Upgrading ZT3150 firmware..." : "Upgrading ZT3120 firmware...");
	fwup_ascii_string_draw(&ascii_str, disp_buff, UPGRADE_POS_X, UPGRADE_POS_Y+20, "Do not power off now");
	OSQPost(DisplayTaskQ, (void *) (disp_buff | MSG_DISPLAY_TASK_JPEG_DRAW));
	OSTimeDly(5);

	// read bin file
	if (read(fd, (INT32U) firmware, memlen) <= 0) {
		msg(("[m]: read file fail, stop procedure...\r\n"));
		retry = C_UPGRADE_FAIL_RETRY;
		goto __exit;
	}

//	zt31xx_spi_write((unsigned long) firmware, 0, memlen);
	OSTimeDly(150);

__exit:
	// 
	fwup_clear_dispbuf(disp_buff, buff_size);
	if (retry != C_UPGRADE_FAIL_RETRY) {
		fwup_ascii_string_draw(&ascii_str, disp_buff, UPGRADE_POS_X, UPGRADE_POS_Y,    "Remove SD card and");
		fwup_ascii_string_draw(&ascii_str, disp_buff, UPGRADE_POS_X, UPGRADE_POS_Y+20, "restart now");
		fwup_ascii_string_draw(&ascii_str, disp_buff, UPGRADE_POS_X, UPGRADE_POS_Y+40, "100%");
	} else {
		fwup_ascii_string_draw(&ascii_str, disp_buff, UPGRADE_POS_X, UPGRADE_POS_Y+10, "Upgrade fail");
	}
	OSQPost(DisplayTaskQ, (void *) (disp_buff | MSG_DISPLAY_TASK_JPEG_DRAW));

	// system hold ...
	fwup_system_hold();

	_dmsg((BROWN "[E]: upgrade_zt31xx()\r\n" NONE));
}


/*
 *****************************************************************************
 * ap_state_firmware_upgrade()
 *
 *
 *
 *****************************************************************************
 */
void ap_state_firmware_upgrade(void)
{
	switch (storage_sd_upgrade_file_flag_get()) {
	case 2:		fwup_upgrade_ztkvr();	break;		// upgrade ZTKVR  firmware
	case 3:		fwup_upgrade_zt31xx(0);	break;		// upgrade ZT3150 firmware
	case 4:		fwup_upgrade_zt31xx(1);	break;		// upgrade ZT3120 firmware
	case 5:					break;		// merge   ZT3150 firmware
	default:	break;
	}
}

