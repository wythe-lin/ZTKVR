#include "state_video_preview.h"

INT8U sensor_clock_disable = 0;
static INT8U mode_key_pressed = 0;
static INT32U preview_zoom_factor = 1;

//	prototypes
void state_video_preview_init(void);
void state_video_preview_exit(void);

extern void capture_mode_exit(void);
extern void preview_zoom_set(INT32U factor);

void state_video_preview_init(void)
{
	DBG_PRINT("video_preview state init enter\r\n");
	OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_OFF);
	ap_setting_value_set_from_user_config();
	OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_ON);
	ap_video_preview_init();
	ap_state_handling_scroll_bar_init();
	
	ap_music_update_icon_status();	
}

void state_video_preview_entry(void *para)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	//FP32 zoom_factor;
	INT32U msg_id;
	INT16U temp_vol;
	STAudioConfirm *audio_temp;

	//zoom_factor = 1;
	mode_key_pressed = 0;
	state_video_preview_init();
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
//						gpio_write_io(SPEAKER_EN, DATA_LOW);	// mark by xyz - 2014.12.10
					  #elif DV188
						gpx_rtc_write(8,0x08);
					  #endif
					#endif

				}else{
					audio_confirm_handler((STAudioConfirm *)ApQ_para);
				}
				break;
			case MSG_STORAGE_SERVICE_MOUNT:
				ap_state_handling_str_draw_exit();			
				ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_video_preview_sts_set(~VIDEO_PREVIEW_UNMOUNT);
        		ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
        		ap_music_update_icon_status();
        		DBG_PRINT("[Video Preview Mount OK]\r\n");
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_video_preview_sts_set(VIDEO_PREVIEW_UNMOUNT);
        		ap_state_handling_icon_show_cmd(ICON_NO_SD_CARD, NULL, NULL);
   				//ap_music_update_icon_status();
   				ap_state_handling_mp3_index_show_zero_cmd();
   				ap_state_handling_mp3_total_index_show_zero_cmd();
        		DBG_PRINT("[Video Preview Mount FAIL]\r\n");
        		break;
        	case MSG_APQ_SCROLL_BAR_END:
        		ap_state_handling_scroll_bar_exit(SCROLL_DISAPPEAR);
        		break;
#if C_ZOOM == CUSTOM_ON 
        	case MSG_APQ_NEXT_KEY_ACTIVE:
        	case MSG_APQ_PREV_KEY_ACTIVE:
				if((ap_video_preview_sts_get() & VIDEO_PREVIEW_BUSY) || mode_key_pressed) {
					break;
				}

        		ap_state_handling_zoom_active(&preview_zoom_factor, msg_id);
        		break;
#endif        		
        	case MSG_APQ_MENU_KEY_ACTIVE:      		
				if((ap_video_preview_sts_get() & VIDEO_PREVIEW_BUSY) || mode_key_pressed) {
					break;
				}
        		OSQPost(StateHandlingQ, (void *) STATE_SETTING);
        		exit_flag = EXIT_BREAK;
        		break;
        	case MSG_APQ_MODE:
				if(ap_video_preview_sts_get() & VIDEO_PREVIEW_BUSY) {
					break;
				}
        		sensor_clock_disable = 1;
        		break;
        	case MSG_APQ_SENSOR_CLOCK_DISABLED:
				sensor_clock_disable = 0;
        		mode_key_pressed = 0;
        		OSTimeDly(5);
//        		OSQPost(StateHandlingQ, (void *) STATE_BROWSE);

				preview_zoom_factor = 1;
				preview_zoom_set(preview_zoom_factor);
			    capture_mode_exit();

        		OSQPost(StateHandlingQ, (void *) STATE_BROWSE); //wwj modify
        		//OSQPost(StateHandlingQ, (void *) STATE_AUDIO_RECORD); //wwj modify
        		exit_flag = EXIT_BREAK;
        		break;
        	case MSG_APQ_FUNCTION_KEY_ACTIVE:
				if(mode_key_pressed == 0) {
	        		ap_video_preview_func_key_active();
	        	}
        		break;
        	case MSG_STORAGE_SERVICE_PIC_REPLY:
        		ap_video_preview_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
        		break;
        	case MSG_APQ_PREVIEW_EFFECT_END:
        		ap_video_preview_sts_set(~VIDEO_PREVIEW_BUSY);
        		break;
        	case MSG_APQ_CONNECT_TO_PC:
        		audio_send_stop();
        		ap_state_handling_connect_to_pc();
        		break;
        	case MSG_APQ_DISCONNECT_TO_PC:
	        	OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_OFF);
        		ap_setting_value_set_from_user_config();
        		OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_ON);
        		ap_state_handling_disconnect_to_pc();
        		ap_music_update_icon_status();
        		break;
        	case MSG_APQ_POWER_KEY_ACTIVE:
				if(ap_video_preview_sts_get() & VIDEO_PREVIEW_BUSY) { //wwj add
					break;
				}
        		sensor_clock_disable = 1;
				OSTimeDly(10);        		
        		ap_video_preview_power_off_handle();
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

			case MSG_STORAGE_SERVICE_TIMER_STOP_DONE:
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
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
				break;
			case EVENT_KEY_MODE:
				audio_playing_mode_set_process();
				break;
			case EVENT_KEY_MEDIA_QUICK_SEL:
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
			case MSG_APQ_USER_CONFIG_STORE:
				ap_state_config_store();
				break;			

			case MSG_APQ_AUDIO_EFFECT_UP:
			case MSG_APQ_AUDIO_EFFECT_DOWN:
			    //break;

			default:
				ap_state_common_handling(msg_id);
				break;
		}
	}
	
	if (exit_flag == EXIT_BREAK) {
		state_video_preview_exit();
	}
}

void state_video_preview_exit(void)
{
	ap_state_handling_scroll_bar_exit(STATA_EXIT);
	ap_video_preview_exit();
    ap_state_handling_mp3_all_index_clear_cmd();
	DBG_PRINT("Exit video_preview state\r\n");
}