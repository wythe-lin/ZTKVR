#include "state_setting.h"

//	prototypes
void state_setting_init(INT32U prev_state, INT8U *tag);
void state_setting_exit(void);

void state_setting_init(INT32U prev_state, INT8U *tag)
{
	DBG_PRINT("setting state init enter\r\n");
	ap_setting_init(prev_state, tag);
}

void state_setting_entry(void *para)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	INT32U msg_id, prev_state, prev_state1;
	INT8U curr_tag, sub_tag;
	INT16U temp_vol;
	STAudioConfirm *audio_temp;

	curr_tag = 0;
	sub_tag = 0xFF;
	prev_state = prev_state1 = *((INT32U *) para);
	state_setting_init(prev_state, &curr_tag);
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
				ap_state_handling_storage_id_set(ApQ_para[0]);
        		DBG_PRINT("[Setting Mount OK]\r\n");
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		DBG_PRINT("[Setting Mount FAIL]\r\n");
        		break;
        	case MSG_APQ_POWER_KEY_ACTIVE:
        		sensor_clock_disable = 1;
        		ap_setting_exit();
        		OSTimeDly(10);
        		ap_state_handling_power_off();
        		break;
        	case MSG_APQ_MENU_KEY_ACTIVE:
        		exit_flag = ap_setting_menu_key_active(&curr_tag, &sub_tag, &prev_state, &prev_state1);
        		if(exit_flag == EXIT_BREAK) {
        			OSTimeDly(5);
        		}
        		break;
        	case MSG_APQ_MODE:
    			exit_flag = ap_setting_mode_key_active(prev_state1, &sub_tag);
        		break;
        	case MSG_APQ_FUNCTION_KEY_ACTIVE:
        		ap_setting_func_key_active(&curr_tag, &sub_tag, prev_state);
        		break;
        	case MSG_APQ_NEXT_KEY_ACTIVE:
        	case MSG_APQ_PREV_KEY_ACTIVE:       	
        		ap_setting_direction_key_active(&curr_tag, &sub_tag, msg_id, prev_state);
        		break;
        	case MSG_STORAGE_SERVICE_BROWSE_REPLY:
        		ap_setting_reply_action(prev_state, &curr_tag, (STOR_SERV_PLAYINFO *) ApQ_para);
        		break;
        	case MSG_APQ_CONNECT_TO_PC:
        		if ((prev_state1 == STATE_BROWSE) && (ap_state_config_usb_mode_get() == 0)) {
        			OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_OFF);
        				#if VIDEO_ENCODE_USE_MODE == VIDEO_ENCODE_WITH_PPU_IRQ
					 		R_PPU_IRQ_EN |= 0x40;	//enable csi frame end irq
							R_PPU_IRQ_STATUS = 0x40;
						#elif VIDEO_ENCODE_USE_MODE == VIDEO_ENCODE_WITH_TG_IRQ
							R_CSI_TG_CTRL1 |= 0x0080;	//enable sensor clock out
							OSTimeDly(20);
							R_CSI_TG_CTRL0 |= 0x0001;	//enable sensor controller
							R_TGR_IRQ_EN  |= 0x1; //marty add
						#endif	
        		}
        		audio_send_stop();
        		ap_state_handling_connect_to_pc();
        		ap_setting_frame_buff_display();
        		break;
        	case MSG_APQ_DISCONNECT_TO_PC:
        		if ((prev_state1 == STATE_BROWSE) && (ap_state_config_usb_mode_get() == 0)) {
        			sensor_clock_disable = 1;
        		}
        		else {
        			ap_state_handling_disconnect_to_pc();
	        		ap_setting_frame_buff_display();
        		}
        		break;
        	case MSG_APQ_SENSOR_CLOCK_DISABLED:
        		ap_state_handling_disconnect_to_pc();
        		OSTimeDly(10);
        		OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_ON);
        		break;       		
			case MSG_STORAGE_SERVICE_FORMAT_REPLY:
				ap_setting_format_reply(&curr_tag, prev_state);
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
				break;
#if C_SCREEN_SAVER == CUSTOM_ON
			case MSG_APQ_KEY_IDLE:
        		ap_state_handling_lcd_backlight_switch(0);         	
        		break;
			case MSG_APQ_KEY_WAKE_UP:
        		ap_state_handling_lcd_backlight_switch(1);        	
        		break;        		
#endif
        	case MSG_APQ_INIT_THUMBNAIL:
        		OSQPost(StateHandlingQ, (void *) STATE_THUMBNAIL);
        		exit_flag = EXIT_BREAK;
        		break;
        	case MSG_APQ_SELECT_FILE_DEL_REPLY:
        		OSQPost(StateHandlingQ, (void *) (0xA5A50000 | STATE_BROWSE) );
        		OSTimeDly(5);//wait for display queue empty		
        		exit_flag = EXIT_BREAK;
        		break;

			case MSG_APQ_FILE_DEL_ALL_REPLY:
        	case MSG_APQ_FILE_LOCK_ONE_REPLY:
        	case MSG_APQ_FILE_LOCK_ALL_REPLY:
        	case MSG_APQ_FILE_UNLOCK_ONE_REPLY:
        	case MSG_APQ_FILE_UNLOCK_ALL_REPLY:
        		OSQPost(StateHandlingQ, (void *) STATE_BROWSE);
        		OSTimeDly(5);//wait for display queue empty		
        		exit_flag = EXIT_BREAK;
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
			default:
				ap_state_common_handling(msg_id);
				break;
		}
	}

	if (msg_id == MSG_APQ_MODE) {
		msgQFlush(ApQ);
	    msgQSend(ApQ, MSG_APQ_MODE, NULL, NULL, MSG_PRI_NORMAL);
	}
	
	if (exit_flag == EXIT_BREAK) {
		state_setting_exit();
	}
}

void state_setting_exit(void)
{
	ap_setting_exit();	
	DBG_PRINT("Exit setting state\r\n");
}