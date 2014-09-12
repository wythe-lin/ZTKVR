#include "ap_state_handling.h"

// Image decoding relative global definitions and variables
static INT32U jpeg_output_format;
static INT8U jpeg_output_ratio;             // 0=Fit to output_buffer_width and output_buffer_height, 1=Maintain ratio and fit to output_buffer_width or output_buffer_height, 2=Same as 1 but without scale up
static INT16U jpeg_output_buffer_width;
static INT16U jpeg_output_buffer_height;
static INT16U jpeg_output_image_width;
static INT16U jpeg_output_image_height;
static INT32U out_of_boundary_color;
static INT32U jpeg_output_buffer_pointer;

// Control variables
static INT32U jpeg_fifo_line_num = 32;

// JPEG variables
static INT16U jpeg_valid_width, jpeg_valid_height, jpeg_image_yuv_mode;
static INT32U jpeg_extend_width, jpeg_extend_height;
static INT32U jpeg_fifo_register = C_JPG_FIFO_32LINE;

// Buffer addresses
static INT32U jpeg_out_y, jpeg_out_cb, jpeg_out_cr;
static INT32U scaler_out_buffer;

// Scaler variables
static INT32U scaler_fifo_register = C_SCALER_CTRL_FIFO_32LINE;

static STR_ICON sh_str;
static DISPLAY_ICONMOVE zoom_idx;
static INT8U sh_curr_storage_id;
static INT32U jpeg_quality;
#if C_BATTERY_DETECT == CUSTOM_ON 
	static INT8U bat_icon = 3;
	static INT8U charge_icon = 0;	//wwj add
#endif
static INT8U scroll_bar_timerid;

#if C_POWER_OFF_LOGO == CUSTOM_ON
	static INT32S poweroff_logo_img_ptr;
	static INT32U poweroff_logo_decode_buff;
#endif

INT16U present_state;

void ap_state_handling_mp3_index_show_cmd(void);
void ap_state_handling_mp3_total_index_show_cmd(void);
void ap_state_handling_mp3_all_index_clear_cmd(void);
void ap_state_handling_mp3_volume_show_cmd(void);
void ap_state_handling_mp3_FM_channel_show_cmd(void);
void ap_state_handling_mp3_index_show_zero_cmd(void);
void ap_state_handling_mp3_total_index_show_zero_cmd(void);

extern void preview_zoom_set(INT32U factor);

INT32S ap_state_handling_str_draw(INT16U str_index, INT16U str_color)
{
	INT32U i, size;
	t_STRING_TABLE_STRUCT str_res;
	STRING_INFO str;
	
	if (sh_str.addr) {
		gp_free((void *) sh_str.addr);
	}
	str.font_type = 0;
	str.str_idx = str_index;
	str.language = ap_state_config_language_get() - 1;
	if (ap_state_resource_string_resolution_get(&str, &str_res)) {
		DBG_PRINT("String resolution get fail\r\n");
		return STATUS_FAIL;
	}
	str.buff_w = sh_str.w = str_res.string_width;
	str.buff_h = sh_str.h = str_res.string_height;
	str.pos_x = str.pos_y = 0;
	str.font_color = str_color;
	size = sh_str.w*sh_str.h;
	sh_str.addr = (INT32U) gp_malloc(size << 1);
	if (!sh_str.addr) {
		DBG_PRINT("String memory malloc fail\r\n");
		return STATUS_FAIL;
	}
	for (i=0 ; i<size ; i++) {
		*((INT16U *) sh_str.addr + i) = TRANSPARENT_COLOR;
	}
	ap_state_resource_string_draw((INT16U *) sh_str.addr, &str);
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_STRING_DRAW | ((INT32U) &sh_str)));
	return STATUS_OK;
}

void ap_state_handling_str_draw_exit(void)
{
	if (sh_str.addr) {
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_STRING_DRAW | 0xFFFFFF));
		OSTimeDly(3);
		gp_free((void *) sh_str.addr);
		sh_str.addr = 0;
	}
}

//If you want to draw over 3 icons, pls call this function again.
void ap_state_handling_icon_show_cmd(INT8U cmd1, INT8U cmd2, INT8U cmd3)
{	
	if( ((cmd1 < 27) || (cmd1 > 34)) && ((cmd2 < 27) || (cmd2 > 34)) && ((cmd3 < 27) || (cmd3 > 34)) ){
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_SHOW | (cmd1 | (cmd2<<8) | (cmd3<<16))));
	}else{
		if( ((present_state == STATE_VIDEO_PREVIEW) || (present_state == STATE_VIDEO_RECORD)) && (fm_tx_status_get() == 1) ){
			OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_SHOW | (cmd1 | (cmd2<<8) | (cmd3<<16))));
		}
	}
}

void ap_state_handling_icon_clear_cmd(INT8U cmd1, INT8U cmd2, INT8U cmd3)
{	
	if( ((cmd1 < 27) || (cmd1 > 34)) && ((cmd2 < 27) || (cmd2 > 34)) && ((cmd3 < 27) || (cmd3 > 34)) ){
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | (cmd1 | (cmd2<<8) | (cmd3<<16))));
	}else{
		if( ((present_state == STATE_VIDEO_PREVIEW) || (present_state == STATE_VIDEO_RECORD)) && (fm_tx_status_get() == 1) ){
			OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | (cmd1 | (cmd2<<8) | (cmd3<<16))));
		}
	}
}

void ap_state_handling_mp3_index_show_cmd(void)
{	
	if( ((present_state == STATE_VIDEO_PREVIEW) || (present_state == STATE_VIDEO_RECORD)) && (fm_tx_status_get() == 1) ){
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MP3_INDEX_SHOW | ap_music_index_get()) );
	}
}

void ap_state_handling_mp3_total_index_show_cmd(void)
{	
	if( ((present_state == STATE_VIDEO_PREVIEW) || (present_state == STATE_VIDEO_RECORD)) && (fm_tx_status_get() == 1) ){
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MP3_TOTAL_INDEX_SHOW | storage_file_nums_get()) );
	}			
}

void ap_state_handling_mp3_index_show_zero_cmd(void)
{	
	if( ((present_state == STATE_VIDEO_PREVIEW) || (present_state == STATE_VIDEO_RECORD)) && (fm_tx_status_get() == 1) ){
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MP3_INDEX_SHOW | 0) );
	}
}

void ap_state_handling_mp3_total_index_show_zero_cmd(void)
{	
	if( ((present_state == STATE_VIDEO_PREVIEW) || (present_state == STATE_VIDEO_RECORD)) && (fm_tx_status_get() == 1) ){
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MP3_TOTAL_INDEX_SHOW | 0) );
	}			
}

void ap_state_handling_mp3_volume_show_cmd(void)
{	
	if( ((present_state == STATE_VIDEO_PREVIEW) || (present_state == STATE_VIDEO_RECORD)) && (fm_tx_status_get() == 1) ){
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MP3_VOLUME_SHOW | audio_fg_vol_get()) );
	}
}

void ap_state_handling_mp3_FM_channel_show_cmd(void)
{	
	if( ((present_state == STATE_VIDEO_PREVIEW) || (present_state == STATE_VIDEO_RECORD)) && (fm_tx_status_get() == 1) ){
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MP3_FM_CHANNEL_SHOW | audio_fm_freq_ch_get()) );
	}
}

void ap_state_handling_mp3_all_index_clear_cmd(void)
{	
	if( ((present_state == STATE_VIDEO_PREVIEW) || (present_state == STATE_VIDEO_RECORD)) && (fm_tx_status_get() == 1) ){
		OSQPost(DisplayTaskQ, (void *)MSG_DISPLAY_TASK_MP3_ALL_INDEX_CLEAR );
	}
}

extern INT8U s_usbd_pin;	// "extern" isn't good...   Neal
void ap_state_handling_connect_to_pc(void)
{
	INT8U type;
	
	s_usbd_pin = 1;
	
	if (fm_tx_status_get()) {
	    extab_enable_set(EXTA,FALSE);
    }
//	OSQPost(USBAPPTaskQ, (void *) MSG_USBD_PLUG_IN);
	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_STOP, NULL, NULL, MSG_PRI_NORMAL);
	type = FALSE;
	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(INT8U), MSG_PRI_NORMAL);
	type = USBD_DETECT;
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);	
	ap_state_handling_icon_show_cmd(ICON_USB_CONNECT, NULL, NULL);
	
	if (ap_state_config_usb_mode_get() == 0) {
		ap_setting_sensor_command_switch(0x0E, 0x80, 0);	//night mode off
	    ap_setting_sensor_command_switch(0x13, 0x04, 1);	//ISO
    	ap_setting_sensor_command_switch(0xA6, 0x01, 0);	//Color	
		jpeg_quality = video_encode_get_jpeg_quality();
		video_encode_set_jpeg_quality(70);
		OSQPost(USBAPPTaskQ, (void *) MSG_USBCAM_PLUG_IN);
	} else {
		OSQPost(USBAPPTaskQ, (void *) MSG_USBD_PLUG_IN);
	}
}


void ap_state_handling_disconnect_to_pc(void)
{
	INT8U type = GENERAL_KEY;
	
	//OSQPost(USBAPPTaskQ, (void *) MSG_USBD_PLUG_OUT);
	s_usbd_pin = 0;
	
	if (ap_state_config_usb_mode_get() == 0) {
		video_encode_set_jpeg_quality(jpeg_quality);
		msgQSend(StorageServiceQ, MSG_STORAGE_USBD_PCAM_EXIT, NULL, NULL, MSG_PRI_NORMAL);
	} else {
		msgQSend(StorageServiceQ, MSG_STORAGE_USBD_EXIT, NULL, NULL, MSG_PRI_NORMAL);
	}
	
	if ( (audio_playing_state_get() == STATE_IDLE) || (audio_playing_state_get() == STATE_PAUSED) ){
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	}
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_KEY_REGISTER, &type, sizeof(INT8U), MSG_PRI_NORMAL);
	ap_state_handling_icon_clear_cmd(ICON_USB_CONNECT, NULL, NULL);
	if (fm_tx_status_get()) {
	    extab_enable_set(EXTA,TRUE);
	}
}

void ap_state_handling_storage_id_set(INT8U stg_id)
{
	sh_curr_storage_id = stg_id;
}

INT8U ap_state_handling_storage_id_get(void)
{
	return sh_curr_storage_id;
}

void ap_state_handling_led_on(void)
{	
	INT8U type = DATA_LOW;
	
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &type, sizeof(INT8U), MSG_PRI_NORMAL);
}

void ap_state_handling_led_off(void)
{	
	INT8U type = DATA_HIGH;
	
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &type, sizeof(INT8U), MSG_PRI_NORMAL);
}

void ap_state_handling_led_flash_on(void)	//wwj add
{	
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_FLASH_SET, NULL, NULL, MSG_PRI_NORMAL);
}

void ap_state_handling_led_blink_on(void)	//wwj add
{	
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_BLINK_SET, NULL, NULL, MSG_PRI_NORMAL);
}


void ap_state_handling_power_off(void)
{
#if C_POWER_OFF_LOGO == CUSTOM_ON
	IMAGE_DECODE_STRUCT img_info;
	INT32U size,tmp;
	INT16U	logo_fd;
	INT32U  cnt;

	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);
	OSQPost(scaler_task_q, (void *) (0xFFFFFF|MSG_SCALER_TASK_PACKER_FREE));
	audio_effect_play(EFFECT_POWER_OFF);
	OSTimeDly(10);

	logo_fd = nv_open((INT8U *) "POWER_OFF_LOGO.JPG");
	if (logo_fd != 0xFFFF) {
		poweroff_logo_decode_buff = (INT32U) gp_malloc_align(TFT_WIDTH * TFT_HEIGHT * 2, 64);
		if (poweroff_logo_decode_buff) {
			size = nv_rs_size_get(logo_fd);
			poweroff_logo_img_ptr = (INT32S) gp_malloc(size);
			if (poweroff_logo_img_ptr) {
				if (nv_read(logo_fd, (INT32U) poweroff_logo_img_ptr, size) == 0) {
					img_info.image_source = (INT32S) poweroff_logo_img_ptr;
					img_info.source_size = size;
					img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;
					img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
					img_info.output_ratio = 0;
					img_info.out_of_boundary_color = 0x008080;
					img_info.output_buffer_width = TFT_WIDTH;
					img_info.output_buffer_height = TFT_HEIGHT;
					img_info.output_image_width = TFT_WIDTH;
					img_info.output_image_height = TFT_HEIGHT;
					img_info.output_buffer_pointer = poweroff_logo_decode_buff;
					if (jpeg_buffer_decode_and_scale(&img_info) != STATUS_FAIL) {
						OSQPost(DisplayTaskQ, (void *) (poweroff_logo_decode_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
						OSTimeDly(100);
					} else {
						DBG_PRINT("Poweroff decode jpeg file fail.\r\n");
					}
				} else {
					DBG_PRINT("Failed to read resource_header in Poweroff\r\n");
				}

				gp_free((void *) poweroff_logo_img_ptr);
			} else {
				DBG_PRINT("Poweroff allocate jpeg input buffer fail.[%d]\r\n", size);
			}
			gp_free((void *) poweroff_logo_decode_buff);
		} else {
			DBG_PRINT("Poweroff allocate jpeg output buffer fail.\r\n");
		}
	}

	cnt = 0;
	while(dac_dma_status_get() == 1) {
		OSTimeDly(2);
		if (cnt++ >= 250) { //5 sec
		    break;
		}
	}
#endif

	ap_state_config_store();
			SPI_LOCK();
#if 0			
			tmp = R_SPI0_CTRL;
			R_SPI0_CTRL = 0;
			Write_COMMAND16i(0xf6);   
	 		Write_DATA16i(0x01);
			Write_DATA16i(0x00);
			Write_DATA16i(0x00);
			Write_COMMAND16i(0x28);      //Display off
 			cmd_delay(20); 
 			Write_COMMAND16i(0x10);      //ENTER Sleep 
  			cmd_delay(10);
  			R_SPI0_CTRL = tmp;
#endif
			SPI_UNLOCK();
	R_INT_GMASK = 1;
//	while(gpio_read_io(PW_KEY));
	tft_backlight_en_set(FALSE);
	extab_enable_set(EXTA,FALSE);
//	tft_tft_en_set(FALSE);
	gpio_write_io(LED, 1);
	gpio_write_io(SPEAKER_EN, 0);	// add by xyz - 2014.09.11
	
	video_encode_sensor_stop();
	rtc_int_set(GPX_RTC_HR_IEN|GPX_RTC_MIN_IEN|GPX_RTC_SEC_IEN|GPX_RTC_HALF_SEC_IEN,
               (RTC_DIS&GPX_RTC_HR_IEN)|(RTC_DIS&GPX_RTC_MIN_IEN)|(RTC_DIS&GPX_RTC_SEC_IEN)|(RTC_DIS&GPX_RTC_HALF_SEC_IEN));
 	rtc_schedule_disable();
	*P_USBD_CONFIG1 |= 0x100; //[8],SW Suspend For PHY
	ppu_init();
	dac_disable();
	spi_disable(SPI_0);
	adc_vref_enable_set(FALSE);
	system_da_ad_pll_en_set(FALSE);
	drvl2_sd_card_remove();
	/* set SD data pin to low */
	gpio_write_io(IO_C6, 0);
	gpio_write_io(IO_C8, 0);
	gpio_write_io(IO_C9, 0);

	// add by xyz begin - 2014.09.11
	DBG_PRINT("POWER OFF IO OFF\r\n");
	gpio_write_io(POWER_EN, 0);	
	// add by xyz end   - 2014.09.11

	timer_stop(0);		/* stop timer0 */
	timer_stop(1);		/* stop timer1 */
	timer_stop(2);		/* stop timer2 */
	timer_stop(3);		/* stop timer2 */
	time_base_stop(0);	/* stop timebase A */
	time_base_stop(1);
	time_base_stop(2);	/* stop timebase C */
	
#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2

  	DBG_PRINT("POWER OFF IO OFF\r\n");
	gpio_write_io(POWER_EN,0);

#endif
	
	while(gpio_read_io(PW_KEY));

	sys_weak_6M_set(TRUE);
	cache_disable();
	system_set_pll(6);
	sys_power_handler();
}

void ap_state_handling_calendar_init(void)
{
	INT8U dt[3];
	TIME_T  tm;

	ap_state_config_factory_date_get(dt);

	tm.tm_year = dt[0] + 2000;
	tm.tm_mon = dt[1];
	tm.tm_mday = dt[2];

	ap_state_config_factory_time_get(dt);

	tm.tm_hour = dt[0];
	tm.tm_min = dt[1];
	tm.tm_sec = dt[2];

	rtc_ext_to_int_set();

	cal_day_store_get_register(ap_state_config_base_day_set,ap_state_config_base_day_get,ap_state_config_store);
	cal_factory_date_time_set(&tm);
	calendar_init();
}

#if C_BATTERY_DETECT == CUSTOM_ON 
void ap_state_handling_battery_icon_show(INT8U bat_lvl)
{
	if (bat_lvl < 4) {
		if (bat_icon != bat_lvl) {
			ap_state_handling_icon_clear_cmd((bat_icon + 1), NULL, NULL);
			bat_icon = bat_lvl;
		}
		ap_state_handling_icon_show_cmd((bat_lvl + 1), NULL, NULL);
	}
}

void ap_state_handling_charge_icon_show(INT8U charge_flag)	//wwj add
{
	if(charge_flag) {
		charge_icon = 1;
		ap_state_handling_icon_show_cmd(ICON_BATTERY_CHARGED, NULL, NULL);
	} else {
		charge_icon = 0;
		ap_state_handling_icon_clear_cmd(ICON_BATTERY_CHARGED, NULL, NULL);
	}
}

void ap_state_handling_current_bat_lvl_show(void)	//wwj add
{
	ap_state_handling_icon_show_cmd((bat_icon + 1), NULL, NULL);
}

void ap_state_handling_current_charge_icon_show(void)	//wwj add
{
	if(charge_icon) {
		ap_state_handling_icon_show_cmd(ICON_BATTERY_CHARGED, NULL, NULL);
	} else {
		ap_state_handling_icon_clear_cmd(ICON_BATTERY_CHARGED, NULL, NULL);
	}
}

#endif

#if C_SCREEN_SAVER == CUSTOM_ON
void ap_state_handling_lcd_backlight_switch(INT8U enable)
{
	INT8U type;
	
	if(enable){
		type = BL_ON;
	}else{
		type = BL_OFF;
	}	
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LCD_BACKLIGHT_SET, &type, sizeof(INT8U), MSG_PRI_NORMAL);
}
#endif


static INT8U  night_mode_status = 0;
void ap_state_handling_night_mode_switch(void)	//wwj add
{

	if(night_mode_status) {
		night_mode_status = 0;
	} else {
		night_mode_status = 1;
	}

	if(night_mode_status) {
		ap_state_handling_icon_show_cmd(ICON_NIGHT_MODE_ENABLED, NULL, NULL);
		ap_state_handling_icon_clear_cmd(ICON_NIGHT_MODE_DISABLED, NULL, NULL);
	} else {
		ap_state_handling_icon_show_cmd(ICON_NIGHT_MODE_DISABLED, NULL, NULL);
		ap_state_handling_icon_clear_cmd(ICON_NIGHT_MODE_ENABLED, NULL, NULL);
	}
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_NIGHT_MODE_SET, &night_mode_status, sizeof(INT8U), MSG_PRI_NORMAL);
}


INT8U ap_state_handling_night_mode_get(void)	//wwj add
{
	return night_mode_status;
}

void ap_state_handling_scroll_bar_init(void)
{
	INT8U type;
	
	type = CUSTOM_ON;
	scroll_bar_timerid = 0xFF;
	zoom_idx.idx = ICON_SCROLL_BAR_IDX;
	zoom_idx.pos_x = 0xFFFF;
	zoom_idx.pos_y = 149;
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_ZOOM_FLAG, &type, sizeof(INT8U), MSG_PRI_NORMAL);
}

void ap_state_handling_scroll_bar_exit(INT8U exit_type)
{
	INT8U type;
	
	type = CUSTOM_OFF;
	ap_state_handling_icon_clear_cmd(ICON_SCROLL_BAR, ICON_SCROLL_BAR_IDX, NULL);
	if (scroll_bar_timerid != 0xFF) {
		sys_kill_timer(scroll_bar_timerid);
		scroll_bar_timerid = 0xFF;
	}
	if (exit_type == STATA_EXIT) {
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_ZOOM_FLAG, &type, sizeof(INT8U), MSG_PRI_NORMAL);
	}
}

void ap_state_handling_zoom_active(INT32U *factor, INT32U type)
{
	if (type == MSG_APQ_NEXT_KEY_ACTIVE) {
		*factor -= 1;
		if(*factor <= 1) {
			*factor = 1;
			zoom_idx.pos_y = 149;
		} else {
			zoom_idx.pos_y = 88 + 10*(7-*factor);
		}
	} else {
		*factor += 1;
		if(*factor >= 7) {
			*factor = 7;
			zoom_idx.pos_y = 88;
		} else {
			zoom_idx.pos_y = 149 - 10*(*factor-1);
		}
	}
	preview_zoom_set(*factor);

	if (scroll_bar_timerid == 0xFF) {
		scroll_bar_timerid = SCROLL_BAR_TIMER_ID;
		ap_state_handling_icon_show_cmd(ICON_SCROLL_BAR, ICON_SCROLL_BAR_IDX, NULL);
	} else {
		sys_kill_timer(scroll_bar_timerid);
	}
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_MOVE | (INT32U) &zoom_idx));
	sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_SCROLL_BAR_END, scroll_bar_timerid, SCROLL_BAR_TIME_INTERVAL);
}

INT32S ap_state_handling_jpeg_decode(STOR_SERV_PLAYINFO *info_ptr, INT32U jpg_output_addr)
{
	INT32U input_buff, size, shift_byte, data_tmp;
	IMAGE_DECODE_STRUCT img_info;

	if (info_ptr->file_type == TK_IMAGE_TYPE_JPEG) {
		size = info_ptr->file_size;
		shift_byte = 0;
	} else {
		shift_byte = 272;
		lseek(info_ptr->file_handle, shift_byte, SEEK_SET);
		if (read(info_ptr->file_handle, (INT32U) &data_tmp, 4) != 4) {
			return STATUS_FAIL;
		}
		
		if (data_tmp == VIDEO_STREAM) {
			shift_byte = 272;
		} else {
			shift_byte = 372;
		}
		lseek(info_ptr->file_handle, shift_byte, SEEK_SET);
		read(info_ptr->file_handle, (INT32U) &data_tmp, 4);
		read(info_ptr->file_handle, (INT32U) &size, 4);
		if (data_tmp == AUDIO_STREAM) {
			do {
				shift_byte += (size + 8);
				lseek(info_ptr->file_handle, shift_byte, SEEK_SET);
				read(info_ptr->file_handle, (INT32U) &data_tmp, 4);
				read(info_ptr->file_handle, (INT32U) &size, 4);
			} while (data_tmp == AUDIO_STREAM);
		} else if (data_tmp != VIDEO_STREAM) {
			return STATUS_FAIL;
		}
	}
	input_buff = (INT32U) gp_malloc(size);
	if (!input_buff) {
		DBG_PRINT("State browse allocate jpeg input buffer fail.[%d]\r\n", size);
		return STATUS_FAIL;
	}
	if (read(info_ptr->file_handle, input_buff, size) <= 0) {
		gp_free((void *) input_buff);
		DBG_PRINT("State browse read jpeg file fail.\r\n");
		return STATUS_FAIL;
	}
	close(info_ptr->file_handle);
	img_info.image_source = (INT32S) input_buff;
	img_info.source_size = size;
	img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;
	img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
	img_info.output_ratio = 0;
	img_info.out_of_boundary_color = 0x008080;
	img_info.output_buffer_width = TFT_WIDTH;
	img_info.output_buffer_height = TFT_HEIGHT;
	img_info.output_image_width = TFT_WIDTH;
	img_info.output_image_height = TFT_HEIGHT;
	img_info.output_buffer_pointer = jpg_output_addr;
	if (jpeg_buffer_decode_and_scale(&img_info) == STATUS_FAIL) {
		gp_free((void *) input_buff);
		DBG_PRINT("State browse decode jpeg file fail.\r\n");
		return STATUS_FAIL;
	}
	gp_free((void *) input_buff);
	if ( (audio_playing_state_get() == STATE_IDLE) || (audio_playing_state_get() == STATE_PAUSED) ){
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	}
	return STATUS_OK;
}

INT32S jpeg_buffer_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct)
{
	INT32S parse_status;
	INT8U *p_vlc;
	INT32U left_len;
  	INT32S jpeg_status;
  	INT32S scaler_status;
	INT8U scaler_done;

	if (!img_decode_struct) {
		return STATUS_FAIL;
	}
	
    jpeg_output_format = img_decode_struct->output_format;
    jpeg_output_ratio = 0;
    jpeg_output_buffer_width = img_decode_struct->output_buffer_width;
    jpeg_output_buffer_height = img_decode_struct->output_buffer_height;
    jpeg_output_image_width = img_decode_struct->output_image_width;
    jpeg_output_image_height = img_decode_struct->output_image_height;
    out_of_boundary_color = img_decode_struct->out_of_boundary_color;
    jpeg_output_buffer_pointer = img_decode_struct->output_buffer_pointer;

	// Initiate software header parser and hardware engine
	jpeg_decode_init();

 
	parse_status = jpeg_decode_parse_header((INT8U *) img_decode_struct->image_source, img_decode_struct->source_size);
    if (parse_status != JPEG_PARSE_OK) {
		DBG_PRINT("Parse header failed. Skip this file\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;

        return STATUS_FAIL;
	}

	jpeg_valid_width = jpeg_decode_image_width_get();
	jpeg_valid_height = jpeg_decode_image_height_get();
	jpeg_image_yuv_mode = jpeg_decode_image_yuv_mode_get();
	jpeg_extend_width = jpeg_decode_image_extended_width_get();
	jpeg_extend_height = jpeg_decode_image_extended_height_get();


	jpeg_memory_allocate(jpeg_fifo_line_num);
	if (!jpeg_out_y) {
		DBG_PRINT("Failed to allocate memory in jpeg_memory_allocate()\r\n");
		img_decode_struct->decode_status = -2;

		return -2;
	}

	if (jpeg_decode_output_set(jpeg_out_y, jpeg_out_cb, jpeg_out_cr, jpeg_fifo_register)) {
		DBG_PRINT("Failed to call jpeg_decode_output_set()\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
	  	gp_free((void *) jpeg_out_y);
	  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
	  		gp_free((void *) jpeg_out_cb);
	  		gp_free((void *) jpeg_out_cr);
	  	}

		return STATUS_FAIL;
	}

	p_vlc = jpeg_decode_image_vlc_addr_get();
  	if (((INT32U) p_vlc) >= (img_decode_struct->image_source+img_decode_struct->source_size)) {
  		DBG_PRINT("VLC address exceeds file range\r\n");
  		img_decode_struct->decode_status = STATUS_FAIL;
	  	gp_free((void *) jpeg_out_y);
	  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
	  		gp_free((void *) jpeg_out_cb);
	  		gp_free((void *) jpeg_out_cr);
	  	}

        return STATUS_FAIL;
  	}
	left_len = img_decode_struct->source_size - (((INT32U) p_vlc) - img_decode_struct->image_source);
	jpeg_decode_vlc_maximum_length_set(left_len);

	// Now start JPEG decoding
	if (jpeg_decode_once_start(p_vlc, left_len)) {
		DBG_PRINT("Failed to call jpeg_decode_once_start()\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
	  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
	  		gp_free((void *) jpeg_out_cb);
	  		gp_free((void *) jpeg_out_cr);
	  	}
	  	gp_free((void *) jpeg_out_y);

        return STATUS_FAIL;
  	}

  	// Initiate Scaler
  	scaler_init();
	scaler_done = 0;
	// Setup Scaler
	jpeg_scaler_set_parameters(scaler_fifo_register);
  	if (!scaler_out_buffer) {
		jpeg_decode_stop();
		DBG_PRINT("Failed to allocate scaler_out_buffer\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
	  	gp_free((void *) jpeg_out_y);
	  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
	  		gp_free((void *) jpeg_out_cb);
	  		gp_free((void *) jpeg_out_cr);
	  	}

		return STATUS_FAIL;
  	}

	while (1) {
		jpeg_status = jpeg_decode_status_query(1);

		if (jpeg_status & C_JPG_STATUS_DECODE_DONE) {
		  	// Wait until scaler finish its job
		  	while (!scaler_done) {
		  		scaler_status = scaler_wait_idle();
		  		if (scaler_status == C_SCALER_STATUS_STOP) {
					if (scaler_start()) {
						DBG_PRINT("Failed to call scaler_start\r\n");
						break;
					}
				} else if (scaler_status & C_SCALER_STATUS_DONE) {
					break;
				} else if (scaler_status & (C_SCALER_STATUS_TIMEOUT|C_SCALER_STATUS_INIT_ERR)) {
					DBG_PRINT("Scaler failed to finish its job\r\n");
					break;
				} else if (scaler_status & C_SCALER_STATUS_INPUT_EMPTY) {
		  			if (scaler_restart()) {
						DBG_PRINT("Failed to call scaler_restart\r\n");
						break;
					}
		  		} else {
			  		DBG_PRINT("Un-handled Scaler status!\r\n");
			  		break;
			  	}
		  	}
			break;
		}		// if (jpeg_status & C_JPG_STATUS_DECODE_DONE)

		if (jpeg_status & C_JPG_STATUS_OUTPUT_FULL) {
		  	// Start scaler to handle the full output FIFO now
		  	if (!scaler_done) {
			  	scaler_status = scaler_wait_idle();
			  	if (scaler_status == C_SCALER_STATUS_STOP) {
					if (scaler_start()) {
						DBG_PRINT("Failed to call scaler_start\r\n");
						break;
					}
			  	} else if (scaler_status & C_SCALER_STATUS_DONE) {
		  			// Scaler might finish its job before JPEG does when image is zoomed in.
		  			scaler_done = 1;
				} else if (scaler_status & (C_SCALER_STATUS_TIMEOUT|C_SCALER_STATUS_INIT_ERR)) {
					DBG_PRINT("Scaler failed to finish its job\r\n");
					break;
				} else if (scaler_status & C_SCALER_STATUS_INPUT_EMPTY) {
			  		if (scaler_restart()) {
						DBG_PRINT("Failed to call scaler_restart\r\n");
						break;
					}
			  	} else {
			  		DBG_PRINT("Un-handled Scaler status!\r\n");
			  		break;
			  	}
			}

			// Now restart JPEG to output to next FIFO
	  		if (jpeg_decode_output_restart()) {
	  			DBG_PRINT("Failed to call jpeg_decode_output_restart()\r\n");
	  			break;
	  		}
		}		// if (jpeg_status & C_JPG_STATUS_OUTPUT_FULL)

		if (jpeg_status & C_JPG_STATUS_STOP) {
			DBG_PRINT("JPEG is not started!\r\n");
			break;
		}
		if (jpeg_status & C_JPG_STATUS_TIMEOUT) {
			DBG_PRINT("JPEG execution timeout!\r\n");
			break;
		}
		if (jpeg_status & C_JPG_STATUS_INIT_ERR) {
			DBG_PRINT("JPEG init error!\r\n");
			break;
		}
		if (jpeg_status & C_JPG_STATUS_RST_VLC_DONE) {
			DBG_PRINT("JPEG Restart marker number is incorrect!\r\n");
			jpeg_status = C_JPG_STATUS_DECODE_DONE;
			scaler_status = C_SCALER_STATUS_DONE;
			break;
		}
		if (jpeg_status & C_JPG_STATUS_RST_MARKER_ERR) {
			DBG_PRINT("JPEG Restart marker sequence error!\r\n");
			jpeg_status = C_JPG_STATUS_DECODE_DONE;
			scaler_status = C_SCALER_STATUS_DONE;
			break;
		}
		// Check whether we have to break decoding this image
		/*if (image_task_handle_remove_request(cmd_id) > 0) {
			break;
		}*/
	}

	jpeg_decode_stop();

	scaler_stop();

	gp_free((void *) jpeg_out_y);
	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
  		gp_free((void *) jpeg_out_cb);
  		gp_free((void *) jpeg_out_cr);
  	}

	if ((jpeg_status & C_JPG_STATUS_DECODE_DONE) && (scaler_status & C_SCALER_STATUS_DONE)) {
	    if (!jpeg_output_buffer_pointer) {
	        jpeg_output_buffer_pointer = scaler_out_buffer;
	    }
	    cache_invalid_range(scaler_out_buffer, (jpeg_output_buffer_width*jpeg_output_buffer_height)<<1);
	} else {
		img_decode_struct->decode_status = STATUS_FAIL;
    	if (!jpeg_output_buffer_pointer) {
    	    gp_free((void *) scaler_out_buffer);
    	}

        return STATUS_FAIL;
	}

    img_decode_struct->decode_status = STATUS_OK;
    if (!img_decode_struct->output_buffer_pointer && jpeg_output_buffer_pointer) {
        img_decode_struct->output_buffer_pointer = jpeg_output_buffer_pointer;
    }
    img_decode_struct->output_image_width = jpeg_output_image_width;
    img_decode_struct->output_image_height = jpeg_output_image_height;

	return STATUS_OK;
}

void jpeg_memory_allocate(INT32U fifo)
{
	INT32U jpeg_output_y_size;
	INT32U jpeg_output_cb_cr_size;
	INT16U cbcr_shift;

	if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420) {
		cbcr_shift = 2;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411) {
		cbcr_shift = 2;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV422) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV422V) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV444) {
		cbcr_shift = 0;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_GRAYSCALE) {
		cbcr_shift = 20;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411V) {
		cbcr_shift = 2;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420H2) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420V2) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411H2) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411V2) {
		cbcr_shift = 1;
	} else {
		jpeg_out_y = 0;
		return;
	}

	if (fifo) {
		jpeg_output_y_size = jpeg_extend_width*fifo*2;
	} else {
		jpeg_output_y_size = jpeg_extend_width * jpeg_extend_height;
	}
	jpeg_output_cb_cr_size = jpeg_output_y_size >> cbcr_shift;

  	jpeg_out_y = (INT32U) gp_malloc_align(jpeg_output_y_size, 8);
  	if (!jpeg_out_y) {
  		return;
  	}
  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
  		jpeg_out_cb = (INT32U) gp_malloc_align(jpeg_output_cb_cr_size, 8);
  		if (!jpeg_out_cb) {
  			gp_free((void *) jpeg_out_y);
  			jpeg_out_y = NULL;
  			return;
  		}
  		jpeg_out_cr = (INT32U) gp_malloc_align(jpeg_output_cb_cr_size, 8);
  		if (!jpeg_out_cr) {
  			gp_free((void *) jpeg_out_cb);
  			jpeg_out_cb = NULL;
  			gp_free((void *) jpeg_out_y);
  			jpeg_out_y = NULL;
  			return;
  		}

  	} else {
  		jpeg_out_cb = 0;
  		jpeg_out_cr = 0;

  	}
}

void jpeg_scaler_set_parameters(INT32U fifo)
{
	INT32U factor;
	COLOR_MATRIX_CFG color_matrix;
//	umi_scaler_color_matrix_load();

	scaler_color_matrix_switch(1);
	color_matrix.SCM_A11 = 0x0100;
	color_matrix.SCM_A12 = 0x0000;
	color_matrix.SCM_A13 = 0x0000;
	color_matrix.SCM_A21 = 0x0000;
	color_matrix.SCM_A22 = 0x0180;
	color_matrix.SCM_A23 = 0x0000;
	color_matrix.SCM_A31 = 0x0000;
	color_matrix.SCM_A32 = 0x0000;
	color_matrix.SCM_A33 = 0x0180;
	scaler_color_matrix_config(&color_matrix);

	scaler_input_pixels_set(jpeg_extend_width, jpeg_extend_height);
	scaler_input_visible_pixels_set(jpeg_valid_width, jpeg_valid_height);
	if (jpeg_output_image_width && jpeg_output_image_height) {
	    scaler_output_pixels_set((jpeg_valid_width<<16)/jpeg_output_image_width, (jpeg_valid_height<<16)/jpeg_output_image_height, jpeg_output_buffer_width, jpeg_output_buffer_height);
	} else {
	    if (!jpeg_output_image_width) {
	        jpeg_output_image_width = jpeg_output_buffer_width;
	    }
	    if (!jpeg_output_image_height) {
	        jpeg_output_image_height = jpeg_output_buffer_height;
	    }
    	if (jpeg_output_ratio == 0x0) {      // Fit to output buffer width and height
      		scaler_output_pixels_set((jpeg_valid_width<<16)/jpeg_output_image_width, (jpeg_valid_height<<16)/jpeg_output_image_height, jpeg_output_buffer_width, jpeg_output_buffer_height);
    	} else if (jpeg_output_ratio==2 && jpeg_valid_width<=jpeg_output_image_width && jpeg_valid_height<=jpeg_output_image_height) {
    		scaler_output_pixels_set(1<<16, 1<<16, jpeg_output_buffer_width, jpeg_output_buffer_height);
    		jpeg_output_image_width = jpeg_valid_width;
    		jpeg_output_image_height = jpeg_output_buffer_height;
    	} else {						// Fit to output buffer width or height
      		if (jpeg_output_image_height*jpeg_valid_width > jpeg_output_image_width*jpeg_valid_height) {
      			factor = (jpeg_valid_width<<16)/jpeg_output_image_width;
      			jpeg_output_image_height = (jpeg_valid_height<<16)/factor;
      		} else {
      			factor = (jpeg_valid_height<<16)/jpeg_output_image_height;
      			jpeg_output_image_width = (jpeg_valid_width<<16)/factor;
      		}
      		scaler_output_pixels_set(factor, factor, jpeg_output_buffer_width, jpeg_output_buffer_height);
      	}
    }

	scaler_input_addr_set(jpeg_out_y, jpeg_out_cb, jpeg_out_cr);

	if (jpeg_output_buffer_pointer) {
	    scaler_out_buffer = jpeg_output_buffer_pointer;
	} else {
	    scaler_out_buffer = (INT32U) gp_malloc((jpeg_output_buffer_width*jpeg_output_buffer_height)<<1);
	    if (!scaler_out_buffer) {
		    return;
		}
	}

   	scaler_output_addr_set(scaler_out_buffer, NULL, NULL);
   	scaler_fifo_line_set(fifo);
	scaler_YUV_type_set(C_SCALER_CTRL_TYPE_YCBCR);
	if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV444) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV444);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV422) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV422);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV422V) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV422V);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV420);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV411);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_GRAYSCALE) {
		scaler_input_format_set(C_SCALER_CTRL_IN_Y_ONLY);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411V) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV411V);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420H2) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV422V);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420V2) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV422);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411H2) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV422);
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411V2) {
		scaler_input_format_set(C_SCALER_CTRL_IN_YUV422V);
	}
	scaler_output_format_set(jpeg_output_format);
	scaler_out_of_boundary_color_set(out_of_boundary_color);
}

void ap_state_ir_key_init(void)
{
	INT16U num;
	INT16U i;
	INT8U *key_code;
	INT32U *msg_id;
	
	ap_peripheral_irkey_message_init();
	// Register IR keycode and event mapping to system task
	num = ap_state_resource_ir_key_num_get();
	ap_state_resource_ir_key_map_get(&key_code, &msg_id);
	for (i=0; i<num; i++) {
		ap_peripheral_irkey_message_set(*(key_code+i), *(msg_id+i));
	}
}

INT32S ap_state_common_handling(INT32U msg_id)
{		
	switch(msg_id) {
		case MSG_APQ_AUDIO_EFFECT_OK:
		case MSG_APQ_AUDIO_EFFECT_MENU:
		case MSG_APQ_AUDIO_EFFECT_UP:
		case MSG_APQ_AUDIO_EFFECT_DOWN:
		case MSG_APQ_AUDIO_EFFECT_MODE:
			audio_effect_play(EFFECT_CLICK);
			break;
		default:
			break;
	}
	return 0;
}

