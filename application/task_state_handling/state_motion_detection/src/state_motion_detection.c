#include "state_motion_detection.h"

//	prototypes
void state_motion_detect_init(void);
void state_motion_detect_exit(void);

void state_motion_detect_init(void)
{
	DBG_PRINT("motion_detect state init enter\r\n");
	ap_motion_detect_init();
	ap_state_handling_scroll_bar_init();
}

void state_motion_detect_entry(void *para)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	FP32 zoom_factor;
	INT32U msg_id;

	zoom_factor = 1;
	state_motion_detect_init();
	while (exit_flag == EXIT_RESUME) {
		if (msgQReceive(ApQ, &msg_id, (void *) ApQ_para, AP_QUEUE_MSG_MAX_LEN) == STATUS_FAIL) {
			continue;
		}

		switch (msg_id) {
			case MSG_STORAGE_SERVICE_MOUNT:
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_motion_detect_sts_set(~MOTION_DETECT_UNMOUNT);
        		ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
        		DBG_PRINT("[Motion Detection Mount OK]\r\n");
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		ap_motion_detect_sts_set(MOTION_DETECT_UNMOUNT);
        		ap_state_handling_icon_show_cmd(ICON_NO_SD_CARD, NULL, NULL);
        		DBG_PRINT("[Motion Detection Mount FAIL]\r\n");
        		break;
        	case MSG_APQ_SCROLL_BAR_END:
        		ap_state_handling_scroll_bar_exit(SCROLL_DISAPPEAR);
        		break;
        	case MSG_APQ_NEXT_KEY_ACTIVE:
        	case MSG_APQ_PREV_KEY_ACTIVE:
        		ap_state_handling_zoom_active((INT32U *)&zoom_factor, msg_id);
        		break;
        	case MSG_APQ_POWER_KEY_ACTIVE:
        		ap_motion_detect_power_off_handle();
        		break;
        	case MSG_APQ_MODE:
        		OSQPost(StateHandlingQ, (void *) STATE_BROWSE);
        		exit_flag = EXIT_BREAK;
        		break;
        	case MSG_APQ_FUNCTION_KEY_ACTIVE:
        		ap_motion_detect_func_key_active();
        		break;
        	case MSG_APQ_MOTION_DETECT_ACTIVE:
        		ap_motion_detect_video_record_active();
        		break;
        	case MSG_APQ_MOTION_DETECT_TICK:
        		ap_motion_detect_timer_tick();
        		break;
        	case MSG_APQ_MOTION_DETECT_TICK_END:
        		ap_motion_detect_timer_tick_end();
        		break;
        	case MSG_STORAGE_SERVICE_VID_REPLY:
        		ap_motion_detect_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
        		break;
#if C_AUTO_DEL_FILE == CUSTOM_ON			
			case MSG_APQ_VIDEO_FILE_DEL_REPLY:
				if (!ap_motion_detect_del_reply(*((INT32S *) ApQ_para))) {
					ap_motion_detect_reply_action(0);
				}				
				break;
#endif        		
			case MSG_APQ_CONNECT_TO_PC:
        		ap_motion_detect_error_handle();
        		ap_state_handling_connect_to_pc();
        		break;
        	case MSG_APQ_DISCONNECT_TO_PC:
        		ap_state_handling_disconnect_to_pc();
        		break;
        	case MSG_APQ_AVI_PACKER_ERROR:
        		ap_motion_detect_error_handle();
        		break;
#if C_BATTERY_DETECT == CUSTOM_ON        	
        	case MSG_APQ_BATTERY_LVL_SHOW:
        		ap_state_handling_battery_icon_show(ApQ_para[0]);
        		break;
#endif        		
#if C_SCREEN_SAVER == CUSTOM_ON
			case MSG_APQ_KEY_IDLE:
        		ap_state_handling_lcd_backlight_close();        	
        		break;
#endif        		
			default:
				break;
		}

	}

	if (exit_flag == EXIT_BREAK) {
		state_motion_detect_exit();
	}
}

void state_motion_detect_exit(void)
{
	ap_state_handling_scroll_bar_exit(STATA_EXIT);
	ap_motion_detect_exit();
	DBG_PRINT("Exit motion_detect state\r\n");
}