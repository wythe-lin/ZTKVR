#include "state_browse.h"

//	prototypes
void state_browse_init(INT32U prev_state, INT16U play_index);
void state_browse_exit(INT32U next_state_msg);
extern INT8U sensor_clock_disable;
//INT8U sensor_clock_disable = 0;
static INT8U mode_key_enabled = 0;

void state_browse_init(INT32U prev_state, INT16U play_index)
{
	DBG_PRINT("Browse state init enter\r\n");
	g_browser_reply_action_flag = 0; //wwj add
	if(prev_state != STATE_SETTING) {
		//video_encode_preview_stop();
		OSQPost(scaler_task_q, (void *) (0xFFFFFF|MSG_SCALER_TASK_PACKER_FREE));
//		sensor_clock_disable = 1;
	} else {
		ap_browse_init(prev_state, play_index);
		pre_audio_state = audio_playing_state_get();
	}
}

void state_browse_entry(void *para, INT16U play_index)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	INT32U msg_id, prev_state, search_type;
	INT16U /*search_type,*/ temp_vol;
	STAudioConfirm *audio_temp;
	INT8U  func_key_act = 0;

	prev_state = *((INT32U *) para);	
	state_browse_init(prev_state, play_index);
	while (exit_flag == EXIT_RESUME) {
		if (msgQReceive(ApQ, &msg_id, (void *) ApQ_para, AP_QUEUE_MSG_MAX_LEN) == STATUS_FAIL) {
			continue;
		}

		switch (msg_id) {
			case EVENT_APQ_ERR_MSG:
				audio_temp = (STAudioConfirm *)ApQ_para;
				if( (audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_FS) ) {
					ap_state_handling_icon_clear_cmd(ICON_PLAY, ICON_PAUSE, NULL);
					ap_state_handling_icon_show_cmd(ICON_PLAY, NULL, NULL);
					ap_browse_wav_stop();
					ap_browse_display_timer_draw();
	  				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL); //wwj add
				} else if ((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_APP_RS)){

#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
    #if DV185	//wwj add
//					gpio_write_io(SPEAKER_EN, DATA_LOW);	// mark by xyz - 2014.12.10
    #elif DV188
					gpx_rtc_write(8,0x08);
    #endif
#endif

				    if (func_key_act) {
						if(g_browser_reply_action_flag) { //wwj add
					        ap_browse_func_key_active();
					    }
				        func_key_act = 0;
				    }
				}else{
					audio_confirm_handler((STAudioConfirm *)ApQ_para);
				}
				break;
			case MSG_STORAGE_SERVICE_MOUNT:
        		ap_state_handling_str_draw_exit();
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_browse_sts_set(~BROWSE_UNMOUNT);
        		g_browser_reply_action_flag = 0;
        		search_type = STOR_SERV_SEARCH_INIT;
        		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_BROWSE_REQ, (void *) &search_type, sizeof(INT32U), MSG_PRI_NORMAL);
        		DBG_PRINT("[State Browse Mount OK]\r\n");
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
        		g_browser_reply_action_flag = 1;
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_browse_sts_set(BROWSE_UNMOUNT);
        		ap_browse_stop_handle();
        		ap_browse_no_media_show(STR_NO_SD);
        		DBG_PRINT("[State Browse Mount FAIL]\r\n");
        		break;
        	case MSG_APQ_FREE_PACKER_MEM_DONE:
        		ap_browse_init(prev_state, play_index);
        		pre_audio_state = audio_playing_state_get();
        		break;
        	case MSG_APQ_POWER_KEY_ACTIVE:    	
	        	ap_browse_exit();
        		ap_state_handling_power_off();
        		break;
        	case MSG_APQ_MENU_KEY_ACTIVE:
				if(g_browser_reply_action_flag) { //wwj add
	        		OSQPost(StateHandlingQ, (void *) STATE_SETTING);
	        		exit_flag = EXIT_BREAK;
	        	}
        		break;
        	case MSG_APQ_MODE:
				if(g_browser_reply_action_flag) { //wwj add
	        		OSQPost(StateHandlingQ, (void *) STATE_VIDEO_RECORD);
	        		exit_flag = EXIT_BREAK;
	        	} else if(prev_state == STATE_SETTING) {
	        		OSTimeDly(5);
				    msgQSend(ApQ, MSG_APQ_MODE, NULL, NULL, MSG_PRI_NORMAL);
	        	}
        		break;
        	case MSG_APQ_FUNCTION_KEY_ACTIVE:
				if(g_browser_reply_action_flag) { //wwj add
	        	    if (func_key_act == 0) {
	        	        ap_browse_func_key_active();
	        		}
	        	}
        		break;
        	case MSG_STORAGE_SERVICE_BROWSE_REPLY:
        		g_browser_reply_action_flag = 1;
        		ap_browse_reply_action((STOR_SERV_PLAYINFO *) ApQ_para);
        		break;
        	case MSG_APQ_NEXT_KEY_ACTIVE:
				if(g_browser_reply_action_flag) { //wwj add
	        		ap_browse_next_key_active(ApQ_para[0]);
	        		if ((audio_playing_state_get() == STATE_PAUSED) && (pre_audio_state == STATE_PLAY)) {

	        			#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
						#if DV185	//wwj add
//							gpio_write_io(SPEAKER_EN, DATA_LOW);	// mark by xyz - 2014.12.10
						#elif DV188
							gpx_rtc_write(8,0x08);
						#endif
						Chip_powerup();
					#endif

						audio_send_resume();
					}
				}
        		break;
        	case MSG_APQ_PREV_KEY_ACTIVE:
				if(g_browser_reply_action_flag) { //wwj add
	        		ap_browse_prev_key_active(ApQ_para[0]);
	        		if ((audio_playing_state_get() == STATE_PAUSED) && (pre_audio_state == STATE_PLAY)) {

						#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
						  #if DV185	//wwj add
//							gpio_write_io(SPEAKER_EN, DATA_LOW);	// mark by xyz - 2014.12.10
						  #elif DV188
							gpx_rtc_write(8,0x08);
						  #endif
						Chip_powerup();
						#endif

						audio_send_resume();
					}
				}
        		break;
        	case MSG_APQ_MJPEG_DECODE_END:
        		ap_browse_mjpeg_stop();
        		ap_browse_mjpeg_play_end();
	  			//OSTimeDly(2);
	  			if ((audio_playing_state_get() == STATE_IDLE) || (audio_playing_state_get() == STATE_PAUSED)) {
	  				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	  			}

        		if ((audio_playing_state_get() == STATE_PAUSED) && (pre_audio_state == STATE_PLAY)) {

					#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
					  #if DV185	//wwj add
//						gpio_write_io(SPEAKER_EN, DATA_LOW);	// mark by xyz - 2014.12.10
					  #elif DV188
						gpx_rtc_write(8,0x08);
					  #endif
					Chip_powerup();
					#endif

					audio_send_resume();
				}
        		break;
        	case MSG_APQ_CONNECT_TO_PC:
        		ap_browse_connect_to_pc();
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
        		break;
        	case MSG_APQ_DISCONNECT_TO_PC:
        		if (ap_state_config_usb_mode_get() == 0) {
        			sensor_clock_disable = 1;
        		}
        		else {
        			ap_state_handling_disconnect_to_pc();
        			ap_browse_disconnect_to_pc();
        		}
        		break;
        	case MSG_APQ_SENSOR_CLOCK_DISABLED:
        		ap_state_handling_disconnect_to_pc();
        		OSTimeDly(10);
        		OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_ON);
        		ap_browse_disconnect_to_pc();
        		break;
#if C_BATTERY_DETECT == CUSTOM_ON        	
        	case MSG_APQ_BATTERY_LVL_SHOW:
        		ap_state_handling_battery_icon_show(ApQ_para[0]);
        		if(g_browser_reply_action_flag){
	        		ap_browse_display_update_command();	// give DRAW command
	        	}
        		break;
        	case MSG_APQ_BATTERY_CHARGED_SHOW:
//        		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_SHOW | ICON_BATTERY_CHARGED));
				ap_state_handling_charge_icon_show(1);	//wwj modify
        		if(g_browser_reply_action_flag){
	        		ap_browse_display_update_command();	// give DRAW command
	        	}
        		break;
        	case MSG_APQ_BATTERY_CHARGED_CLEAR:
//        		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | ICON_BATTERY_CHARGED));
				ap_state_handling_charge_icon_show(0);	//wwj modify
        		if(g_browser_reply_action_flag){
	        		ap_browse_display_update_command();	// give DRAW command
	        	}        		
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
        		if(g_browser_reply_action_flag){
	        		ap_browse_display_update_command();	// give DRAW command
	        	}
				break;

			case EVENT_KEY_MEDIA_PLAY_PAUSE:
				if (ap_browse_sts_get() == 0) {

					#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
					  #if DV185	//wwj add
						if ((audio_playing_state_get() != STATE_PLAY)  && gpio_read_io(SPEAKER_EN)) {
//							gpio_write_io(SPEAKER_EN, DATA_LOW);	// mark by xyz - 2014.12.10
					  #elif DV188
						if (audio_playing_state_get() != STATE_PLAY) {
							gpx_rtc_write(8,0x08);
					  #endif
						Chip_powerup();
	  				}
	  				#endif

			    	audio_play_pause_process();
			    	pre_audio_state = audio_playing_state_get();
			    }
			    break;
			case EVENT_KEY_MEDIA_VOL_UP:
				audio_vol_inc_set();
				break;
			case EVENT_KEY_MEDIA_VOL_DOWN:
				audio_vol_dec_set();
				break;
			case EVENT_KEY_MEDIA_PREV:
				if (ap_browse_sts_get() == 0) {
					audio_prev_process();
				}
				break;
			case EVENT_KEY_MEDIA_NEXT:
				if (ap_browse_sts_get() == 0) {
					audio_next_process();
				}
				break;
			case EVENT_KEY_MEDIA_STOP:
				if (ap_browse_sts_get() == 0) {
					audio_send_stop();
					pre_audio_state = audio_playing_state_get();
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
				}
				break;
			case EVENT_KEY_MODE:
				audio_playing_mode_set_process();
				break;
			case EVENT_KEY_MEDIA_QUICK_SEL:
				if (ap_browse_sts_get() == 0) {
					audio_quick_select(*(INT32U*) ApQ_para);
				}
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

			case MSG_APQ_AUDIO_EFFECT_OK:
			    if ((ap_browse_sts_get() == 0) || (ap_browse_sts_get() & BROWSE_UNMOUNT)) {
					if(g_browser_reply_action_flag) {
				        func_key_act = 1;
				        ap_state_common_handling(MSG_APQ_AUDIO_EFFECT_OK);
					}
			    }
			    break;

			case MSG_APQ_DISPLAY_DRAW_TIMER:
				if(g_browser_reply_action_flag) {
					ap_browse_display_timer_draw();
				}
				break;

			case MSG_APQ_AUDIO_EFFECT_UP:
			case MSG_APQ_AUDIO_EFFECT_DOWN:
				break;

			case MSG_APQ_AUDIO_EFFECT_MODE:
			case MSG_APQ_AUDIO_EFFECT_MENU:
				mode_key_enabled = 1;
				break;

			default:
				break;
		}
	}

	if (exit_flag == EXIT_BREAK) {
		state_browse_exit(msg_id);
	}
}

void state_browse_exit(INT32U next_state_msg)
{
//	VIDEO_ARGUMENT arg;
	g_browser_reply_action_flag = 0;
	ap_browse_exit();
	ap_state_handling_str_draw_exit(); //wwj add to prevent "No SD card" from showing too long time
	if (next_state_msg != MSG_APQ_MENU_KEY_ACTIVE) {
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

  	if(mode_key_enabled == 1) {
  		audio_effect_play(EFFECT_CLICK);
  		mode_key_enabled = 0;
  	}

	if ((audio_playing_state_get() == STATE_PAUSED) && (pre_audio_state == STATE_PLAY)) {
		audio_send_resume();
	}
	DBG_PRINT("Exit browse state\r\n");
}