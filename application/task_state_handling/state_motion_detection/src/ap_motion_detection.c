#include "ap_motion_detection.h"

static STOR_SERV_FILEINFO md_curr_file_info;
static INT16U motion_detect_timer_cnt;
static INT8S motion_detect_sts;
static INT8U motion_detect_timerid;

void ap_motion_detect_init(void)
{
	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);
	if (ap_state_handling_storage_id_get() == NO_STORAGE) {
		ap_state_handling_icon_show_cmd(ICON_MOTION_DETECT, ICON_NO_SD_CARD, NULL);
	} else {
		ap_state_handling_icon_show_cmd(ICON_MOTION_DETECT, NULL, NULL);
	}
#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_BAT_STS_REQ, NULL, NULL, MSG_PRI_NORMAL);
#endif	
	ap_motion_detect_sts_set(MOTION_DETECT_LOCK);
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
	motion_detect_timerid = 0xFF;
}

void ap_motion_detect_exit(void)
{
	if (video_encode_status() == VIDEO_CODEC_PROCESSING) {
#if C_AUTO_DEL_FILE == CUSTOM_ON
		INT8U type = FALSE;
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(INT32U), MSG_PRI_NORMAL);
#endif
		ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
		if (video_encode_stop() != START_OK) {
			close(md_curr_file_info.file_handle);
			unlink((CHAR *) md_curr_file_info.file_path_addr);
			DBG_PRINT("Motion Detection REC Stop Fail\r\n");
		}
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		chdir("C:\\DCIM");	
	}
	if (motion_detect_timerid != 0xFF) {
		sys_kill_timer(motion_detect_timerid);
		motion_detect_timerid = 0xFF;
	}
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_STOP, NULL, NULL, MSG_PRI_NORMAL);
	motion_detect_sts = 0;
}

void ap_motion_detect_power_off_handle(void)
{
	ap_motion_detect_error_handle();
	ap_state_handling_power_off();
}

void ap_motion_detect_sts_set(INT8S sts)
{
	if (sts > 0) {
		motion_detect_sts |= sts;
	} else {
		motion_detect_sts &= sts;
	}
}

void ap_motion_detect_func_key_active(void)
{
	if (video_encode_status() == VIDEO_CODEC_PROCESSING) {
#if C_AUTO_DEL_FILE == CUSTOM_ON
		INT8U type = FALSE;
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(INT32U), MSG_PRI_NORMAL);
#endif
		if (motion_detect_timerid != 0xFF) {
			sys_kill_timer(motion_detect_timerid);
			motion_detect_timerid = 0xFF;
		}
		ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
		if (video_encode_stop() != START_OK) {
			close(md_curr_file_info.file_handle);
			unlink((CHAR *) md_curr_file_info.file_path_addr);
			DBG_PRINT("Motion Detection REC Stop Fail\r\n");
		}
		ap_motion_detect_sts_set(~(MOTION_DETECT_CONTINUE|MOTION_DETECT_BUSY|MOTION_DETECT_LOCK));
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		chdir("C:\\DCIM");	
	} else {
		ap_state_handling_icon_clear_cmd(ICON_MOTION_DETECT, NULL, NULL);
		ap_state_handling_icon_show_cmd(ICON_MOTION_DETECT_START, NULL, NULL);
		ap_motion_detect_sts_set(~MOTION_DETECT_LOCK);
	}
}

void ap_motion_detect_video_record_active(void)
{
	INT32U time_interval, value;
	
	value = ap_state_config_record_time_get();
	if (value == 0) {
		time_interval = MOTION_DETECT_FILE_TIME_INTERVAL*2;
	} else if (value == 1) {
		time_interval = MOTION_DETECT_FILE_TIME_INTERVAL*5;
	} else {
		time_interval = MOTION_DETECT_FILE_TIME_INTERVAL*10;
	}	
	if (video_encode_status() != VIDEO_CODEC_PROCESSING) {
		if (motion_detect_sts == 0) {
			ap_motion_detect_sts_set(MOTION_DETECT_BUSY);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_REQ, NULL, NULL, MSG_PRI_NORMAL);
		}
	} else if (video_encode_status() == VIDEO_CODEC_PROCESSING) {
		if (motion_detect_timer_cnt > time_interval - MOTION_DETECT_FILE_TIME_COUNTDOWN) {
			ap_motion_detect_sts_set(MOTION_DETECT_CONTINUE);
		}
	}
}

void ap_motion_detect_reply_action(STOR_SERV_FILEINFO *file_info_ptr)
{
	MEDIA_SOURCE src;

	if (file_info_ptr) {
		gp_memcpy((INT8S *) &md_curr_file_info, (INT8S *) file_info_ptr, sizeof(STOR_SERV_FILEINFO));
	} else {
		md_curr_file_info.storage_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
	}
	src.type_ID.FileHandle = md_curr_file_info.file_handle;
	src.type = SOURCE_TYPE_FS;
	src.Format.VideoFormat = MJPEG;
	if (src.type_ID.FileHandle >=0 && md_curr_file_info.storage_free_size > bkground_del_thread_size_get()) {
		ap_state_handling_icon_show_cmd(ICON_REC, NULL, NULL);
		if (video_encode_start(src) != START_OK) {
			ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
			close(src.type_ID.FileHandle);
			unlink((CHAR *) md_curr_file_info.file_path_addr);
			DBG_PRINT("Motion Detection REC Fail\r\n");
		} else {
#if C_AUTO_DEL_FILE == CUSTOM_ON
			{
				INT8U type = TRUE;
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(INT32U), MSG_PRI_NORMAL);
			}
#endif
		}
		if (motion_detect_timerid == 0xFF) {
			motion_detect_timerid = VIDEO_RECORD_CYCLE_TIMER_ID;
			motion_detect_timer_cnt = 0;
			sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_MOTION_DETECT_TICK, motion_detect_timerid, MOTION_DETECT_TIME_TICK_INTERVAL);
		}
	} else {
#if C_AUTO_DEL_FILE == CUSTOM_ON
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VIDEO_FILE_DEL, NULL, NULL, MSG_PRI_NORMAL);
#else
		ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
		close(src.type_ID.FileHandle);
		unlink((CHAR *) md_curr_file_info.file_path_addr);
		DBG_PRINT("MD Video Record Stop [NO free space]\r\n");		
#endif
	}
}

void ap_motion_detect_timer_tick(void)
{
	INT32U time_interval, value;
	
	value = ap_state_config_record_time_get();
	if (value == 0) {
		time_interval = MOTION_DETECT_FILE_TIME_INTERVAL*2;
	} else if (value == 1) {
		time_interval = MOTION_DETECT_FILE_TIME_INTERVAL*5;
	} else {
		time_interval = MOTION_DETECT_FILE_TIME_INTERVAL*10;
	}	
	motion_detect_timer_cnt++;
	if (motion_detect_timer_cnt > time_interval) {
		motion_detect_timer_cnt = 0;
		msgQSend(ApQ, MSG_APQ_MOTION_DETECT_TICK_END, NULL, NULL, MSG_PRI_NORMAL);
	}
}

void ap_motion_detect_timer_tick_end(void)
{
#if C_AUTO_DEL_FILE == CUSTOM_ON
	INT8U type = FALSE;
	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(INT32U), MSG_PRI_NORMAL);
#endif
	if (motion_detect_timerid != 0xFF) {
		sys_kill_timer(motion_detect_timerid);
		motion_detect_timerid = 0xFF;
	}		
	ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
	if (video_encode_stop() != START_OK) {
		close(md_curr_file_info.file_handle);
		unlink((CHAR *) md_curr_file_info.file_path_addr);
		DBG_PRINT("Motion Detection REC Stop Fail\r\n");
	}		
	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	chdir("C:\\DCIM");
	if (motion_detect_sts & MOTION_DETECT_CONTINUE) {
		ap_motion_detect_sts_set(~(MOTION_DETECT_CONTINUE|MOTION_DETECT_BUSY|MOTION_DETECT_LOCK));
		if (video_encode_status() != VIDEO_CODEC_PROCESSING) {
			if (motion_detect_sts == 0) {
				ap_motion_detect_sts_set(MOTION_DETECT_BUSY);
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_REQ, NULL, NULL, MSG_PRI_NORMAL);
			}
		}
	} else {
		ap_motion_detect_sts_set(~MOTION_DETECT_BUSY);
	}
	ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
}

INT32S ap_motion_detect_del_reply(INT32S ret)
{
	if (ret == STATUS_FAIL) {
		ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
		ap_motion_detect_error_handle();
		return STATUS_FAIL;
	} else {
		return STATUS_OK;
	}
}

void ap_motion_detect_error_handle(void)
{
	if (motion_detect_sts & MOTION_DETECT_BUSY) {
		ap_motion_detect_func_key_active();
	}
}