#include "state_thumbnail.h"

extern INT8U g_thumbnail_reply_action_flag; //wwj add

//	prototypes
void state_thumbnail_init(void);
void state_thumbnail_exit(INT32U next_state_msg);

void state_thumbnail_init(void)
{
	DBG_PRINT("Thumbnail state init enter\r\n");
	ap_thumbnail_init();
}

void state_thumbnail_entry(void *para)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	INT32U msg_id, search_type;
	INT16U /*search_type,*/ play_index, temp_vol;
	STAudioConfirm *audio_temp;

	state_thumbnail_init();
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
        		ap_thumbnail_sts_set(~THUMBNAIL_UNMOUNT);
        		ap_thumbnail_clean_frame_buffer();
				g_thumbnail_reply_action_flag = 0;
        		search_type = STOR_SERV_SEARCH_INIT;
        		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_THUMBNAIL_REQ, (void *) &search_type, sizeof(INT32U), MSG_PRI_NORMAL);
        		DBG_PRINT("[State Thumbnail Mount OK]\r\n");
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
				g_thumbnail_reply_action_flag = 1;
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_thumbnail_sts_set(THUMBNAIL_UNMOUNT);
        		ap_thumbnail_stop_handle();
        		ap_thumbnail_no_media_show(STR_NO_SD);
        		DBG_PRINT("[State Thumbnail Mount FAIL]\r\n");
        		break;
        	case MSG_APQ_POWER_KEY_ACTIVE:
        		ap_state_handling_power_off();
        		break;
        	case MSG_APQ_MODE:
				if(g_thumbnail_reply_action_flag) {
	        		OSQPost(StateHandlingQ, (void *) STATE_VIDEO_RECORD);
	        		exit_flag = EXIT_BREAK;
	        	}
        		break;
        	case MSG_APQ_FUNCTION_KEY_ACTIVE:
				if(g_thumbnail_reply_action_flag) {
	        		play_index = ap_thumbnail_func_key_active();
	        		OSQPost(StateHandlingQ, (void *) ((play_index<<16) | STATE_BROWSE) );
	        		exit_flag = EXIT_BREAK;
	        	}
        		break;
        	case MSG_STORAGE_SERVICE_BROWSE_REPLY:
				g_thumbnail_reply_action_flag = 1;
        		ap_thumbnail_reply_action((STOR_SERV_PLAYINFO *) ApQ_para);
        		break;
        	case MSG_APQ_NEXT_KEY_ACTIVE:
				if(g_thumbnail_reply_action_flag) {
	        		ap_thumbnail_next_key_active(ApQ_para[0]);
	        	}
        		break;
        	case MSG_APQ_PREV_KEY_ACTIVE:
				if(g_thumbnail_reply_action_flag) {
	        		ap_thumbnail_prev_key_active(ApQ_para[0]);
	        	}
        		break;
        	case MSG_APQ_CONNECT_TO_PC:
        		if (ap_state_config_usb_mode_get() == 0) {
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
        		ap_thumbnail_connect_to_pc();
        		break;
        	case MSG_APQ_DISCONNECT_TO_PC:
        		if (ap_state_config_usb_mode_get() == 0) {
        			sensor_clock_disable = 1;
        		}
        		else {
        			ap_state_handling_disconnect_to_pc();
        			ap_thumbnail_disconnect_to_pc();
        		}
        		break;
        	case MSG_APQ_SENSOR_CLOCK_DISABLED:
        		ap_state_handling_disconnect_to_pc();
        		OSTimeDly(10);
        		OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_ON);
        		ap_thumbnail_disconnect_to_pc();
        		break;
#if C_SCREEN_SAVER == CUSTOM_ON
			case MSG_APQ_KEY_IDLE:
        		ap_state_handling_lcd_backlight_switch(0);         	
        		break;
			case MSG_APQ_KEY_WAKE_UP:			
        		ap_state_handling_lcd_backlight_switch(1);        	
        		break;        		
#endif        		
        	case MSG_APQ_MENU_KEY_ACTIVE:
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

	if (exit_flag == EXIT_BREAK) {
		state_thumbnail_exit(msg_id);
	}
}

void state_thumbnail_exit(INT32U next_state_msg)
{
//	VIDEO_ARGUMENT arg;
	
	ap_thumbnail_exit();
	ap_state_handling_str_draw_exit(); //wwj add to prevent "NO SD card" from showing too long time
	if (next_state_msg == MSG_APQ_MODE) {
/*		arg.bScaler = 1;
		arg.TargetWidth = AVI_WIDTH;
		arg.TargetHeight = AVI_HEIGHT;
		arg.SensorWidth	= 640;
		arg.SensorHeight = 480;
		arg.DisplayWidth = AVI_WIDTH;
		arg.DisplayHeight = AVI_HEIGHT;
		arg.DisplayBufferWidth = TFT_WIDTH;
		arg.DisplayBufferHeight = TFT_HEIGHT;	
		arg.VidFrameRate = AVI_FRAME_RATE;
		arg.AudSampleRate = 8000;
		arg.OutputFormat = IMAGE_OUTPUT_FORMAT_YUYV;
		video_encode_preview_start(arg);
*/
		OSQPost(scaler_task_q, (void *) (R_TFT_FBI_ADDR|MSG_SCALER_TASK_PREVIEW_LOCK));
		OSQPost(scaler_task_q, (void *) (0xFFFFFF|MSG_SCALER_TASK_PACKER_ALLOCATE));
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
	DBG_PRINT("Exit Thumbnail state\r\n");
}