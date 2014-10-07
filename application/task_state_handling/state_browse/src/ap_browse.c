#include "ap_browse.h"

static INT32U browse_output_buff;
static INT8S browse_sts;
static INT8U browse_display_timerid = 0xFF;
static INT16U browse_total_file_num;
static STOR_SERV_PLAYINFO browse_curr_avi;
INT32S pre_audio_state;
INT8U g_browser_reply_action_flag = 0;

//	prototypes
INT32S ap_browse_mjpeg_decode(void);
void ap_browse_display_update_command(void);
void ap_browse_display_timer_start(void);
void ap_browse_display_timer_stop(void);
void ap_browse_show_file_name(INT8U enable);

void ap_browse_init(INT32U prev_state, INT16U play_index)
{
	//INT16U search_type;
	INT32U search_type_with_index, search_type;
	
	if (prev_state == STATE_SETTING) {
		if(play_index == 0xA5A5){
			search_type = STOR_SERV_SEARCH_PREV;	//Daniel modified for returning to previous one after deleting the current one
		} else {
			search_type = STOR_SERV_SEARCH_ORIGIN;	
		}
	} else {
		search_type = STOR_SERV_SEARCH_INIT;
	}
	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);
	ap_state_handling_icon_show_cmd(ICON_PLAYBACK, NULL, NULL);

	/*	//wwj mark
	if(ap_state_handling_night_mode_get()) {	//wwj add
		ap_state_handling_icon_show_cmd(ICON_NIGHT_MODE_ENABLED, NULL, NULL);
		ap_state_handling_icon_clear_cmd(ICON_NIGHT_MODE_DISABLED, NULL, NULL);
	} else {
		ap_state_handling_icon_show_cmd(ICON_NIGHT_MODE_DISABLED, NULL, NULL);
		ap_state_handling_icon_clear_cmd(ICON_NIGHT_MODE_ENABLED, NULL, NULL);
	} */

/*	if(vid_dec_entry() < 0) {
		DBG_PRINT("Failed to init motion jpeg task\r\n");
	}*/
#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
//	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_BAT_STS_REQ, NULL, NULL, MSG_PRI_NORMAL);
	ap_state_handling_current_bat_lvl_show();	//wwj modify
	ap_state_handling_current_charge_icon_show();	//wwj add
#endif		
	browse_output_buff = (INT32U) gp_malloc_align(TFT_WIDTH * TFT_HEIGHT * 2, 64);
	if (!browse_output_buff) {
		DBG_PRINT("State browse allocate jpeg output buffer fail.\r\n");
	}

	browse_sts = 0;
	if (ap_state_handling_storage_id_get() == NO_STORAGE) {
		ap_browse_no_media_show(STR_NO_SD);
		g_browser_reply_action_flag = 1;
		ap_browse_sts_set(BROWSE_UNMOUNT); //wwj add
	} else {
		g_browser_reply_action_flag = 0; //wwj add
		ap_browse_sts_set(~BROWSE_UNMOUNT); //wwj add
		if(prev_state == STATE_THUMBNAIL){
			search_type_with_index = STOR_SERV_SEARCH_GIVEN | (play_index << 16);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_BROWSE_REQ, (void *) &search_type_with_index, sizeof(INT32U), MSG_PRI_NORMAL);
		}else{
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_BROWSE_REQ, (void *) &search_type, sizeof(INT32U), MSG_PRI_NORMAL);
		}
	}
}

void ap_browse_exit(void)
{
	//wwj add
	if (browse_sts == BROWSE_PLAYBACK_BUSY) {
		if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV) {
			audio_wav_pause();
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		} else if(vid_dec_pause() == STATUS_OK){
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		}
	}
	//wwj add end

	#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
	if (browse_sts & BROWSE_PLAYBACK_BUSY) {
		if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV) {
			ap_browse_wav_stop();
		} else {
			ap_browse_mjpeg_stop();
		}
		//Chip_powerup();
	}
	#endif

	gp_free((void *) browse_output_buff);
	browse_output_buff = 0;
	browse_sts = 0;
}

void ap_browse_sts_set(INT8S sts)
{
	if (sts > 0) {
		browse_sts |= sts;
	} else {
		browse_sts &= sts;
	}
}

INT8S ap_browse_sts_get(void)
{
	return 	browse_sts;
}

void ap_browse_func_key_active(void)
{
	if((browse_sts & BROWSE_UNMOUNT)||(browse_curr_avi.file_type == TK_IMAGE_TYPE_JPEG)){
		return;
	}
	ap_state_handling_icon_clear_cmd(ICON_PAUSE, ICON_PLAY, NULL);
	if (!browse_sts) {
		if(browse_total_file_num) {
			ap_browse_sts_set(BROWSE_PLAYBACK_BUSY);
			if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV)
			{
				ap_browse_wav_play();
				ap_state_handling_icon_show_cmd(ICON_PAUSE, NULL, NULL);
			} else if (ap_browse_mjpeg_decode() != STATUS_FAIL) {
				ap_state_handling_icon_show_cmd(ICON_PAUSE, NULL, NULL);
			}
		}
	} else if (browse_sts == BROWSE_PLAYBACK_BUSY) {
		ap_browse_sts_set(BROWSE_PLAYBACK_PAUSE);
		ap_state_handling_icon_show_cmd(ICON_PLAY, NULL, NULL);
		ap_peripheral_auto_off_force_disable_set(1/*0*/); //wwj add
		ap_state_handling_led_on(); //wwj add
		OSTimeDly(5);
		if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV) {
			audio_wav_pause();
		} else if(vid_dec_pause() == STATUS_OK){
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		}
	} else if (browse_sts == (BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE)) {
		ap_browse_sts_set(~BROWSE_PLAYBACK_PAUSE);
		ap_state_handling_icon_show_cmd(ICON_PAUSE, NULL, NULL);
		ap_peripheral_auto_off_force_disable_set(1); //wwj add
		//ap_state_handling_led_blink_on(); //wwj add

		if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV){
			audio_wav_resume();
		} else if(vid_dec_resume() == STATUS_OK) {
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_STOP, NULL, NULL, MSG_PRI_NORMAL);
		}
	}
}

void ap_browse_mjpeg_stop(void)
{
	ap_browse_sts_set(~(BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE));
	ap_peripheral_auto_off_force_disable_set(0); //wwj add
	ap_state_handling_led_on(); //wwj add
	timer_counter_force_display_browser(0);

	vid_dec_stop();
	vid_dec_parser_stop();

	#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
	  #if DV185	//wwj add
		gpio_write_io(SPEAKER_EN, DATA_LOW);
	  #elif DV188
		gpx_rtc_write(8,0x08);
	  #endif
	#endif
}

void ap_browse_mjpeg_play_end(void)
{
	ap_state_handling_icon_clear_cmd(ICON_PAUSE, ICON_PLAY, NULL);
	ap_state_handling_icon_show_cmd(ICON_PLAY, NULL, NULL);
	OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
}

void ap_browse_next_key_active(INT8U err_flag)
{
	/*INT16U*/INT32U search_type = STOR_SERV_SEARCH_NEXT;
	
	if (err_flag) {
		search_type |= (err_flag << 8);
	}
	
	if((browse_sts & BROWSE_UNMOUNT)||(browse_total_file_num == 1)||(browse_total_file_num == 0)) {
		if(browse_sts & BROWSE_PLAYBACK_BUSY) {
			if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV) {
				ap_state_handling_icon_clear_cmd(ICON_PLAY, ICON_PAUSE, NULL);
				ap_state_handling_icon_show_cmd(ICON_PLAY, NULL, NULL);
				ap_browse_wav_stop();
				ap_browse_display_timer_draw();
			} else {
				ap_browse_mjpeg_stop();
	       		ap_browse_mjpeg_play_end();
			}
		}
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		if(!err_flag) {
			audio_effect_play(EFFECT_CLICK);
		}
		return;
	}

	g_browser_reply_action_flag = 0; //wwj add
	if (browse_sts & (BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE)) {
		if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV) {
			ap_browse_wav_stop();
		} else {
			ap_browse_mjpeg_stop();
		}
	}

	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_BROWSE_REQ, (void *) &search_type, sizeof(INT32U), MSG_PRI_NORMAL);
	if(!err_flag) {
		audio_effect_play(EFFECT_CLICK);
	}
}

void ap_browse_prev_key_active(INT8U err_flag)
{
	/*INT16U*/INT32U search_type = STOR_SERV_SEARCH_PREV;
	
	if (err_flag) {
		search_type |= (err_flag << 8);
	}
	
	if((browse_sts & BROWSE_UNMOUNT)||(browse_total_file_num == 1)||(browse_total_file_num == 0)){
		if(browse_sts & BROWSE_PLAYBACK_BUSY) {
			if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV){
				ap_state_handling_icon_clear_cmd(ICON_PLAY, ICON_PAUSE, NULL);
				ap_state_handling_icon_show_cmd(ICON_PLAY, NULL, NULL);
				ap_browse_wav_stop();
				ap_browse_display_timer_draw();
			} else {
				ap_browse_mjpeg_stop();
	       		ap_browse_mjpeg_play_end();
			}
		}
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		if(!err_flag) {
			audio_effect_play(EFFECT_CLICK);
		}
		return;
	}

	g_browser_reply_action_flag = 0; //wwj add
	if (browse_sts & (BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE)) {
		if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV) {
			ap_browse_wav_stop();
		}else{
			ap_browse_mjpeg_stop();
		}
	}

	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_BROWSE_REQ, (void *) &search_type, sizeof(INT32U), MSG_PRI_NORMAL);
	if(!err_flag) {
		audio_effect_play(EFFECT_CLICK);
	}
}

void ap_browse_reply_action(STOR_SERV_PLAYINFO *info_ptr)
{
	INT32S ret, logo_img_ptr;
	IMAGE_DECODE_STRUCT img_info;
	INT32U size;
	INT16U logo_fd;
	struct stat_t buf_tmp;
	
	if (!browse_sts) {
		ap_browse_sts_set(BROWSE_DECODE_BUSY);
		if (info_ptr->err_flag == STOR_SERV_OPEN_OK) {
			browse_total_file_num = info_ptr->total_file_number;
			ap_state_handling_icon_clear_cmd(ICON_PAUSE, ICON_PLAY, NULL);
			browse_curr_avi.file_type = info_ptr->file_type;
			browse_curr_avi.file_path_addr = info_ptr->file_path_addr;
			if (info_ptr->file_type == TK_IMAGE_TYPE_WAV)
			{
				stat((CHAR *)info_ptr->file_path_addr, &buf_tmp);	//check this file is Locked or not
				if(buf_tmp.st_mode & D_RDONLY){
				   	ap_state_handling_icon_show_cmd(ICON_LOCKED, NULL, NULL);
				} else {
				   	ap_state_handling_icon_clear_cmd(ICON_LOCKED, NULL, NULL);
				}

				//for WAV files
				logo_fd = nv_open((INT8U *) "AUDIO_REC_BG.JPG");
				if (logo_fd != 0xFFFF) {
					size = nv_rs_size_get(logo_fd);
					logo_img_ptr = (INT32S) gp_malloc(size);
					if (!logo_img_ptr) {
						DBG_PRINT("State browser allocate jpeg input buffer fail.[%d]\r\n", size);
						return;
					}
					if (nv_read(logo_fd, (INT32U) logo_img_ptr, size)) {
						DBG_PRINT("Failed to read resource_header in ap_browse_reply_action()\r\n");
						gp_free((void *) logo_img_ptr);
						return;
					}
					img_info.image_source = (INT32S) logo_img_ptr;
					img_info.source_size = size;
					img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;
					img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
					img_info.output_ratio = 0;
					img_info.out_of_boundary_color = 0x008080;
					img_info.output_buffer_width = TFT_WIDTH;
					img_info.output_buffer_height = TFT_HEIGHT;
					img_info.output_image_width = TFT_WIDTH;
					img_info.output_image_height = TFT_HEIGHT;
					img_info.output_buffer_pointer = browse_output_buff;
					if (jpeg_buffer_decode_and_scale(&img_info) == STATUS_FAIL) {
						gp_free((void *) logo_img_ptr);
						DBG_PRINT("State browser decode AUDIO_REC_BG.jpeg file fail.\r\n");
						return;
					}
					ap_state_handling_icon_show_cmd(ICON_PLAY, NULL, NULL);
					ap_browse_show_file_name(1);
					ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
					/*
                    if (ap_storage_fast_card_flag_get()==1) {
                        ap_state_handling_icon_clear_cmd(ICON_SD_CARD_RED, ICON_NO_SD_CARD, NULL);
                        ap_state_handling_icon_show_cmd(ICON_SD_CARD_WHITE, NULL, NULL);
                    } else {
                        ap_state_handling_icon_clear_cmd(ICON_SD_CARD_WHITE, ICON_NO_SD_CARD, NULL);
                        ap_state_handling_icon_show_cmd(ICON_SD_CARD_RED, NULL, NULL);
                    } */
					OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
					gp_free((void *) logo_img_ptr);
				}
				close(info_ptr->file_handle);
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
			}
			else	//for AVI and JPG files
			{
				ret = ap_state_handling_jpeg_decode(info_ptr, browse_output_buff);
				if (ret == STATUS_OK) {
					stat((CHAR *)info_ptr->file_path_addr, &buf_tmp);	//check this file is Locked or not
					if(buf_tmp.st_mode & D_RDONLY){
					   	ap_state_handling_icon_show_cmd(ICON_LOCKED, NULL, NULL);
					} else {
					   	ap_state_handling_icon_clear_cmd(ICON_LOCKED, NULL, NULL);
					}
					if (info_ptr->file_type != TK_IMAGE_TYPE_JPEG) {
						ap_state_handling_icon_show_cmd(ICON_PLAY, NULL, NULL);
					}
					ap_browse_show_file_name(1);
					ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
					OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
				} else {
					INT8U err = 1;
					
					if (browse_total_file_num == 1) {
						browse_total_file_num = 0; //wwj add
						ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
						ap_browse_no_media_show(STR_NO_MEDIA);
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
					} else {
						if (info_ptr->search_type == STOR_SERV_SEARCH_NEXT) {
							msgQSend(ApQ, MSG_APQ_NEXT_KEY_ACTIVE, &err, sizeof(INT8U), MSG_PRI_NORMAL);
						} else {
							msgQSend(ApQ, MSG_APQ_PREV_KEY_ACTIVE, &err, sizeof(INT8U), MSG_PRI_NORMAL);
						}
					}
					close(info_ptr->file_handle);
					DBG_PRINT("[SKIP]State browse decode file fail.\r\n");
				}
			}
		} else if (info_ptr->err_flag == STOR_SERV_NO_MEDIA) {
			browse_total_file_num = 0; //wwj add
			ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
			ap_browse_no_media_show(STR_NO_MEDIA);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL); //wwj add
		} else {
			browse_total_file_num = 0; //wwj add
			if (ap_state_handling_storage_id_get() == NO_STORAGE) {
				ap_browse_no_media_show(STR_NO_SD);
			} else {
				ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
				ap_browse_no_media_show(STR_NO_MEDIA);
				if (info_ptr->err_flag == STOR_SERV_DECODE_ALL_FAIL) {
					close(info_ptr->file_handle);
				}
			}
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		}
		ap_browse_sts_set(~BROWSE_DECODE_BUSY);
	}
}

void ap_browse_show_file_name(INT8U enable)
{
	if(enable){
		STRING_ASCII_INFO ascii_str;

		ascii_str.font_color = BROWSE_FILE_NAME_COLOR;
		ascii_str.font_type = 0;
		ascii_str.pos_x = BROWSE_FILE_NAME_START_X;
		ascii_str.pos_y = BROWSE_FILE_NAME_START_Y;
		ascii_str.str_ptr = (CHAR *) browse_curr_avi.file_path_addr;
		ascii_str.buff_w = TFT_WIDTH;
		ascii_str.buff_h = TFT_HEIGHT;
		ap_state_resource_string_ascii_draw((INT16U *) browse_output_buff, &ascii_str);	
	} else {
		INT32S logo_img_ptr;
		IMAGE_DECODE_STRUCT img_info;
		INT32U size;
		INT16U logo_fd;

		logo_fd = nv_open((INT8U *) "AUDIO_REC_BG.JPG");
		if (logo_fd != 0xFFFF) {
			size = nv_rs_size_get(logo_fd);
			logo_img_ptr = (INT32S) gp_malloc(size);
			if (!logo_img_ptr) {
				DBG_PRINT("State browser allocate jpeg input buffer fail.[%d]\r\n", size);
				return;
			}
			if (nv_read(logo_fd, (INT32U) logo_img_ptr, size)) {
				DBG_PRINT("Failed to read resource_header in ap_browse_reply_action()\r\n");
				gp_free((void *) logo_img_ptr);
				return;
			}
			img_info.image_source = (INT32S) logo_img_ptr;
			img_info.source_size = size;
			img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;
			img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
			img_info.output_ratio = 0;
			img_info.out_of_boundary_color = 0x008080;
			img_info.output_buffer_width = TFT_WIDTH;
			img_info.output_buffer_height = TFT_HEIGHT;
			img_info.output_image_width = TFT_WIDTH;
			img_info.output_image_height = TFT_HEIGHT;
			img_info.output_buffer_pointer = browse_output_buff;
			if (jpeg_buffer_decode_and_scale(&img_info) == STATUS_FAIL) {
				gp_free((void *) logo_img_ptr);
				DBG_PRINT("State browser decode AUDIO_REC_BG.jpeg file fail.\r\n");
				return;
			}
			gp_free((void *) logo_img_ptr);
		}

		/*
		ascii_str.font_color = BROWSE_FILE_NAME_COLOR_BLACK;
		ascii_str.font_type = 0;
		ascii_str.pos_x = BROWSE_FILE_NAME_START_X;
		ascii_str.pos_y = BROWSE_FILE_NAME_START_Y;
		ascii_str.str_ptr = (CHAR *) browse_curr_avi.file_path_addr;
		ascii_str.buff_w = TFT_WIDTH;
		ascii_str.buff_h = TFT_HEIGHT;
		ap_state_resource_string_ascii_draw((INT16U *) browse_output_buff, &ascii_str);
		*/
	}
}

void ap_browse_wav_play(void)
{
	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_STOP, NULL, NULL, MSG_PRI_NORMAL);

	browse_curr_avi.file_handle = open((CHAR *) browse_curr_avi.file_path_addr, O_RDONLY);
	if (browse_curr_avi.file_handle >= 0) {

		#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
		  #if DV185	//wwj add
			gpio_write_io(SPEAKER_EN, 1);
		  #elif DV188
			gpx_rtc_write(8,0x28);		//lqy 2011.11.25
		  #endif
		//Chip_powerdown();
  		#endif

		audio_wav_play(browse_curr_avi.file_handle);

		timer_counter_force_display_browser(1);	//wwj add
		ap_peripheral_auto_off_force_disable_set(1); //wwj add
		//ap_state_handling_led_blink_on();//wwj add
		ap_browse_display_timer_start();
		ap_browse_show_file_name(0);
	} else {
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	}
}


void ap_browse_wav_stop(void)
{
	ap_browse_sts_set(~(BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE));
	ap_peripheral_auto_off_force_disable_set(0); //wwj add
	ap_state_handling_led_on(); //wwj add
	ap_browse_display_timer_stop();
	timer_counter_force_display_browser(0);
	ap_browse_show_file_name(1);

	audio_wav_stop();
	//close(browse_curr_avi.file_handle); //wwj mark

	#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
	  #if DV185	//wwj add
		gpio_write_io(SPEAKER_EN, DATA_LOW);
	  #elif DV188
		gpx_rtc_write(8,0x08);
	  #endif
	#endif
}

INT8S ap_browse_stop_handle(void)
{
	INT8S browse_sts_tmp;

	browse_sts_tmp = browse_sts;
	if (browse_sts & (BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE)) {
		if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV){
			ap_browse_wav_stop();
		} else {
			ap_browse_mjpeg_stop();
		}
		ap_state_handling_icon_clear_cmd(ICON_PAUSE, ICON_PLAY, NULL);
	} else {
		ap_state_handling_icon_clear_cmd(ICON_PLAY, NULL, NULL);
	}
	return browse_sts_tmp;
}

void ap_browse_connect_to_pc(void)
{
	if (ap_browse_stop_handle() & (BROWSE_PLAYBACK_BUSY | BROWSE_PLAYBACK_PAUSE)) {
		ap_state_handling_icon_show_cmd(ICON_PLAY, NULL, NULL);
		OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
	} else {
		OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
	}
}

void ap_browse_disconnect_to_pc(void)
{
	OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
}

void ap_browse_display_update_command(void)
{
//	if((browse_sts == 0) || (browse_sts & BROWSE_UNMOUNT) ){
	if((browse_sts == 0) || (browse_sts & BROWSE_UNMOUNT) || (browse_sts & BROWSE_PLAYBACK_PAUSE)) {	//wwj modify
		if(browse_sts & BROWSE_PLAYBACK_PAUSE) {
			INT32U tft_buff;
			tft_buff = R_TFT_FBI_ADDR;
			OSQPost(DisplayTaskQ, (void *) (tft_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
		} else {
			OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
		}
	}
}

void ap_browse_no_media_show(INT16U str_type)
{
	INT32U i, *buff_ptr, color_data, cnt;

	buff_ptr = (INT32U *) browse_output_buff;
	color_data = 0x11a4 | (0x11a4<<16);
	cnt = (TFT_WIDTH * TFT_HEIGHT * 2) >> 2;
	for (i=0 ; i<cnt ; i++) {
		*buff_ptr++ = color_data;
	}
	if (str_type == STR_NO_SD) {
		ap_state_handling_icon_show_cmd(ICON_NO_SD_CARD, NULL, NULL);
	}
#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
//	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_BAT_STS_REQ, NULL, NULL, MSG_PRI_NORMAL);
	ap_state_handling_current_bat_lvl_show();	//wwj modify
	ap_state_handling_current_charge_icon_show();	//wwj add
#endif
	ap_state_handling_str_draw_exit();
	ap_state_handling_str_draw(str_type, 0xF800);
	ap_state_handling_icon_clear_cmd(ICON_PLAY, NULL, NULL); //wwj add
   	ap_state_handling_icon_clear_cmd(ICON_LOCKED, NULL, NULL); //wwj add
	OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
}


void ap_browse_display_timer_start(void)
{
	if (browse_display_timerid == 0xFF) {
		browse_display_timerid = VIDEO_RECORD_CYCLE_TIMER_ID;
		sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_DISPLAY_DRAW_TIMER, browse_display_timerid, BROWSE_DISPLAY_TIMER_INTERVAL);
	}
}

void ap_browse_display_timer_stop(void)
{
	if (browse_display_timerid != 0xFF) {
		sys_kill_timer(browse_display_timerid);
		browse_display_timerid = 0xFF;
	}
}

void ap_browse_display_timer_draw(void)
{
	if(browse_curr_avi.file_type == TK_IMAGE_TYPE_WAV){
		OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_WAV_TIME_DRAW));
	} else {
		OSQPost(DisplayTaskQ, (void *) (browse_output_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
	}
}

INT32S ap_browse_mjpeg_decode(void)
{
	INT32S ret = 0;

	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_STOP, NULL, NULL, MSG_PRI_NORMAL);

	browse_curr_avi.file_handle = open((CHAR *) browse_curr_avi.file_path_addr, O_RDONLY);
	if (browse_curr_avi.file_handle >= 0) {
		if (vid_dec_parser_start(browse_curr_avi.file_handle, 1, NULL)) {
			ret = STATUS_FAIL;
		}
		if (ret != STATUS_FAIL) {
			if (audio_playing_state_get() == STATE_PLAY) {
				audio_send_pause();
				OSTimeDly(2*MAX_DAC_BUFFERS);
			}
	  	}

		vid_dec_set_scaler(1, C_SCALER_CTRL_OUT_RGB565, TFT_WIDTH, TFT_HEIGHT, TFT_WIDTH, TFT_HEIGHT);
		vid_dec_set_user_define_buffer(0, 0, 0);
		if (vid_dec_start()) {
			ret = STATUS_FAIL;
		}
		if (ret == STATUS_FAIL) {
			ap_browse_mjpeg_stop();
			if ( (audio_playing_state_get() == STATE_IDLE) || (audio_playing_state_get() == STATE_PAUSED) ){
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
			}
		} else {
			#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
			  #if DV185	//wwj add
				gpio_write_io(SPEAKER_EN, 1);
			  #elif DV188
				gpx_rtc_write(8,0x28);
			  #endif
			//Chip_powerdown();
	  		#endif

			timer_counter_force_display_browser(1);	//wwj add
			ap_peripheral_auto_off_force_disable_set(1); //wwj add
			//ap_state_handling_led_blink_on();//wwj add
		}
	} else {
		if ( (audio_playing_state_get() == STATE_IDLE) || (audio_playing_state_get() == STATE_PAUSED) ){
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		}
	}
	return ret;
}