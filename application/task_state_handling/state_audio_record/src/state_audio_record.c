#include "state_audio_record.h"

#if (defined AUD_RECORD_EN) && (AUD_RECORD_EN==1)
//	prototypes
INT8S state_audio_record_init(INT32U prev_state);
void state_audio_record_exit(INT32U next_state_msg);

static INT8U audio_record_mem_free_done;
static INT8U g_during_switch_process = 0;

extern INT8U audio_record_proc;

INT8S state_audio_record_init(INT32U prev_state)
{
	DBG_PRINT("audio_record state init enter\r\n");
	
	if(prev_state != STATE_SETTING)
	{
		audio_record_mem_free_done = 0;
		OSQPost(scaler_task_q, (void *) (0xFFFFFF|MSG_SCALER_TASK_PACKER_FREE));
	} else {
		audio_encode_entrance();    // create audio task
		if (ap_audio_record_init(prev_state) == STATUS_OK) {
			return STATUS_OK;
		} else {
			return STATUS_FAIL;
		}
	}
	return STATUS_OK;
}

void state_audio_record_entry(void *para)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	INT32U msg_id, prev_state, audio_record_status_temp = 0, disk_free_size;
	INT16U temp_vol;
	INT8U switch_flag;
	INT8S before_time=-1;
	TIME_T	g_osd_time;

	prev_state = *((INT32U *) para);
	switch_flag = 0;
	state_audio_record_init(prev_state);
	while (exit_flag == EXIT_RESUME) {
		if (msgQReceive(ApQ, &msg_id, (void *) ApQ_para, AP_QUEUE_MSG_MAX_LEN) == STATUS_FAIL) {
			continue;
		}
		switch (msg_id) {
			case EVENT_APQ_ERR_MSG:
				audio_confirm_handler((STAudioConfirm *)ApQ_para);
				break;
			case MSG_STORAGE_SERVICE_MOUNT:
				ap_state_handling_storage_id_set(ApQ_para[0]);
				ap_state_handling_str_draw_exit();
        		ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
        		//ap_music_update_icon_status();
        		ap_audio_record_sts_set(~AUDIO_RECORD_UNMOUNT);
				audio_calculate_left_recording_time_enable();
        		ap_audio_record_timer_draw();
        		DBG_PRINT("[Audio Record Mount OK]\r\n");
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_state_handling_str_draw_exit();
        		ap_state_handling_icon_show_cmd(ICON_NO_SD_CARD, NULL, NULL);
 				//ap_state_handling_mp3_index_show_zero_cmd();
 				//ap_state_handling_mp3_total_index_show_zero_cmd();
        		ap_audio_record_sts_set(AUDIO_RECORD_UNMOUNT);
        		if (ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
        			// Stop audio recording
        			ap_audio_record_func_key_active();
        		}
				audio_calculate_left_recording_time_disable();
				ap_audio_record_timer_draw();
        		DBG_PRINT("[Audio Record Mount FAIL]\r\n");
        		break;
        	case MSG_APQ_FREE_PACKER_MEM_DONE:
        		audio_encode_entrance();    // create audio task
        		ap_audio_record_init(prev_state);
        		audio_record_mem_free_done = 1;
        		break;     
			/*
            case MSG_APQ_MENU_LONG_KEY_ACTIVE:  // dominant add
                //DBG_PRINT ("APQ PEND MSG_APQ_MENU_LONG_KEY_ACTIVE\r\n");    
                if(audio_record_mem_free_done == 1) 
                {      
                    if (ApQ_para[0]==0)
                    {
                        preview_rotate(0, 3);
                    } 
                    else if (ApQ_para[0]==1)
                    {
                        preview_rotate(1, 0);
                    } 
                }
                break; */

        	case MSG_APQ_MENU_KEY_ACTIVE:
				if((ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) || audio_record_proc) {
					break;
				}
        		if(audio_record_mem_free_done == 1) {
					OSQPost(StateHandlingQ, (void *) STATE_SETTING);
					exit_flag = EXIT_BREAK;
				}
        		break;

        	case MSG_APQ_NEXT_KEY_ACTIVE:
        	case MSG_APQ_PREV_KEY_ACTIVE:
        		break;

        	case MSG_APQ_MODE:
				if(audio_record_proc) {
					break;
				}

        		if(audio_record_mem_free_done == 1) {
	        		OSQPost(StateHandlingQ, (void *) STATE_BROWSE);
	        		audio_record_mem_free_done = 0;
	        		exit_flag = EXIT_BREAK;
	        	}
        		break;

        	case MSG_APQ_FUNCTION_KEY_ACTIVE:		// OK key pressed
        		if(audio_record_mem_free_done == 1) {
					if (ap_state_handling_storage_id_get() == NO_STORAGE) 
					{
						ap_state_handling_str_draw_exit();
						ap_state_handling_str_draw(STR_NO_SD, 0xF800);
						ap_audio_record_timer_draw();				
					} else {
						ap_audio_record_func_key_active();
					}
				}
        		break;

        	case MSG_STORAGE_SERVICE_AUD_REPLY:
        		ap_audio_record_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
        		switch_flag = 0;
        		break;

        	case MSG_APQ_RECORD_SWITCH_FILE:
        		if (!switch_flag) {
	        		g_during_switch_process = 1;
	        		switch_flag = 1;
	        		ap_audio_record_func_key_active();
	        		ap_audio_record_func_key_active();
	        	}
        		break;

        	case MSG_APQ_POWER_KEY_ACTIVE:
        		ap_audio_record_exit(msg_id);
        		ap_state_handling_power_off();
        		break;
        	case MSG_APQ_CONNECT_TO_PC:
        		audio_send_stop();		// Stop MP3
        		if (ap_audio_record_sts_get() == AUDIO_RECORD_BUSY) {
        			// Stop audio recording
        			ap_audio_record_func_key_active();
        		}
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

        		ap_state_handling_connect_to_pc();
        		break;
        	case MSG_APQ_DISCONNECT_TO_PC:
				if (ap_state_config_usb_mode_get() == 0) {
					sensor_clock_disable = 1;
				}
				else {
					ap_state_handling_disconnect_to_pc();
					ap_audio_record_timer_draw();
        		}
        		//ap_music_update_icon_status();		// Restore MP3 icons
				break;
        	case MSG_APQ_SENSOR_CLOCK_DISABLED:
        		ap_state_handling_disconnect_to_pc();
        		OSTimeDly(10);
        		ap_audio_record_timer_draw();
        		break;
#if C_BATTERY_DETECT == CUSTOM_ON
        	case MSG_APQ_BATTERY_LVL_SHOW:
        		ap_state_handling_battery_icon_show(ApQ_para[0]);
				ap_audio_record_timer_draw();
        		break;
        	case MSG_APQ_BATTERY_CHARGED_SHOW:
//        		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_SHOW | ICON_BATTERY_CHARGED));
				ap_state_handling_charge_icon_show(1);	//wwj modify
				ap_audio_record_timer_draw();
        		break;
        	case MSG_APQ_BATTERY_CHARGED_CLEAR:
//        		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | ICON_BATTERY_CHARGED));
				ap_state_handling_charge_icon_show(0);	//wwj modify
				ap_audio_record_timer_draw();
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
			case MSG_APQ_DISPLAY_DRAW_TIMER:
				if(ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
					audio_record_status_temp = audio_record_get_status();
					if(g_during_switch_process && (audio_record_status_temp == 0x00000002)){
						g_during_switch_process = 0;
						DBG_PRINT("finally audio_record_get_status = 0x%x\r\n", audio_record_status_temp);
					}
					else if( (!g_during_switch_process) && ((audio_record_status_temp == 0) || (audio_record_status_temp == 0x1)) )	//check if the recording status is STOP or not
					{
						timer_counter_force_display(0);
						audio_calculate_left_recording_time_disable();
						ap_peripheral_auto_off_force_disable_set(1/*0*/);
						ap_audio_record_sts_set(~AUDIO_RECORD_BUSY);
						ap_state_handling_led_on();
						ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);

						disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
						if (disk_free_size <= CARD_FULL_MB_SIZE) {
							ap_state_handling_str_draw_exit();
							ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
							//OSTimeDly(30);
					    	//msgQSend(ApQ, MSG_APQ_POWER_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
						}
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
						//ap_audio_record_timer_stop();
					}
				}
				cal_time_get(&g_osd_time);
				if(g_osd_time.tm_sec!=before_time) 
				{
					ap_audio_record_timer_draw();
					before_time = g_osd_time.tm_sec;
				}
				break;

			case MSG_APQ_AUDIO_ENCODE_ERR: //wwj add
				if(ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
					ap_audio_record_func_key_active();
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL);
				}
				break;

			case MSG_APQ_AUDIO_EFFECT_MENU:
				if(ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
					break;
				}
			case MSG_APQ_AUDIO_EFFECT_OK:
			case MSG_APQ_AUDIO_EFFECT_MODE:
				ap_state_common_handling(msg_id);
				break;

			default:
				break;
		}

	}

	if (exit_flag == EXIT_BREAK) {
		state_audio_record_exit(msg_id);
	}
}

void state_audio_record_exit(INT32U next_state_msg)
{
	ap_audio_record_exit(next_state_msg);
	audio_encode_exit();  // delete audio record task
	DBG_PRINT("Exit audio_record state\r\n");
}

#endif  //#if (defined AUD_RECORD_EN) && (AUD_RECORD_EN==1)
