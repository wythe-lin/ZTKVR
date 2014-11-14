#include "state_video_record.h"

//	prototypes
void state_video_record_init(INT32U prev_state);
void state_video_record_exit(void);

extern INT8U s_usbd_pin;	// "extern" isn't good...   Neal
extern STOR_SERV_FILEINFO curr_file_info;	//wwj add
extern STOR_SERV_FILEINFO next_file_info;	//wwj add

extern void capture_mode_enter(void);

void state_video_record_init(INT32U prev_state)
{
	DBG_PRINT("video_record state init enter\r\n");
	if(prev_state != STATE_STARTUP){	
		OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_OFF);
	}
	ap_setting_value_set_from_user_config();
	ap_setting_sensor_command_switch(0x0E, 0x80, 0);	//night mode off
	ap_setting_sensor_command_switch(0x13, 0x04, 1);	//ISO
	ap_setting_sensor_command_switch(0xA6, 0x01, 0);	//Color
	if(prev_state != STATE_STARTUP){
		OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_ON);
	}
	ap_video_record_init();
	ap_state_handling_scroll_bar_init();
	ap_music_update_icon_status();

	ap_video_clear_lock_flag();
	if( (ap_state_handling_storage_id_get() != NO_STORAGE) && (prev_state == STATE_STARTUP) && (s_usbd_pin == 0) ) {
/* #BEGIN# modify by ShengHua - 2014.10.20 */
		OSTimeDly(70);
/* #END# modify by ShengHua - 2014.10.20 */
		ap_video_record_func_key_active(MSG_APQ_FUNCTION_KEY_ACTIVE);
	}
}

void state_video_record_entry(void *para)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	FP32 zoom_factor;
	INT32U msg_id,prev_state;
	INT16U temp_vol;
	INT8U  USB_connect_flag = 0;
	STAudioConfirm *audio_temp;

	zoom_factor = 1;
	prev_state = *((INT32U *) para);	
	state_video_record_init(prev_state);
	while (exit_flag == EXIT_RESUME) {
		if (msgQReceive(ApQ, &msg_id, (void *) ApQ_para, AP_QUEUE_MSG_MAX_LEN) == STATUS_FAIL) {
			continue;
		}
		switch (msg_id) {
			case EVENT_APQ_ERR_MSG:
				audio_temp = (STAudioConfirm *)ApQ_para;
				if ((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_APP_RS)){

					#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
					  #if DV185	//wwj add
						gpio_write_io(SPEAKER_EN, DATA_LOW);
					  #elif DV188
						gpx_rtc_write(8,0x08);
					  #endif
					#endif
				}else{
					audio_confirm_handler((STAudioConfirm *)ApQ_para);
				}
				break;
			case MSG_STORAGE_SERVICE_MOUNT:
				ap_video_clear_lock_flag();
				ap_state_handling_str_draw_exit();
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_video_record_sts_set(~VIDEO_RECORD_UNMOUNT);
        		ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
        		video_calculate_left_recording_time_enable();
        		ap_music_update_icon_status();
					//wwj mark
				if( (ap_state_handling_storage_id_get() != NO_STORAGE) && (s_usbd_pin == 0) ) {
					ap_video_record_func_key_active(MSG_APQ_FUNCTION_KEY_ACTIVE);
					//DBG_PRINT("Video Record Key Active\r\n");
				} 
        		DBG_PRINT("[Video Record Mount OK]\r\n");
				//ap_video_record_func_key_active(MSG_APQ_FUNCTION_KEY_ACTIVE);
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
				ap_video_clear_lock_flag();
				#if C_MOTION_DETECTION == CUSTOM_ON
                ap_video_record_md_icon_clear_all(); // dominant add to disable md icon
	            ap_video_record_md_disable();  // dominant add to disable md ISR
                ap_state_config_md_set(0); 
				#endif
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_video_record_sts_set(VIDEO_RECORD_UNMOUNT);
        		ap_state_handling_str_draw_exit();
        		video_calculate_left_recording_time_disable();
        		ap_state_handling_icon_show_cmd(ICON_NO_SD_CARD, NULL, NULL);
				ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL); //wwj add
   				//ap_music_update_icon_status();
   				ap_state_handling_mp3_index_show_zero_cmd();
   				ap_state_handling_mp3_total_index_show_zero_cmd();
   				//Daniel add for step delete
   				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_AUTO_DEL_LOCK, NULL, NULL, MSG_PRI_NORMAL);   				
                ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
                msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
        		DBG_PRINT("[Video Record Mount FAIL]\r\n");
        		break;
        	case MSG_APQ_SCROLL_BAR_END:
        		ap_state_handling_scroll_bar_exit(SCROLL_DISAPPEAR);
        		break;
#if 0//C_ZOOM == CUSTOM_ON 
        	case MSG_APQ_NEXT_KEY_ACTIVE:
        	case MSG_APQ_PREV_KEY_ACTIVE:
        		ap_state_handling_zoom_active(&zoom_factor, msg_id);
        		break;
#endif        		
        	case MSG_APQ_POWER_KEY_ACTIVE:
				if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) && ((curr_file_info.file_handle == -1) ||(next_file_info.file_handle == -1))) {	//wwj add
					break;
				}

//        		video_calculate_left_recording_time_disable();	//wwj mark, move to ap_video_record_power_off_handle()
        		ap_state_config_md_set(0);
        		sensor_clock_disable = 1;
        		OSTimeDly(10);
        		ap_video_record_power_off_handle();
        		break;
        	case MSG_APQ_MENU_KEY_ACTIVE:
        		if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) == 0){
        			OSQPost(StateHandlingQ, (void *) STATE_SETTING);
        			exit_flag = EXIT_BREAK;
        		}
        		break;	

		case MSG_APQ_CAMSWITCH_ACTIVE:
			camswitch_set_mode(ApQ_para[0]);
			break;

        	case MSG_APQ_MODE:
				if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) && ((curr_file_info.file_handle == -1)||(next_file_info.file_handle == -1))) {	//wwj add
					break;
				}

				sensor_clock_disable = 0;
				#if C_MOTION_DETECTION == CUSTOM_ON
				ap_video_record_md_disable();
				#endif
				ap_state_config_md_set(0);
#if 0//C_MOTION_DETECTION == CUSTOM_ON
				OSQPost(StateHandlingQ, (void *) STATE_MOTION_DETECTION);
#elif 1		// browse mode
				sensor_clock_disable = 1;
				OSQPost(StateHandlingQ, (void *) STATE_BROWSE);
				break;
#else		// image capture mode
				capture_mode_enter();
        		OSQPost(StateHandlingQ, (void *) STATE_VIDEO_PREVIEW);
#endif
        		exit_flag = EXIT_BREAK;
        		break;
        	case MSG_APQ_SENSOR_CLOCK_DISABLED:
        		sensor_clock_disable = 0;
        		exit_flag = EXIT_BREAK;
        		break;
#if C_MOTION_DETECTION == CUSTOM_ON
			case MSG_APQ_MOTION_DETECT_TICK:
				ap_video_record_md_tick();
				break;	
			case MSG_APQ_MOTION_DETECT_ICON_UPDATE:
				ap_video_record_md_icon_update(ApQ_para[0]);
				break;
			case MSG_APQ_MOTION_DETECT_ACTIVE:
				if (ap_video_record_md_active()) {
					break;
				}
#endif			
        	case MSG_APQ_FUNCTION_KEY_ACTIVE:
        		if (video_encode_status() == VIDEO_CODEC_PROCESSING) {
					#if C_MOTION_DETECTION == CUSTOM_ON
        			ap_video_record_md_disable();
					#endif
        		}
				ap_video_record_func_key_active(msg_id);
        		break;
#if C_AUTO_DEL_FILE == CUSTOM_ON			
			case MSG_APQ_VIDEO_FILE_DEL_REPLY:
              #if 0
                if (!ap_video_record_del_reply(*((INT32S *) ApQ_para))) {
					ap_video_record_reply_action(0);
				}
              #endif  
				break;

			case MSG_APQ_FREE_FILESIZE_CHECK_REPLY:
				if(*((INT32S *) ApQ_para) == STATUS_OK) {
					ap_video_record_start();
				} else {
					ap_state_handling_str_draw_exit();
					ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
					video_calculate_left_recording_time_disable();
				}
				break;

#else
			case MSG_APQ_VID_CYCLIC_FILE_DEL_REPLY:
				ap_video_cyclic_record_del_reply(*((INT32S *) ApQ_para));
				break;	
        	case MSG_APQ_FREE_SIZE_CHECK_REPLY:
        		ap_video_record_reply_action(0);
                break;
#endif				
        	case MSG_STORAGE_SERVICE_VID_REPLY:
        		ap_video_record_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
        		if(USB_connect_flag) {
					USB_connect_flag = 0;
					goto	USB_connect_start;
        		}
        		break;
                
#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
			case MSG_STORAGE_SERVICE_VID_CYCLIC_REPLY:
        		ap_video_record_cycle_reply_open((STOR_SERV_FILEINFO *) ApQ_para);
        		break;      	
        	case MSG_APQ_CYCLIC_VIDEO_RECORD:      		
        		ap_video_record_cycle_reply_action();
        		break;
#endif
			case MSG_APQ_CONNECT_TO_PC:
				if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) && ((curr_file_info.file_handle == -1)||(next_file_info.file_handle == -1))) {	//wwj add
					USB_connect_flag = 1;
					break;
				}
USB_connect_start:
				audio_send_stop();
        		//ap_video_record_error_handle();
        		if (video_encode_status() == VIDEO_CODEC_PROCESSING) {
				#if C_MOTION_DETECTION == CUSTOM_ON
        			ap_video_record_md_disable();
				#endif
        		}
        		if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) != 0) {
        			ap_video_record_func_key_active(MSG_APQ_FUNCTION_KEY_ACTIVE);
        		}
        		ap_state_config_md_set(0);
        		ap_state_handling_connect_to_pc();
        		break;
        	case MSG_APQ_DISCONNECT_TO_PC:
        		ap_state_handling_disconnect_to_pc();
        		ap_music_update_icon_status();
        		break;
            case MSG_APQ_VDO_REC_RESTART:
				if(((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) == 0) //wwj add
					|| ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) && ((curr_file_info.file_handle == -1)||(next_file_info.file_handle == -1))))
				{
					break;
				}
        	case MSG_APQ_AVI_PACKER_ERROR:
				ap_video_record_error_handle();

                // Dominant add. restart AVI record again.
                if(drvl2_sdc_live_response() == 0) {  // Card exist
					if (ap_state_config_md_get()) {
					    break;  // motion detect mode..... no need restart
					} else {
						INT64U  disk_free_size;

						disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
						if (disk_free_size <= CARD_FULL_MB_SIZE) {
						    ap_state_handling_str_draw_exit();
						    ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
				            video_calculate_left_recording_time_disable();
						} else {
							msgQSend(ApQ, MSG_APQ_FUNCTION_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL); //restart
						}
					}
                } else {
                #if C_MOTION_DETECTION == CUSTOM_ON
					// if no card, md detection function need to disable
					if(ap_state_config_md_get())
					{
						ap_video_record_md_icon_clear_all();
						ap_video_record_md_disable();
						ap_state_config_md_set(0);
					}
				#endif
			        msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL); //wwj add
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL); //wwj add
                }
                break;

           case MSG_APQ_VDO_REC_STOP:
                if(((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)==VIDEO_RECORD_BUSY) || ((ap_video_record_sts_get() & VIDEO_RECORD_MD)==VIDEO_RECORD_MD)) 
                {
					if((curr_file_info.file_handle == -1)||(next_file_info.file_handle == -1)) { //wwj add
						break;
					}
					
					ap_video_record_error_handle();
					ap_state_handling_str_draw_exit();
					ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
			        video_calculate_left_recording_time_disable();
					#if C_MOTION_DETECTION == CUSTOM_ON
					if(ap_state_config_md_get())
					{
						ap_video_record_md_icon_clear_all();
						ap_video_record_md_disable();
						ap_state_config_md_set(0);
					}
					#endif
                }
                break;                
#if C_BATTERY_DETECT == CUSTOM_ON        	
        	case MSG_APQ_BATTERY_LVL_SHOW:
        		ap_state_handling_battery_icon_show(ApQ_para[0]);
        		break;
        	case MSG_APQ_BATTERY_CHARGED_SHOW:
//        		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_SHOW | ICON_BATTERY_CHARGED));
				ap_state_handling_charge_icon_show(1);	//wwj modify
        		break;
        	case MSG_APQ_BATTERY_CHARGED_CLEAR:
//        		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | ICON_BATTERY_CHARGED));
				ap_state_handling_charge_icon_show(0);	//wwj modify
        		break;        		        		
#endif        		
#if C_SCREEN_SAVER == CUSTOM_ON
			case MSG_APQ_KEY_IDLE:
        		ap_state_handling_lcd_backlight_switch(0);         	
        		break;
			case MSG_APQ_KEY_WAKE_UP:			
        		ap_state_handling_lcd_backlight_switch(1);        	
        		break;        		
#endif        		
			case MSG_APQ_NIGHT_MODE_KEY:	//wwj add
				ap_state_handling_night_mode_switch();
				break;

			case MSG_APQ_UPDATE_MP3_TOTAL_INDEX:
				ap_state_handling_mp3_total_index_show_cmd();
				break;
							
			case MSG_APQ_STORAGE_SCAN_DONE:
				if(storage_file_nums_get() != 0) {
					if(music_file_idx >= storage_file_nums_get()){
						music_file_idx = 0;
					}				
					ap_state_handling_mp3_index_show_cmd();
				}
				else {
					music_file_idx = 0;
					ap_state_handling_mp3_index_show_zero_cmd();
				}	
				break;			
			case EVENT_KEY_MEDIA_PLAY_PAUSE:
			    audio_play_pause_process();
			    break;
			case EVENT_KEY_MEDIA_VOL_UP:
				audio_vol_inc_set();
				break;
			case EVENT_KEY_MEDIA_VOL_DOWN:
				audio_vol_dec_set();
				break;
			case EVENT_KEY_MEDIA_PREV:
				audio_prev_process();
				break;
			case EVENT_KEY_MEDIA_NEXT:
				audio_next_process();
				break;
			case EVENT_KEY_MEDIA_STOP:
				audio_send_stop();
				if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) == 0) {
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
				}
				break;
			case EVENT_KEY_MODE:
				audio_playing_mode_set_process();
				break;
			case EVENT_KEY_MEDIA_QUICK_SEL:
				DBG_PRINT("quick select = %d\r\n",*(INT32U*) ApQ_para); 
				audio_quick_select(*(INT32U*) ApQ_para);
				break;
			case EVENT_KEY_FM_CH_UP:
				audio_fm_freq_inc_set();
				break;
			case EVENT_KEY_FM_CH_DOWN:
				audio_fm_freq_dec_set();
				break;
			case EVENT_KEY_FM_CH_QUICK_SEL:
				audio_fm_freq_quick_set(*(INT32U*) ApQ_para);
				break;
			case EVENT_KEY_VOLUME_QUICK_SEL:
				temp_vol = (ApQ_para[1]<<8)|ApQ_para[0];
				if(temp_vol < 17){
					audio_vol_set(*(INT8U*) ApQ_para);
				}
				break;

			case MSG_APQ_AUDIO_EFFECT_MODE:
			case MSG_APQ_AUDIO_EFFECT_MENU:
			    if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) == 0){
			        ap_state_common_handling(msg_id);
			    }
			    break;

			case MSG_APQ_AUDIO_EFFECT_UP:
			case MSG_APQ_AUDIO_EFFECT_DOWN:
			    break;			

			case MSG_APQ_USER_CONFIG_STORE:
				ap_state_config_store();
				break;				
			case MSG_APQ_FILE_LOCK_DURING_RECORDING:
				if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)){
//					audio_effect_play(EFFECT_FILE_LOCK);
					ap_video_record_lock_current_file();
					ap_state_handling_icon_show_cmd(ICON_LOCKED, NULL, NULL);
				}
				break;			
			default:
		        ap_state_common_handling(msg_id);
				break;
		}
	}
	
	if (exit_flag == EXIT_BREAK) {
		state_video_record_exit();
	}
}

void state_video_record_exit(void)
{
	ap_state_handling_scroll_bar_exit(STATA_EXIT);
	ap_video_record_exit();
	DBG_PRINT("Exit video_record state\r\n");
}