#include "ap_audio_record.h"

#if (defined AUD_RECORD_EN) && (AUD_RECORD_EN==1)

static INT8U audio_record_sts;
static INT8U audio_record_display_timerid = 0xFF;
static INT8U audio_record_time_interval;
/* static */ INT8U audio_record_proc = 0; //wwj modify
static INT32U audio_record_bg_buff = 0;
static INT32U audio_record_image_back_buff = 0; //wwj add

static STOR_SERV_FILEINFO curr_audio_file_info;	//wwj add
static CHAR g_curr_audio_file_path[24];			//wwj add

void audio_calculate_left_recording_time_enable(void);
void audio_calculate_left_recording_time_disable(void);

INT8S ap_audio_record_init(INT32U prev_state)
{
	IMAGE_DECODE_STRUCT img_info;
	INT32U background_image_ptr;
	INT32U size;
	INT16U logo_fd;

	//load time interval setting
	audio_record_time_interval = ap_state_config_record_time_get();
	ap_state_config_record_time_set(3); //mean: 5min, 0~4 <--> 0, 1, 3, 5, 10
	audio_record_proc = 0; //wwj add

	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);		// Clear all icons
	
	if (ap_state_handling_storage_id_get() == NO_STORAGE) {
		ap_audio_record_sts_set(AUDIO_RECORD_UNMOUNT);
		ap_state_handling_icon_show_cmd(ICON_AUDIO_RECORD, ICON_NO_SD_CARD, NULL);
		audio_calculate_left_recording_time_disable();
	} else {
		ap_audio_record_sts_set(~AUDIO_RECORD_UNMOUNT);
		ap_state_handling_icon_show_cmd(ICON_AUDIO_RECORD, NULL, NULL);
		ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
		audio_calculate_left_recording_time_enable();
	}

#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
//	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_BAT_STS_REQ, NULL, NULL, MSG_PRI_NORMAL);
	ap_state_handling_current_bat_lvl_show();	//wwj modify
	ap_state_handling_current_charge_icon_show();	//wwj add
#endif

	audio_record_bg_buff = (INT32U) gp_malloc_align(TFT_WIDTH * TFT_HEIGHT * 2, 64);
	if (!audio_record_bg_buff) {
		DBG_PRINT("State audio record allocate jpeg output buffer fail.\r\n");
		return	STATUS_FAIL;
	}

	logo_fd = nv_open((INT8U *) "AUDIO_REC_BG.JPG");
	if (logo_fd != 0xFFFF) {
		size = nv_rs_size_get(logo_fd);
		background_image_ptr = (INT32U) gp_malloc(size);
		if (!background_image_ptr) {
			DBG_PRINT("State audio record allocate jpeg input buffer fail.[%d]\r\n", size);
			gp_free((void *) audio_record_bg_buff);
			return STATUS_FAIL;
		}
		if (nv_read(logo_fd, background_image_ptr, size)) {
			DBG_PRINT("Failed to read resource_header in ap_audio_record_init\r\n");
			gp_free((void *) background_image_ptr);
			gp_free((void *) audio_record_bg_buff);
			return STATUS_FAIL;
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
		img_info.output_buffer_pointer = audio_record_bg_buff;
		if (jpeg_buffer_decode_and_scale(&img_info) == STATUS_FAIL) {
			gp_free((void *) background_image_ptr);
			gp_free((void *) audio_record_bg_buff);
			DBG_PRINT("State audio record decode jpeg file fail.\r\n");
			return STATUS_FAIL;
		}
		gp_free((void *) background_image_ptr);
	} else {
		gp_memset((void *) audio_record_bg_buff, 80, TFT_WIDTH * TFT_HEIGHT * 2);
	}

	audio_record_image_back_buff = (INT32U) gp_malloc_align(TFT_WIDTH * 20 * 2, 64);
	if(!audio_record_image_back_buff) {
		gp_free((void *) audio_record_bg_buff);
		DBG_PRINT("state audio record allocate image back buffer fail.\r\n");
		return	STATUS_FAIL;
	}
	gp_memcpy((INT8S *)audio_record_image_back_buff, (INT8S *)(audio_record_bg_buff + TFT_WIDTH * (TFT_HEIGHT - 20) * 2), TFT_WIDTH * 20 * 2);

	//OSQPost(DisplayTaskQ, (void *) (audio_record_bg_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
	ap_audio_record_timer_draw();
	ap_audio_record_timer_start();
	return STATUS_OK;
}

void ap_audio_record_exit(INT32U next_state_msg)
{
	OS_Q_DATA OSQData;

	//store time interval setting
	ap_audio_record_timer_stop();
	ap_state_config_record_time_set(audio_record_time_interval);

	if(next_state_msg == MSG_APQ_MENU_KEY_ACTIVE) {
		msgQFlush(ApQ);
		msgQSend(ApQ, MSG_APQ_MENU_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
	}

	if (audio_record_sts & AUDIO_RECORD_BUSY) {
		struct stat_t file_stat;	//wwj add

		ap_audio_record_sts_set(~AUDIO_RECORD_BUSY);
		ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
		timer_counter_force_display(0);
//		OSQPost(DisplayTaskQ, (void *) (audio_record_bg_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
		ap_audio_record_timer_draw();

		audio_encode_stop();

		close(curr_audio_file_info.file_handle);
		curr_audio_file_info.file_handle = open((CHAR *) curr_audio_file_info.file_path_addr, O_RDONLY);
		fstat(curr_audio_file_info.file_handle, &file_stat);
		close(curr_audio_file_info.file_handle);
		if(file_stat.st_size == 0) {
			unlink((CHAR *) curr_audio_file_info.file_path_addr);
		}

		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	}
	audio_record_sts = 0;

	audio_calculate_left_recording_time_disable();
	ap_peripheral_auto_off_force_disable_set(0);	//wwj add, disable auto off
	ap_state_handling_led_on();//wwj add

	OSQQuery(DisplayTaskQ, &OSQData);
	while(OSQData.OSMsg != NULL) {
		OSTimeDly(3);
		OSQQuery(DisplayTaskQ, &OSQData);
	}

	if(audio_record_bg_buff) {
		gp_free((void *) audio_record_bg_buff);
		audio_record_bg_buff = NULL;
	}
	
	if(audio_record_image_back_buff) {
		gp_free((void *) audio_record_image_back_buff);
		audio_record_image_back_buff = NULL;
	}
}

void ap_audio_record_sts_set(INT8S sts)
{
	if (sts > 0) {
		audio_record_sts |= sts;
	} else {
		audio_record_sts &= sts;
	}
}

INT8U ap_audio_record_sts_get(void)
{
	return audio_record_sts;
}

void ap_audio_record_func_key_active(void)
{
	if (audio_record_proc == 1) {
        return;
    }
	
	if (audio_record_sts & AUDIO_RECORD_BUSY) {
		struct stat_t file_stat;	//wwj add

		//ap_audio_record_timer_stop();
		ap_audio_record_sts_set(~AUDIO_RECORD_BUSY);
		ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
		timer_counter_force_display(0);
		audio_calculate_left_recording_time_enable();
//		OSQPost(DisplayTaskQ, (void *) (audio_record_bg_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
		ap_audio_record_timer_draw();
		ap_peripheral_auto_off_force_disable_set(0);	//wwj add, disable auto off
		ap_state_handling_led_on();//wwj add

		audio_encode_stop();

		close(curr_audio_file_info.file_handle);
		curr_audio_file_info.file_handle = open((CHAR *) curr_audio_file_info.file_path_addr, O_RDONLY);
		fstat(curr_audio_file_info.file_handle, &file_stat);
		close(curr_audio_file_info.file_handle);
		if(file_stat.st_size == 0) {
			unlink((CHAR *) curr_audio_file_info.file_path_addr);
		}

		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	} else if (!(audio_record_sts & AUDIO_RECORD_UNMOUNT)) {
		INT64U  disk_free_size;

		disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
		if (disk_free_size <= CARD_FULL_MB_SIZE) {
		    ap_state_handling_str_draw_exit();
		    ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
            audio_calculate_left_recording_time_disable();
		} else {
			audio_record_proc = 1;
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_AUD_REQ, NULL, NULL, MSG_PRI_NORMAL);
		}
	}
}

void ap_audio_record_timer_start(void)
{
	if (audio_record_display_timerid == 0xFF) {
		audio_record_display_timerid = VIDEO_RECORD_CYCLE_TIMER_ID;
		sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_DISPLAY_DRAW_TIMER, audio_record_display_timerid, AUDIO_RECORD_DISPLAY_TIMER_INTERVAL);
	}
}

void ap_audio_record_timer_stop(void)
{
	if (audio_record_display_timerid != 0xFF) {
		sys_kill_timer(audio_record_display_timerid);
		audio_record_display_timerid = 0xFF;
	}
}

extern INT8U s_usbd_pin;
void ap_audio_record_timer_draw(void)
{
	if(s_usbd_pin == 0) {
		TIME_T	g_osd_time;

		if(audio_record_bg_buff && audio_record_image_back_buff) {
			gp_memcpy((INT8S *)(audio_record_bg_buff + TFT_WIDTH * (TFT_HEIGHT - 20) * 2), (INT8S *)audio_record_image_back_buff, TFT_WIDTH * 20 * 2);
		} else if(audio_record_bg_buff) {
			gp_memset((void *) audio_record_bg_buff, 80, TFT_WIDTH * TFT_HEIGHT * 2);
		}

		cal_time_get(&g_osd_time);
		cpu_draw_time_osd(g_osd_time, audio_record_bg_buff, RGB565_DRAW, STATE_AUDIO_RECORD & 0xF);
	}

	if(ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
		OSQPost(DisplayTaskQ, (void *) (audio_record_bg_buff|MSG_DISPLAY_TASK_WAV_TIME_DRAW));
	} else {
		OSQPost(DisplayTaskQ, (void *) (audio_record_bg_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
	}
}

INT8S ap_audio_record_reply_action(STOR_SERV_FILEINFO *file_info_ptr)
{
	MEDIA_SOURCE src;
	INT16S free_size = file_info_ptr->storage_free_size;

	if (file_info_ptr) {
		gp_memcpy((INT8S *) &curr_audio_file_info, (INT8S *) file_info_ptr, sizeof(STOR_SERV_FILEINFO));
		gp_memcpy((INT8S *) g_curr_audio_file_path, (INT8S *) curr_audio_file_info.file_path_addr, sizeof(g_curr_audio_file_path));
		curr_audio_file_info.file_path_addr = (INT32U)g_curr_audio_file_path;
	}

	src.type_ID.FileHandle = file_info_ptr->file_handle;
	src.type = SOURCE_TYPE_FS;
	src.Format.AudioFormat = WAV;
	if (src.type_ID.FileHandle>=0 && free_size>1) {
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_STOP, NULL, NULL, MSG_PRI_NORMAL);
		if (audio_encode_start(src, AUD_REC_SAMPLING_RATE, 0)) {
			audio_record_proc = 0;
			timer_counter_force_display(0);
			audio_calculate_left_recording_time_enable();
			ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
			//OSQPost(DisplayTaskQ, (void *) (audio_record_bg_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
			ap_audio_record_timer_draw();
			ap_peripheral_auto_off_force_disable_set(0);	//wwj add, disable auto off
			ap_state_handling_led_on();
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
			return STATUS_FAIL;
		}
 
		// Start audio record timer display
		ap_peripheral_auto_off_force_disable_set(0/*1*/);	//wwj add, disable auto off
		ap_state_handling_led_blink_on();//wwj add
		//ap_audio_record_timer_start();
		ap_state_handling_icon_show_cmd(ICON_REC, NULL, NULL);
		audio_calculate_left_recording_time_disable();
		timer_counter_force_display(1);
		ap_audio_record_sts_set(AUDIO_RECORD_BUSY);
	} else {
		ap_audio_record_sts_set(~AUDIO_RECORD_BUSY);

		if (src.type_ID.FileHandle >= 0) {
			// Card full
			close(src.type_ID.FileHandle);
			unlink((CHAR *) file_info_ptr->file_path_addr); 
			ap_state_handling_str_draw_exit();
	    	ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
			//OSTimeDly(30);
	    	//msgQSend(ApQ, MSG_APQ_POWER_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
		}

		audio_calculate_left_recording_time_disable();
		ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
		timer_counter_force_display(0);
	    msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL); //wwj add

	    audio_record_proc = 0;
		ap_state_handling_led_on();
		ap_peripheral_auto_off_force_disable_set(0);	//wwj add, disable auto off
	    return STATUS_FAIL;
	}
	//OSQPost(DisplayTaskQ, (void *) (audio_record_bg_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
	ap_audio_record_timer_draw();

#if 0
	// Restart MP3
	if ((audio_playing_state_get() == STATE_IDLE) || (audio_playing_state_get() == STATE_PAUSED)){
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	}
#endif

	audio_record_proc = 0;
	return STATUS_OK;
}

void audio_calculate_left_recording_time_enable(void)
{
	INT32U freespace, left_recording_time;
	
	freespace = vfsFreeSpace(MINI_DVR_STORAGE_TYPE);
	
	left_recording_time = freespace / (AUD_REC_SAMPLING_RATE*2);
	
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_LEFT_REC_TIME_DRAW | left_recording_time));
}

void audio_calculate_left_recording_time_disable(void)
{	
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_LEFT_REC_TIME_CLEAR));
}

// TBD: Need a timer to check audio encode status

#endif //#if (defined AUD_RECORD_EN) && (AUD_RECORD_EN==1)
