#include "ap_setting.h"

static SETTINGFUNC setting_category_set_fptr[4];
static SETUP_ITEM_INFO setting_item;
static INT32U setting_frame_buff;
static INT32U setting_browse_frame_buff = 0;
static INT16U state_str;
static char dt_str[] = "2000 / 00 / 00   00 : 00";
static INT8U setup_date_time[6];
static INT8U setup_date_time_counter = 0;
static INT8U g_setting_delete_choice = 0;
static INT8U g_setting_lock_choice = 0;

INT8U video_item[]={STR_SIZE1,STR_VIDEO_TIME,STR_MIC_SWITCH};
INT8U photo_item[]={STR_SIZE};
INT8U browse_item[]={STR_DELETE,STR_LOCK,STR_THUMBNAIL,STR_VOLUME};
INT8U basic_item[]= {STR_FORMAT,STR_LANGUAGE,STR_SYS_RESET,STR_LIGHT_FREQ,STR_DATE_INPUT,STR_AUTO_OFF,STR_APP_VERSION};
//	prototypes
void ap_setting_page_draw(INT32U state, INT8U *tag);
void ap_setting_sub_menu_draw(INT8U curr_tag);
void ap_setting_background_str_draw(STRING_INFO *str, INT8U flag);
void ap_setting_video_record_set(void);
void ap_setting_photo_capture_set(void);
void ap_setting_browse_set(void);
void ap_setting_basic_set(void);
void ap_setting_general_state_draw(STRING_INFO *str, INT8U *tag);
INT8U ap_setting_right_menu_active(STRING_INFO *str, INT8U type, INT8U *sub_tag);
void ap_setting_format_busy_show(void);
void ap_setting_icon_draw(INT16U *frame_buff, INT16U *icon_stream, DISPLAY_ICONSHOW *icon_info, INT8U type);
void ap_setting_date_time_string_process(INT8U dt_tag);
void ap_setting_frame_buff_display(void);
INT8U CalendarCalculateDays(INT8U month, INT8U year);
void ap_setting_value_set_from_user_config(void);
void ap_setting_sensor_sccb_cmd_set(INT8U *table_ptr, INT16U idx, INT8U cmd_num);
void ap_setting_sensor_command_switch(INT16U cmd_addr, INT16U reg_bit, INT8U enable);
void ap_setting_show_software_version(void);

void ap_setting_init(INT32U prev_state, INT8U *tag)
{		
	IMAGE_DECODE_STRUCT img_info;
	INT32U background_image_ptr;
	INT32U size;
	INT16U logo_fd;

	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);
	setting_frame_buff = (INT32U) gp_malloc_align(TFT_WIDTH * TFT_HEIGHT * 2, 64);
	if (!setting_frame_buff) {
		DBG_PRINT("State setting allocate frame buffer fail.\r\n");
	}
	if (prev_state == STATE_BROWSE) {
		/*INT16U*/INT32U search_type = STOR_SERV_SEARCH_ORIGIN;
		setting_browse_frame_buff = (INT32U) gp_malloc_align(TFT_WIDTH * TFT_HEIGHT * 2, 64);
		if (!setting_browse_frame_buff) {
			DBG_PRINT("State setting allocate browse frame buffer fail.\r\n");
		} else if (ap_state_handling_storage_id_get() == NO_STORAGE) {
			STOR_SERV_PLAYINFO dummy_info = {0};
			dummy_info.err_flag = STOR_SERV_NO_MEDIA;
			dummy_info.file_handle = -1;
//			msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, NULL, NULL, MSG_PRI_NORMAL); //to give a null pointer is incorrect, wwj commnet
			msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, &dummy_info, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL); //wwj modify

		} else {
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_BROWSE_REQ, (void *) &search_type, sizeof(/*INT16U*/INT32U), MSG_PRI_NORMAL);
		}
	} else if(prev_state == STATE_AUDIO_RECORD) {
		setting_browse_frame_buff = (INT32U) gp_malloc_align(TFT_WIDTH * TFT_HEIGHT * 2, 64);
		if (!setting_browse_frame_buff) {
			DBG_PRINT("State setting allocate browse frame buffer fail.\r\n");
		}
		logo_fd = nv_open((INT8U *) "AUDIO_REC_BG.JPG");
		if (logo_fd != 0xFFFF) {
			size = nv_rs_size_get(logo_fd);
			background_image_ptr = (INT32U) gp_malloc(size);
			if (!background_image_ptr) {
				DBG_PRINT("State audio record allocate jpeg input buffer fail.[%d]\r\n", size);
			} else {
				if (nv_read(logo_fd, background_image_ptr, size)) {
					DBG_PRINT("Failed to read resource_header in ap_audio_record_init\r\n");
				}
				img_info.image_source = (INT32S) background_image_ptr;
				img_info.source_size = size;
				img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;
				img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
				img_info.output_ratio = 0;
				img_info.out_of_boundary_color = 0x008080;
				img_info.output_buffer_width = TFT_WIDTH;
				img_info.output_buffer_height = TFT_HEIGHT;
				img_info.output_image_width = TFT_WIDTH;
				img_info.output_image_height = TFT_HEIGHT;
				img_info.output_buffer_pointer = setting_browse_frame_buff;
				if (jpeg_buffer_decode_and_scale(&img_info) == STATUS_FAIL) {
					DBG_PRINT("State audio record decode jpeg file fail.\r\n");
				}
				gp_free((void *) background_image_ptr);
			}
		} else {
			gp_memset((void *) setting_browse_frame_buff, 80, TFT_WIDTH * TFT_HEIGHT * 2);
		}
	}
	setting_category_set_fptr[0] = (SETTINGFUNC) ap_setting_video_record_set;
	setting_category_set_fptr[1] = (SETTINGFUNC) ap_setting_photo_capture_set;
	setting_category_set_fptr[2] = (SETTINGFUNC) ap_setting_browse_set;
	setting_category_set_fptr[3] = (SETTINGFUNC) ap_setting_basic_set;
	if (prev_state != STATE_BROWSE) {
		ap_setting_page_draw(prev_state, tag);
	}
	setup_date_time_counter = 0;
}

void ap_setting_exit(void)
{
	OS_Q_DATA OSQData;

	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_SETTING_EXIT);
	OSQPost(scaler_task_q,(void*)MSG_SCALER_TASK_UPDATE_DISP_MODE);
	OSQQuery(DisplayTaskQ, &OSQData);
	while(OSQData.OSMsg != NULL) {
		OSTimeDly(3);
		OSQQuery(DisplayTaskQ, &OSQData);
	}

	if (setting_browse_frame_buff) {
		gp_free((void *) setting_browse_frame_buff);
		setting_browse_frame_buff = 0;
	}
	if (setting_frame_buff) {
		gp_free((void *) setting_frame_buff);
		setting_frame_buff = 0;
	}
	setup_date_time_counter = 0;
	g_setting_delete_choice = 0;
	g_setting_lock_choice = 0;
}

void ap_setting_reply_action(INT32U state, INT8U *tag, STOR_SERV_PLAYINFO *info_ptr)
{
	INT32U i, *buff_ptr, color_data, cnt;
	INT32S ret, logo_img_ptr;
	IMAGE_DECODE_STRUCT img_info;
	INT32U size;
	INT16U logo_fd;

	if ((info_ptr->err_flag == STOR_SERV_OPEN_OK) && (ap_state_handling_storage_id_get() != NO_STORAGE)) {
		if (info_ptr->file_type == TK_IMAGE_TYPE_WAV) {
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
					DBG_PRINT("Failed to read resource_header in ap_setting_reply_action\r\n");
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
				img_info.output_buffer_pointer = setting_browse_frame_buff;
				if (jpeg_buffer_decode_and_scale(&img_info) == STATUS_FAIL) {
					gp_free((void *) logo_img_ptr);
					DBG_PRINT("State browser decode AUDIO_REC_BG.jpeg file fail.\r\n");
					return;
				}
				gp_free((void *) logo_img_ptr);
				close(info_ptr->file_handle);
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
			} else {
				DBG_PRINT("Failed to nv_open browser decode AUDIO_REC_BG.jpg!\r\n");
			}
		}
		else	//for AVI and JPG files
		{
			ret = ap_state_handling_jpeg_decode(info_ptr, setting_browse_frame_buff);
			if (ret != STATUS_OK) {
				close(info_ptr->file_handle);

				//wwj add
				buff_ptr = (INT32U *) setting_browse_frame_buff;
				color_data = 0x11a4 | (0x11a4<<16);
				cnt = (TFT_WIDTH * TFT_HEIGHT * 2) >> 2;
				for (i=0 ; i<cnt ; i++) {
					*buff_ptr++ = color_data;
				}
			}
		}
	} else {
		buff_ptr = (INT32U *) setting_browse_frame_buff;
		color_data = 0x11a4 | (0x11a4<<16);
		cnt = (TFT_WIDTH * TFT_HEIGHT * 2) >> 2;
		for (i=0 ; i<cnt ; i++) {
			*buff_ptr++ = color_data;
		}
//		ap_state_handling_str_draw_exit();
//		ap_state_handling_str_draw(STR_NO_MEDIA, 0xF800);
		if (info_ptr->err_flag == STOR_SERV_DECODE_ALL_FAIL) {
			close(info_ptr->file_handle);
		}
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	}
	ap_setting_page_draw(state, tag);
}

void ap_setting_page_draw(INT32U state, INT8U *tag)
{
	DISPLAY_ICONSHOW icon = {320, 234, TRANSPARENT_COLOR, 0, 0};
	STRING_INFO str;
	
	if (state != STATE_SETTING) {
		ap_setting_icon_draw((INT16U *)setting_frame_buff, background_a, &icon, SETTING_ICON_NORMAL_DRAW);
	} else {
		ap_setting_icon_draw((INT16U *)setting_frame_buff, background_b, &icon, SETTING_ICON_NORMAL_DRAW);
	}	
	icon.icon_h = ICON_SELECT_BAR_HEIGHT;
	icon.pos_y = ICON_SELECT_BAR_START_Y + (*tag - ((*tag/5)*5))*27;
	ap_setting_icon_draw((INT16U *)setting_frame_buff, select_bar_gb, &icon, SETTING_ICON_NORMAL_DRAW);
	str.font_type = 0;	
	str.buff_w = TFT_WIDTH;
	str.buff_h = TFT_HEIGHT;
	str.language = ap_state_config_language_get() - 1;	
	str.font_color = 0xFFFF;
	ap_setting_background_str_draw(&str, 1);
	if(state == STATE_AUDIO_RECORD) {
		setting_category_set_fptr[STATE_VIDEO_PREVIEW & 0xF](); //wwj add
	} else {
		setting_category_set_fptr[state & 0xF]();
	}
	ap_setting_general_state_draw(&str, tag);
	ap_setting_frame_buff_display();
}

void ap_setting_sub_menu_draw(INT8U curr_tag)
{
	STRING_INFO sub_str;
	DISPLAY_ICONSHOW icon;
	INT16U draw_cnt, i;

	icon.transparent = TRANSPARENT_COLOR;
	icon.icon_w = 184;
	icon.icon_h = 120;
	icon.pos_x = (TFT_WIDTH - icon.icon_w) >> 1;	
	icon.pos_y = (TFT_HEIGHT - icon.icon_h) >> 1;
	ap_setting_icon_draw((INT16U *)setting_frame_buff, sub_menu_4, &icon, SETTING_ICON_NORMAL_DRAW);
	icon.icon_h = ICON_SELECT_BAR_HEIGHT;
	i = curr_tag & 0x3;
	if (i) {
		icon.pos_y += (i * (icon.icon_h - i));
	} else {
		icon.pos_y += 2;
	}
	ap_setting_icon_draw((INT16U *)setting_frame_buff, select_bar_gs, &icon, SETTING_ICON_NORMAL_DRAW);
	sub_str.font_type = 0;	
	sub_str.buff_w = TFT_WIDTH;
	sub_str.buff_h = TFT_HEIGHT;
	sub_str.font_color = 0xFFFF;
	sub_str.pos_x = 75;	
	sub_str.pos_y = 70;
	if (setting_item.sub_item_start != STR_MULTI_LANGUAGE) {
		sub_str.language = ap_state_config_language_get() - 1;
		sub_str.str_idx = ((curr_tag>>2) << 2) + setting_item.sub_item_start;
		draw_cnt = setting_item.sub_item_max - (sub_str.str_idx - setting_item.sub_item_start);
	} else {
		sub_str.str_idx = STR_MULTI_LANGUAGE;
		sub_str.language = ((curr_tag>>2) << 2) + LCD_EN;
		draw_cnt = setting_item.sub_item_max - (sub_str.language - setting_item.sub_item_start);
	}	
	if (draw_cnt > 4) {
		draw_cnt = 4;
	}

	if(sub_str.str_idx == STR_4_MIN) {	//wwj add
		sub_str.str_idx = STR_10_MIN;
	}

	for (i=0; i<draw_cnt; i++) {
		ap_state_resource_string_draw((INT16U *)setting_frame_buff, &sub_str);
		if (setting_item.sub_item_start != STR_MULTI_LANGUAGE) {
			sub_str.str_idx++;

			//wwj add
			if(sub_str.str_idx == STR_2_MIN) {
				sub_str.str_idx = STR_3_MIN;
			} else if (sub_str.str_idx == STR_4_MIN) {
				sub_str.str_idx = STR_5_MIN;
			}
			//wwj add end

		} else {
			sub_str.language++;
		}
		sub_str.pos_x = 75;
		sub_str.pos_y += 28;
	}
	ap_setting_frame_buff_display();
}

void ap_setting_date_time_menu_draw(INT8U tag)
{
	STRING_INFO str;
	STRING_ASCII_INFO ascii_str;
	DISPLAY_ICONSHOW cursor_icon = {320, 234, TRANSPARENT_COLOR, 0, 0};
	INT8U cursor_offset_x[] = {0, 67, 122, 178, 232};
//	INT8U cursor_offset_x[] = {0, 59, 110, 159, 210}; //wwj modify
	
	ap_setting_icon_draw((INT16U *)setting_frame_buff, background_c, &cursor_icon, SETTING_ICON_NORMAL_DRAW);
	str.font_type = 0;	
	str.buff_w = TFT_WIDTH;
	str.buff_h = TFT_HEIGHT;
	str.language = ap_state_config_language_get() - 1;	
	str.font_color = 0xFFFF;
	ap_setting_background_str_draw(&str, 0);
	ap_setting_date_time_string_process(SETTING_DATE_TIME_DRAW_ALL);
	ascii_str.font_color = 0xFFFF;
	ascii_str.font_type = 0;
	ascii_str.buff_w = TFT_WIDTH;
	ascii_str.buff_h = TFT_HEIGHT;
	ascii_str.pos_x = 28;
	ascii_str.pos_y = 115;
	ascii_str.str_ptr = dt_str;
	ap_state_resource_string_ascii_draw((INT16U *)setting_frame_buff, &ascii_str);
	cursor_icon.icon_w = 16;
	cursor_icon.icon_h = 16;
	cursor_icon.pos_x = 40 + cursor_offset_x[tag];
	cursor_icon.pos_y = 75;
	ap_setting_icon_draw((INT16U *)setting_frame_buff, ui_up, &cursor_icon, SETTING_ICON_NORMAL_DRAW);
	cursor_icon.pos_y += 80;
	ap_setting_icon_draw((INT16U *)setting_frame_buff, ui_down, &cursor_icon, SETTING_ICON_NORMAL_DRAW);
	ap_setting_frame_buff_display();
}

void ap_setting_background_str_draw(STRING_INFO *str, INT8U flag)
{
	t_STRING_TABLE_STRUCT str_res;
	
	str->str_idx = STR_EXIT;
	ap_state_resource_string_resolution_get(str, &str_res);
	str->pos_x = (160 - str_res.string_width) >> 1;
	str->pos_y = 198;
	ap_state_resource_string_draw((INT16U *)setting_frame_buff, str);	
	str->str_idx = STR_SETUP;
	ap_state_resource_string_resolution_get(str, &str_res);
	str->pos_x = ((160 - str_res.string_width) >> 1) + 170;
	ap_state_resource_string_draw((INT16U *)setting_frame_buff, str);
	if (flag) {
		str->pos_x -= str_res.string_width + 10;
		str->pos_y = 23;
		ap_state_resource_string_draw((INT16U *)setting_frame_buff, str);
	} else {
		str->str_idx = STR_DATE_INPUT;
		ap_state_resource_string_resolution_get(str, &str_res);
		str->pos_x = ((TFT_WIDTH - str_res.string_width) >> 1);
		str->pos_y = 20;
		ap_state_resource_string_draw((INT16U *)setting_frame_buff, str);
	}
}

void ap_setting_video_record_set(void)
{
	
	setting_item.item =video_item; /*STR_EV*/	//wwj modify
	setting_item.item_max = sizeof(video_item)/sizeof(char);		//wwj modify
	state_str = STR_VIDEO_OUTPUT;
}

void ap_setting_photo_capture_set(void)
{	

	setting_item.item =photo_item; /*STR_QUALITY*/	//wwj modify
	setting_item.item_max = sizeof(photo_item)/sizeof(char)/*7*/;	//wwj modify
	state_str = STR_CAPTURE;
}

void ap_setting_browse_set(void)
{
	
	setting_item.item = browse_item;
	setting_item.item_max = sizeof(browse_item)/sizeof(char);
	state_str = STR_PLAY;
}

void ap_setting_basic_set(void)
{
	
	setting_item.item =basic_item;
	setting_item.item_max = sizeof(basic_item)/sizeof(char);
}

void ap_setting_general_state_draw(STRING_INFO *str, INT8U *tag)
{
	t_STRING_TABLE_STRUCT str_res;
	INT16U draw_cnt, i, str_tmp;
	INT8U start_index;

	start_index = ((*tag/5)*5);
	str->str_idx = state_str;
	ap_state_resource_string_resolution_get(str, &str_res);
	str->pos_x = (160 - str_res.string_width) >> 1;
	str->pos_y = 23;
	ap_state_resource_string_draw((INT16U *)setting_frame_buff, str);	
	
	draw_cnt = setting_item.item_max - start_index;
	if (draw_cnt > 5) {
		draw_cnt = 5;
	}

	/*
	if(str->str_idx == STR_DATE_INPUT) {	//wwj add
		str->str_idx += 1;
	} */

	for (i=0; i<draw_cnt; i++) {
		str->pos_x = 13;
		str->pos_y = 57+i*27;
		str->str_idx = setting_item.item[start_index+i];
		//DBG_PRINT("String idx:%d\r\n",str->str_idx);
		if(str->str_idx != STR_APP_VERSION){
			ap_state_resource_string_draw((INT16U *)setting_frame_buff, str);
		}else{
			STRING_ASCII_INFO ascii_str;
			ascii_str.buff_w = TFT_WIDTH;
			ascii_str.buff_h = TFT_HEIGHT;
			ascii_str.str_ptr = "Version";
			ascii_str.font_color = 0xFFFF;
			ascii_str.font_type = 0;
			ascii_str.pos_x = str->pos_x;
			ascii_str.pos_y = str->pos_y;		
			ap_state_resource_string_ascii_draw((INT16U *) setting_frame_buff, &ascii_str);	
			
		}
		str->pos_x = 163;
		ap_setting_right_menu_active(str, STRING_DRAW, NULL);
	}
}

INT8U ap_setting_right_menu_active(STRING_INFO *str, INT8U type, INT8U *sub_tag)
{
	TIME_T	g_time;
	INT8U curr_tag;

	switch (str->str_idx) {
		//case STR_EV:	//wwj mark
		case STR_EV1:
			curr_tag = ap_state_config_ev_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_P2_0;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 13;
				setting_item.sub_item_start = STR_P2_0;
			} else if (type == SUB_MENU_MOVE) {
				ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_EV_data, str->font_color, sizeof(ap_setting_EV_data)/setting_item.sub_item_max);
			} else {
				ap_state_config_ev_set(str->font_color);
			}
			break;
		case STR_WHITE_BALANCE:
		case STR_WHITE_BALANCE1:
			curr_tag = ap_state_config_white_balance_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_AUTO;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 5;
				setting_item.sub_item_start = STR_AUTO;
			} else if (type == SUB_MENU_MOVE) {
				if(str->font_color == 0){
					ap_setting_sensor_command_switch(0x13, 0x02, 1);
				}else{
					ap_setting_sensor_command_switch(0x13, 0x02, 0);
				}				
				ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_White_Balance, str->font_color, sizeof(ap_setting_White_Balance)/setting_item.sub_item_max);
			} else {
				ap_state_config_white_balance_set(str->font_color);
			}
			break;
		case STR_TIME_STAMP:
			curr_tag = ap_state_config_date_stamp_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_OFF;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_OFF;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_date_stamp_set(str->font_color);
			}
			break;
		case STR_MOTIONDETECT:
			curr_tag = ap_state_config_md_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_OFF;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_OFF;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_md_set(str->font_color);
			}
			break;
		case STR_VIDEO_TIME:
			curr_tag = ap_state_config_record_time_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_OFF2;

				//wwj add
				if(str->str_idx == STR_2_MIN) {
					str->str_idx = STR_3_MIN;
				} else if (str->str_idx == STR_3_MIN) {
					str->str_idx = STR_5_MIN;
				} else if (str->str_idx == STR_4_MIN) {
					str->str_idx = STR_10_MIN;
				} 
				//wwj add end

			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 5;//6;	//wwj modify
				setting_item.sub_item_start = STR_OFF2;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_record_time_set(str->font_color);
				/*
				if(str->font_color==0) {
                    bkground_del_disable(1);
                } else {
                	bkground_del_disable(0);
                } */
                ap_storage_service_del_thread_mb_set();
			}
			break;
		
		/*case STR_NIGHT_MODE: */
		case STR_MIC_SWITCH:	//wwj modify
			//curr_tag = ap_state_config_night_mode_get();
			curr_tag = ap_state_config_voice_record_switch_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_ON/*STR_OFF*/;	//wwj modify
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_ON;//STR_OFF;	//wwj modify
			} else if (type == SUB_MENU_MOVE) {
				/*	//wwj modify
				 if(str->font_color == 0){
					gpio_write_io(IR_CTRL, DATA_HIGH);
				} else if (str->font_color == 1) {
					gpio_write_io(IR_CTRL, DATA_LOW);
				} */
			} else if (type == FUNCTION_ACTIVE) {
//				ap_state_config_night_mode_set(str->font_color);
				ap_state_config_voice_record_switch_set(str->font_color);	//wwj modify
			}
			break;
		case STR_SIZE:
		case STR_SIZE1:	//marty add
			curr_tag = ap_state_config_pic_size_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_D_VGA/*STR_5M*/;		//marty modify
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = /*5*/3;			//marty modify
				setting_item.sub_item_start = STR_D_VGA/*STR_5M*/;	//marty modify
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_pic_size_set(str->font_color);
			}
			break;
		case STR_QUALITY:
			curr_tag = ap_state_config_quality_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_FINE;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 3;
				setting_item.sub_item_start = STR_FINE;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_quality_set(str->font_color);
			}
			break;
/*		case STR_SCENE_MODE:
			curr_tag = ap_state_config_scene_mode_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_AUTO1;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_AUTO1;
			} else if (type == SUB_MENU_MOVE) {
				ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Scene_Mode, str->font_color, sizeof(ap_setting_Scene_Mode)/setting_item.sub_item_max);
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_scene_mode_set(str->font_color);
			}
			break;
*/		case STR_ISO:
			curr_tag = ap_state_config_iso_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_AUTO2;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 3;
				setting_item.sub_item_start = STR_AUTO2;
			} else if (type == SUB_MENU_MOVE) {
				if(str->font_color == 0){
					ap_setting_sensor_command_switch(0x13, 0x04, 1);
				}else{
					ap_setting_sensor_command_switch(0x13, 0x04, 0);
				}			
				ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_ISO_data, str->font_color, sizeof(ap_setting_ISO_data)/setting_item.sub_item_max);				
			} else {
				ap_state_config_iso_set(str->font_color);
			}
			break;
		case STR_COLOR:
			curr_tag = ap_state_config_color_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_STANDARD1;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_STANDARD1;
			} else if (type == SUB_MENU_MOVE) {
				if(str->font_color == 0){
					ap_setting_sensor_command_switch(0xA6, 0x01, 0);
				}else{
					ap_setting_sensor_command_switch(0xA6, 0x01, 1);
					ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Color, str->font_color, sizeof(ap_setting_Color)/setting_item.sub_item_max);
				}
			} else {
				ap_state_config_color_set(str->font_color);
			}
			break;
		case STR_SATURATION:
			curr_tag = ap_state_config_saturation_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_HIGH;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 3;
				setting_item.sub_item_start = STR_HIGH;
			} else if (type == SUB_MENU_MOVE) {
				ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Saturation, str->font_color, sizeof(ap_setting_Saturation)/setting_item.sub_item_max);
			} else {
				ap_state_config_saturation_set(str->font_color);
			}
			break;
		case STR_SHARPNESS:
			curr_tag = ap_state_config_sharpness_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_HARD;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 3;
				setting_item.sub_item_start = STR_HARD;
			} else if (type == SUB_MENU_MOVE) {
				if(str->font_color == 1){
					ap_setting_sensor_command_switch(0xAC, 0x20, 1);
				}else{
					ap_setting_sensor_command_switch(0xAC, 0x20, 1);
					ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Sharpness, str->font_color, sizeof(ap_setting_Sharpness)/setting_item.sub_item_max);
				}
			} else {
				ap_state_config_sharpness_set(str->font_color);
			}
			break;
		case STR_PREVIEW:
			curr_tag = ap_state_config_preview_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_ON;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_ON;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_preview_set(str->font_color);
			}
			break;
		case STR_BURST:
			curr_tag = ap_state_config_burst_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_OFF;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_OFF;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_burst_set(str->font_color);
			}
			break;
		case STR_DELETE:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_DELETE_ONE;
				setting_item.stage = STR_DELETE;
			} else if (type == SECOND_SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
				setting_item.stage = 0;
				g_setting_delete_choice = str->font_color;
			} else if (type == FUNCTION_ACTIVE) {
				if(g_setting_delete_choice == 0){
					if (str->font_color == 1) {
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VIDEO_FILE_DEL, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
					}
				} else if (g_setting_delete_choice == 1){
					if (str->font_color == 1) {
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FILE_DEL_ALL, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
					}
				}
			}
			break;
		case STR_LOCK:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 4;
				setting_item.sub_item_start = STR_LOCK_ONE;
				setting_item.stage = STR_LOCK;
			} else if (type == SECOND_SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
				setting_item.stage = 0;
				g_setting_lock_choice = str->font_color;
			} else if (type == FUNCTION_ACTIVE) {
				if(g_setting_lock_choice == 0){
					if (str->font_color == 1) {
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_LOCK_ONE, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
					}
				} else if (g_setting_lock_choice == 1){
					if (str->font_color == 1) {
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_LOCK_ALL, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
					}				
				} else if (g_setting_lock_choice == 2){
					if (str->font_color == 1) {
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_UNLOCK_ONE, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
					}				
				} else if (g_setting_lock_choice == 3){
					if (str->font_color == 1) {
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_UNLOCK_ALL, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
					}				
				}
			}
			break;						
/*		case STR_DELETE_ONE:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
			} else if (type == FUNCTION_ACTIVE) {
				if (str->font_color == 1) {
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VIDEO_FILE_DEL, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
				}
			}
			break;
		case STR_DELETE_ALL:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
			} else if (type == FUNCTION_ACTIVE) {
				if (str->font_color == 1) {
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FILE_DEL_ALL, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
				}
			}
			break;
		case STR_LOCK_ONE:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
			} else if (type == FUNCTION_ACTIVE) {
				if (str->font_color == 1) {
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_LOCK_ONE, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
				}
			}
			break;
		case STR_LOCK_ALL:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
			} else if (type == FUNCTION_ACTIVE) {
				if (str->font_color == 1) {
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_LOCK_ALL, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
				}
			}
			break;
		case STR_UNLOCK_ONE:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
			} else if (type == FUNCTION_ACTIVE) {
				if (str->font_color == 1) {
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_UNLOCK_ONE, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
				}
			}
			break;
		case STR_UNLOCK_ALL:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
			} else if (type == FUNCTION_ACTIVE) {
				if (str->font_color == 1) {
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_UNLOCK_ALL, &str->font_color, sizeof(INT32U), MSG_PRI_NORMAL);
				}
			}
			break;					
*/			
		case STR_THUMBNAIL:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
			} else if (type == FUNCTION_ACTIVE) {
				if (str->font_color == 1) {
					msgQSend(ApQ, MSG_APQ_INIT_THUMBNAIL, NULL, NULL, MSG_PRI_NORMAL);
				}
			}
			break;
		case STR_VOLUME:
			curr_tag = ap_state_config_volume_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_VOLUME_0;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 7;
				setting_item.sub_item_start = STR_VOLUME_0;
			} else if (type == SUB_MENU_MOVE) {
				audio_vol_set(str->font_color);
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_volume_set(str->font_color);
				audio_vol_set(str->font_color);
			}
			break;
		case STR_FORMAT:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
			} else if (type == FUNCTION_ACTIVE) {
				if (str->font_color == 1 && ap_state_handling_storage_id_get() != NO_STORAGE) {					
					if(fm_tx_status_get() == 1){
						msgQSend(AudioTaskQ, MSG_AUD_STOP, NULL, 0, MSG_PRI_NORMAL);
					}
					setting_item.stage = 0x55AA;
					return 0;
				}
			}
			break;
		case STR_LANGUAGE:
			curr_tag = ap_state_config_language_get() - 1;
			if (type == STRING_DRAW) {
				str->str_idx = STR_MULTI_LANGUAGE;
				str->language = curr_tag;
				ap_state_resource_string_draw((INT16U *)setting_frame_buff, str);
				return curr_tag;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = ap_state_resource_language_num_get() - 2;
				setting_item.sub_item_start = STR_MULTI_LANGUAGE;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_language_set(str->font_color);
			}
			break;
		case STR_AUTO_OFF:
			curr_tag = ap_state_config_auto_off_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_OFF2;

				//wwj add
				if(str->str_idx == STR_2_MIN) {
					str->str_idx = STR_3_MIN;
				} else if (str->str_idx == STR_3_MIN) {
					str->str_idx = STR_5_MIN;
				}
				//wwj add end

			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 4;//6;	//wwj modify
				setting_item.sub_item_start = STR_OFF2;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_auto_off_set(str->font_color);
			}
			break;
		case STR_SYS_RESET:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				curr_tag = 0;
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_CANCEL;
			} else if (type == FUNCTION_ACTIVE) {
				if (str->font_color == 1) {
					DBG_PRINT("menu - system reset\r\n");
					ap_state_config_default_set();
/* #BEGIN# modify by xyz - 2014.12.11 */
//					ap_state_config_factory_date_get(&setup_date_time[0]);
//					ap_state_config_factory_time_get(&setup_date_time[3]);
//					g_time.tm_year = setup_date_time[0] + 2000;
//					g_time.tm_mon  = setup_date_time[1];
//					g_time.tm_mday = setup_date_time[2];
//					g_time.tm_hour = setup_date_time[3];
//					g_time.tm_min  = setup_date_time[4];
//					g_time.tm_sec  = setup_date_time[5];
//					cal_time_set(g_time);					
/* #END#   modify by xyz - 2014.12.11 */
					ap_setting_value_set_from_user_config();
				}
			}
			break;
		case STR_LIGHT_FREQ:
			curr_tag = ap_state_config_light_freq_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_50HZ;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_50HZ;
			} else if (type == SUB_MENU_MOVE) {
				ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Light_Freq, str->font_color, sizeof(ap_setting_Light_Freq)/setting_item.sub_item_max);			
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_light_freq_set(str->font_color);
			}
			break;
		case STR_TV_OUTPUT:
			curr_tag = ap_state_config_tv_out_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_NTSC;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_NTSC;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_tv_out_set(str->font_color);
			}
			break;
		case STR_DATE_INPUT:
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 100;
				setting_item.sub_item_start = 0;
				setting_item.stage = STR_DATE_INPUT;
				cal_time_get(&g_time);
				setup_date_time[0] = g_time.tm_year - 2000;
				setup_date_time[1] = g_time.tm_mon;
				setup_date_time[2] = g_time.tm_mday;
				setup_date_time[3] = g_time.tm_hour;
				setup_date_time[4] = g_time.tm_min;
				setup_date_time[5] = g_time.tm_sec;
				ap_setting_date_time_menu_draw(0);
				return 0;
			} else if (type == FUNCTION_ACTIVE) {
				if(setup_date_time_counter == 4){
					g_time.tm_year = setup_date_time[0] + 2000;
					g_time.tm_mon = setup_date_time[1];
					g_time.tm_mday = setup_date_time[2];
					g_time.tm_hour = setup_date_time[3];
					g_time.tm_min = setup_date_time[4];
					g_time.tm_sec = setup_date_time[5];
					cal_time_set(g_time);
				} else {
					*sub_tag += 1;
					if (*sub_tag == 1) {
						setting_item.sub_item_start = 1;
						setting_item.sub_item_max = 13;
					} else if (*sub_tag == 2) {
						setting_item.sub_item_start = 1;
						setting_item.sub_item_max = CalendarCalculateDays(setup_date_time[1], setup_date_time[0]) + 1;
					} else if (*sub_tag == 3) {
						setting_item.sub_item_start = 0;
						setting_item.sub_item_max = 24;
					} else {
						setting_item.sub_item_start = 0;
						setting_item.sub_item_max = 60;
					}
					ap_setting_date_time_menu_draw(*sub_tag);
				}
			}
			break;
		case STR_APP_VERSION:
			DBG_PRINT("App version:%s\r\n",__DATE__);
			if (type == STRING_DRAW) {
				str->str_idx = STR_NEXT_MENU;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 100;
				setting_item.sub_item_start = 0;
				setting_item.stage = STR_DATE_INPUT;
				
				ap_setting_show_software_version();
				return 0;
			} else if (type == FUNCTION_ACTIVE) {
				
			}
			break;
		case STR_USB:
			curr_tag = ap_state_config_usb_mode_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_PC_CAM;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 2;
				setting_item.sub_item_start = STR_PC_CAM;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_usb_mode_set(str->font_color);
			}
			break;
			
		case STR_MD_SENSITIVITY:
			curr_tag = ap_state_config_motion_detect_get();
			if (type == STRING_DRAW) {
				str->str_idx = curr_tag + STR_HIGH;
			} else if (type == SUB_MENU_DRAW) {
				setting_item.sub_item_max = 3;
				setting_item.sub_item_start = STR_HIGH;
			} else if (type == FUNCTION_ACTIVE) {
				ap_state_config_motion_detect_set(str->font_color);
			}
			break;
	}
	if (type == STRING_DRAW) {
		ap_state_resource_string_draw((INT16U *)setting_frame_buff, str);
	} else if ((type == SUB_MENU_DRAW) || (type == SECOND_SUB_MENU_DRAW)) {
		ap_setting_sub_menu_draw(curr_tag);
	} else if (type == FUNCTION_ACTIVE) {
		ap_state_config_store();
	}
	return curr_tag;
}

void ap_setting_func_key_active(INT8U *tag, INT8U *sub_tag, INT32U state)
{
	STRING_INFO str_fake;

	if (setting_item.stage == 0x55AA) {
		return;
	}
	str_fake.str_idx = setting_item.item[*tag];
	if (*sub_tag == 0xFF) {
		*sub_tag = ap_setting_right_menu_active(&str_fake, SUB_MENU_DRAW, NULL);
	} else {
		str_fake.font_color = *sub_tag;
		if ((setting_item.stage == STR_DELETE) || (setting_item.stage == STR_LOCK)) {
			ap_setting_right_menu_active(&str_fake, SECOND_SUB_MENU_DRAW, (INT8U *)sub_tag);
			*sub_tag = 0;
			return;
		}
		ap_setting_right_menu_active(&str_fake, FUNCTION_ACTIVE, (INT8U *)sub_tag);
		if (setting_item.stage == STR_DATE_INPUT) {
			setup_date_time_counter++;
			if(setup_date_time_counter == 5){
				setting_item.stage = 0;
				setup_date_time_counter = 0;				
			} else {
				return;
			}
		} else if (setting_item.stage == 0x55AA) {
			ap_setting_page_draw(state, tag);
			ap_setting_format_busy_show();
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FORMAT_REQ, NULL, NULL, MSG_PRI_NORMAL);
			*sub_tag = 0xFF;
			return;
		}
		*sub_tag = 0xFF;
		ap_setting_page_draw(state, tag);		
	}
}

void ap_setting_direction_key_active(INT8U *tag, INT8U *sub_tag, INT32U key_type, INT32U state)
{
	STRING_INFO str_fake;
	INT8U total, *tag_ptr, bound;
	INT32U key_type_temp;

	key_type_temp = MSG_APQ_NEXT_KEY_ACTIVE;
	if (setting_item.stage == 0x55AA) {
		return;
	}
	str_fake.str_idx = setting_item.item[*tag];	
	
	bound = 0;
	if (*sub_tag == 0xFF) {
		total = setting_item.item_max;
		tag_ptr = tag;
	} else {
		if (setting_item.stage != STR_DATE_INPUT) {
			total = setting_item.sub_item_max;
			tag_ptr = sub_tag;
		} else {
			total = setting_item.sub_item_max;
			tag_ptr = &setup_date_time[*sub_tag];
			bound = setting_item.sub_item_start;
			key_type_temp = MSG_APQ_PREV_KEY_ACTIVE;
		}
	}
	if (key_type == key_type_temp) {
		(*tag_ptr)++;
		if (*tag_ptr >= total) {
			*tag_ptr = bound;
		}
	} else {
		if (*tag_ptr == bound) {
			*tag_ptr = total;
		}
		(*tag_ptr)--;
	}
	if (*sub_tag == 0xFF) {
		ap_setting_page_draw(state, tag);
	} else {
		if (setting_item.stage != STR_DATE_INPUT) {
			ap_setting_sub_menu_draw(*sub_tag);
			str_fake.font_color = *sub_tag;
			ap_setting_right_menu_active(&str_fake, SUB_MENU_MOVE, NULL);
		} else {
			if(str_fake.str_idx == STR_APP_VERSION){
				return;
			}
			ap_setting_date_time_menu_draw(*sub_tag);
		}
	}
}

EXIT_FLAG_ENUM ap_setting_menu_key_active(INT8U *tag, INT8U *sub_tag, INT32U *state, INT32U *state1)
{
	if (setting_item.stage == 0x55AA) {
		return EXIT_RESUME;
	}
	if (*sub_tag == 0xFF) {
		*tag = 0;
		if (*state != STATE_SETTING) {
			*state = STATE_SETTING;
		} else {
			*state = *state1;
			OSQPost(StateHandlingQ, (void *) *state);
			return EXIT_BREAK;
		}
	} else {
		*sub_tag = 0xFF;
		setting_item.stage = 0;
		setup_date_time_counter = 0;
		ap_setting_value_set_from_user_config();
	}
	ap_setting_page_draw(*state, tag);
	return EXIT_RESUME;
}

EXIT_FLAG_ENUM ap_setting_mode_key_active(INT32U next_state, INT8U *sub_tag)
{
	if (setting_item.stage == 0x55AA) {
		return EXIT_RESUME;
	} else {
		setting_item.stage = 0;
		setup_date_time_counter = 0;
		OSQPost(StateHandlingQ, (void *) next_state);
		ap_setting_value_set_from_user_config();
		return EXIT_BREAK;
	}
}

void ap_setting_format_reply(INT8U *tag, INT32U state)
{
	setting_item.stage = 0;
	ap_setting_page_draw(state, tag);
	//MP3 function reset value
	if(fm_tx_status_get() == 1){
		ap_music_reset();
	}	
}

void ap_setting_format_busy_show(void)
{
	DISPLAY_ICONSHOW icon;
	t_STRING_TABLE_STRUCT str_res;
	STRING_INFO str;
	
	str.font_type = 0;
	str.str_idx = STR_PLEASE_WAIT;
	str.buff_w = TFT_WIDTH;
	str.buff_h = TFT_HEIGHT;
	str.language = ap_state_config_language_get() - 1;
	ap_state_resource_string_resolution_get(&str, &str_res);
	str.pos_x = (TFT_WIDTH - str_res.string_width) >> 1;
	str.pos_y = (TFT_HEIGHT - str_res.string_height) >> 1;
	str.font_color = 0xFFFF;
	icon.pos_x = 70;
	icon.pos_y = 88;
	icon.transparent = TRANSPARENT_COLOR;
	icon.icon_w = 184;
	icon.icon_h = 64;
	ap_setting_icon_draw((INT16U *)setting_frame_buff, sub_menu_2, &icon, SETTING_ICON_NORMAL_DRAW);
	ap_state_resource_string_draw((INT16U *)setting_frame_buff, &str);
	ap_setting_frame_buff_display();
}

void ap_setting_icon_draw(INT16U *frame_buff, INT16U *icon_stream, DISPLAY_ICONSHOW *icon_info, INT8U type)
{
	INT32U x, y, offset_pixel, offset_pixel_tmp, offset_data, offset_data_tmp, *ptr;
	
	if (icon_stream == background_a || icon_stream == background_c || icon_stream == background_b) {
		offset_data = (TFT_WIDTH*TFT_HEIGHT) >> 1;
		ptr = (INT32U *) setting_frame_buff;
		for (x=0 ; x<offset_data ; x++) {
			*(ptr++) = 0x8C718C71;
		}
	}	
	offset_data = 0;
	for (y=0 ; y<icon_info->icon_h ; y++) {
		offset_data_tmp = y*icon_info->icon_w;
		offset_pixel_tmp = (icon_info->pos_y + y)*TFT_WIDTH + icon_info->pos_x;
		for (x=0 ; x<icon_info->icon_w ; x++) {
			if (type) {
				offset_data = offset_data_tmp + x;
			}
			if (*(icon_stream + offset_data) == icon_info->transparent) {
				continue;
			}
			offset_pixel = offset_pixel_tmp + x;
			if (offset_pixel <= TFT_WIDTH*TFT_HEIGHT) {
				*(frame_buff + offset_pixel) = *(icon_stream + offset_data);
			}
		}
	}
}

void ap_setting_date_time_string_process(INT8U dt_tag)
{
//	INT8U i, data_tmp, days, tag[] = {2, 3, 9, 10, 16, 17, 24, 25, 31, 32};
	INT8U i, data_tmp, days, tag[] = {2, 3, 7, 8, 12, 13, 17, 18, 22, 23};

	days = CalendarCalculateDays(setup_date_time[1], setup_date_time[0]);
	if (setup_date_time[2] > days) {
		setup_date_time[2] = days;
	}	
	if (dt_tag == SETTING_DATE_TIME_DRAW_ALL) {
		for (i=0 ; i<5 ; i++) {
			data_tmp = setup_date_time[i]/10;
			*(dt_str + tag[(i<<1)]) = data_tmp + 0x30;
			data_tmp = (setup_date_time[i] - (10*data_tmp));
			*(dt_str + tag[(i<<1)+1]) = data_tmp + 0x30;
		}
	} else {
		data_tmp = setup_date_time[dt_tag]/10;
		*(dt_str + tag[(dt_tag<<1)]) = data_tmp + 0x30;
		data_tmp = (setup_date_time[dt_tag] - (10*data_tmp));
		*(dt_str + tag[(dt_tag<<1)+1]) = data_tmp + 0x30;
	}
}

void ap_setting_frame_buff_display(void)
{
	OSQPost(DisplayTaskQ, (void *) (setting_frame_buff | MSG_DISPLAY_TASK_SETTING_DRAW));
	if (setting_browse_frame_buff) {
		OSQPost(DisplayTaskQ, (void *) (setting_browse_frame_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
	}
}

INT8U CalendarCalculateDays(INT8U month, INT8U year)
{
	INT8U DaysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31}; 
	 
	year += 2000;
	if ((year&0x3) || (!(year%100) && (year%400))) {
		return DaysInMonth[month - 1];
	} else {
		if (month == 2) {
			return 29;
		} else {
			return DaysInMonth[month - 1];
		}
	}
}

void ap_setting_value_set_from_user_config(void)
{
#ifdef	__OV7725_DRV_C__
	/*	//wwj mark
	if(ap_state_config_night_mode_get() == 1) {
		gpio_write_io(IR_CTRL, DATA_HIGH);	
	} else {
		gpio_write_io(IR_CTRL, DATA_LOW);	
	} */	
	ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_EV_data, ap_state_config_ev_get(), 4);
	
	ap_setting_sensor_command_switch(0xAC, 0x20, 1);
	ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Sharpness, ap_state_config_sharpness_get(), 2);
	ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Saturation, ap_state_config_saturation_get(), 4);
	if(ap_state_config_color_get() == 0){
		ap_setting_sensor_command_switch(0xA6, 0x01, 0);
	}else{
		ap_setting_sensor_command_switch(0xA6, 0x01, 1);
	}	
	ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Color, ap_state_config_color_get(), 4);
	if(ap_state_config_white_balance_get() == 0){
		ap_setting_sensor_command_switch(0x13, 0x02, 1);
	}else{
		ap_setting_sensor_command_switch(0x13, 0x02, 0);
	}
	ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_White_Balance, ap_state_config_white_balance_get(), 6);
	if(ap_state_config_iso_get() == 0){
		ap_setting_sensor_command_switch(0x13, 0x04, 1);
	}else{
		ap_setting_sensor_command_switch(0x13, 0x04, 0);
	}	
	ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_ISO_data, ap_state_config_iso_get(), 2);
	ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Scene_Mode, ap_state_config_scene_mode_get(), 4);
	ap_setting_sensor_sccb_cmd_set((INT8U *) ap_setting_Light_Freq, ap_state_config_light_freq_get(), 4);
#endif	
}

void ap_setting_sensor_sccb_cmd_set(INT8U *table_ptr, INT16U idx, INT8U cmd_num)
{
	INT32U i;
#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2	
	INT32U tmp;
	SPI_LOCK();	//wwj add
	tmp = R_SPI0_CTRL;
	R_SPI0_CTRL = 0;
#endif	
	for (i=0; i<cmd_num; i+=2) {
		//sccb_write(SLAVE_ID, *(table_ptr + idx*cmd_num + i), *(table_ptr + idx*cmd_num + (i+1)));
	}
#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2	
	R_SPI0_CTRL = tmp;
	SPI_UNLOCK();	//wwj add
#endif
}

void ap_setting_sensor_command_switch(INT16U cmd_addr, INT16U reg_bit, INT8U enable)
{
		
#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2	
	INT32U spi_temp;
	SPI_LOCK();	//wwj add
	spi_temp = R_SPI0_CTRL;
	R_SPI0_CTRL = 0;
#endif	

	if(enable == 1)
	{	
	//	temp = sccb_read(SLAVE_ID, cmd_addr);
		//temp |= reg_bit;	//Enable
		//sccb_write(SLAVE_ID, cmd_addr, temp);
	}else{
		//temp = sccb_read(SLAVE_ID, cmd_addr);
		//temp &= ~(reg_bit);	//Disable
		//sccb_write(SLAVE_ID, cmd_addr, temp);
	}
		
#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2	
	R_SPI0_CTRL = spi_temp;
	SPI_UNLOCK();	//wwj add
#endif		
}

INT8U* get_cmputer_build_time(void);

void ap_setting_show_software_version(void)
{

	STRING_ASCII_INFO ascii_str;
	INT8U *pDate,date_time[25] ;
	INT16U i;
	DISPLAY_ICONSHOW cursor_icon = {320, 234, TRANSPARENT_COLOR, 0, 0};
	
	ap_setting_icon_draw((INT16U *)setting_frame_buff, background_c, &cursor_icon, SETTING_ICON_NORMAL_DRAW);
	
	ascii_str.font_color = 255;
	ascii_str.font_type = 0;
	ascii_str.pos_x = ICON_SELECTBOX_SUB_START_X+50;
	ascii_str.pos_y = 110;
	ascii_str.str_ptr = "20140819 V0.1";
	
	ascii_str.buff_w = TFT_WIDTH;
	ascii_str.buff_h = TFT_HEIGHT;
	
	ap_state_resource_string_ascii_draw((INT16U *) setting_frame_buff, &ascii_str);	
	ascii_str.str_ptr = __DATE__;
	ascii_str.pos_y += 30;
	ap_state_resource_string_ascii_draw((INT16U *) setting_frame_buff, &ascii_str);
	ascii_str.str_ptr = __TIME__;
	ascii_str.pos_x += 128;
	ap_state_resource_string_ascii_draw((INT16U *) setting_frame_buff, &ascii_str);	
	ap_setting_frame_buff_display();

}

