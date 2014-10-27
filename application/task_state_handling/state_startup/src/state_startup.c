#include "state_startup.h"

//	prototypes
void state_startup_init(void);
void state_startup_exit(void);

INT8U  audio_done = 0;

extern INT8U s_usbd_pin;	// "extern" isn't good...   Neal

void state_startup_init(void)
{
	DBG_PRINT("Startup state init enter\r\n");
	ap_startup_init();

/* #BEGIN# add by xyz - 2014.10.27 */
#if defined(DVR516_CFG) && (DVR516_CFG == 1)
  	gpio_init_io(IO_G5, GPIO_OUTPUT);
  	gpio_set_port_attribute(IO_G5, ATTRIBUTE_HIGH);
  	gpio_write_io(IO_G5, DATA_HIGH);
#endif
/* #END# add by xyz - 2014.10.27 */
}

void state_startup_entry(void *para)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	INT32U msg_id;
	INT8U  dsp_done=0;
	INT8U  scan_done=0;
	STAudioConfirm *audio_confirm;

	//state_startup_init();
	while (exit_flag == EXIT_RESUME) {
		if (msgQReceive(ApQ, &msg_id, (void *) ApQ_para, AP_QUEUE_MSG_MAX_LEN) == STATUS_FAIL) {
			continue;
		}
		switch (msg_id) {
#if C_VIDEO_PREVIEW == CUSTOM_ON
			case MSG_APQ_DISPLAY_TASK_READY:
				//exit_flag = EXIT_BREAK;
				state_startup_init();
				dsp_done = 1;
				if (dsp_done && scan_done && audio_done==2) {
					exit_flag = EXIT_BREAK;
				}	
				break;
#endif				
			case EVENT_APQ_ERR_MSG:
			    audio_confirm = (STAudioConfirm*) ApQ_para;
			    if (audio_confirm->result_type == MSG_AUD_PLAY_RES && audio_confirm->result != AUDIO_ERR_NONE) {
			        audio_done++;
			    }
			    if (dsp_done && scan_done && audio_done==2) {
					exit_flag = EXIT_BREAK;
				}
			    break;
			case MSG_STORAGE_SERVICE_MOUNT:
				ap_state_handling_storage_id_set(ApQ_para[0]);
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		break;
        	case MSG_APQ_POWER_KEY_ACTIVE:
        		ap_state_handling_power_off();
        		break;
        	case MSG_APQ_CONNECT_TO_PC:
        		ap_state_handling_connect_to_pc();
        		break;
        	case MSG_APQ_DISCONNECT_TO_PC:
        		ap_state_handling_disconnect_to_pc();
        		break;
        	case MSG_SYS_STORAGE_SCAN_DONE:
        		if (storage_sd_upgrade_file_flag_get() != 2) {
        			ap_music_init();
        			if (s_usbd_pin == 0) {
        				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
        			}
        			if( (audio_present) && (s_usbd_pin == 0) ){
						audio_play_process();
					}
				}
				scan_done = 1;
				if (dsp_done && scan_done && audio_done==2) {
					exit_flag = EXIT_BREAK;
				}	
        		break;
#if C_BATTERY_DETECT == CUSTOM_ON        	
        	case MSG_APQ_BATTERY_LVL_SHOW:
        		ap_state_handling_battery_icon_show(ApQ_para[0]);
        		break;
        	case MSG_APQ_BATTERY_CHARGED_SHOW:	//wwj add
				ap_state_handling_charge_icon_show(1);
        		break;
        	case MSG_APQ_BATTERY_CHARGED_CLEAR:	//wwj add
				ap_state_handling_charge_icon_show(0);
        		break;        		
#endif
			default:
				break;
		}
		
	}
#if C_VIDEO_PREVIEW == CUSTOM_ON
	if (exit_flag == EXIT_BREAK) {
#else
	{
#endif	
		state_startup_exit();
	}
}

void state_startup_exit(void)
{
    ap_state_firmware_upgrade();
    ap_startup_exit();

	#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
	  #if DV185	//wwj add
		gpio_write_io(SPEAKER_EN, DATA_LOW);
	  #elif DV188
		gpx_rtc_write(8,0x08);
	  #endif
	#endif

  #if USB_PHY_SUSPEND == 1
	if (s_usbd_pin) 
  #endif
	{
		OSQPost(USBAPPTaskQ, (void *) MSG_USBD_INITIAL);
	}
    
    DBG_PRINT("Exit Startup state\r\n");
    OSQPost(StateHandlingQ, (void *) STATE_VIDEO_RECORD);
}