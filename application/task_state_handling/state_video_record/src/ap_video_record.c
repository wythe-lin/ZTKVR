#include "ap_video_record.h"

	/*static*/ STOR_SERV_FILEINFO curr_file_info;	//wwj modify
	CHAR g_curr_file_name[24];			//wwj add

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	/*static*/ STOR_SERV_FILEINFO next_file_info; //wwj modify
	static INT8U cyclic_record_timerid;
	CHAR g_next_file_name[24];				//wwj add
#endif
static INT8S video_record_sts;
#if C_MOTION_DETECTION == CUSTOM_ON
	static INT8U motion_detect_timerid;
	static INT8U md_check_tick;

static INT8U g_lock_current_file_flag = 0;
//	prototypes
void ap_video_record_md_icon_clear_all(void);
void video_calculate_left_recording_time_enable(void);
void video_calculate_left_recording_time_disable(void);
#endif
static INT8U g_lock_current_file_flag = 0;

extern INT8U s_usbd_pin;

static void video_rec_card_full_handle(void);

INT16S drvl2_sdc_live_response(void) //marty add 
{
	return gpio_read_io(SD_CD_PIN);
}

void ap_video_record_init(void)
{
	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);
//	ap_state_handling_icon_show_cmd(ICON_VGA, NULL, NULL);		//wwj add
	if (ap_state_handling_storage_id_get() == NO_STORAGE) {
		ap_state_handling_icon_show_cmd(ICON_VIDEO_RECORD, ICON_NO_SD_CARD, NULL);
		ap_video_record_sts_set(VIDEO_RECORD_UNMOUNT);
	} else {
		ap_state_handling_icon_show_cmd(ICON_VIDEO_RECORD, NULL, NULL);
	}

	if(ap_state_config_voice_record_switch_get()) {
		ap_state_handling_icon_show_cmd(ICON_MIC_OFF, NULL, NULL);
	} else {
		ap_state_handling_icon_clear_cmd(ICON_MIC_OFF, NULL, NULL);
	}

	/*	//wwj mark
	if(ap_state_handling_night_mode_get()) {	//wwj add
		ap_state_handling_icon_show_cmd(ICON_NIGHT_MODE_ENABLED, NULL, NULL);
		ap_state_handling_icon_clear_cmd(ICON_NIGHT_MODE_DISABLED, NULL, NULL);
	} else {
		ap_state_handling_icon_show_cmd(ICON_NIGHT_MODE_DISABLED, NULL, NULL);
		ap_state_handling_icon_clear_cmd(ICON_NIGHT_MODE_ENABLED, NULL, NULL);
	} */

#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
//	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_BAT_STS_REQ, NULL, NULL, MSG_PRI_NORMAL);
	ap_state_handling_current_bat_lvl_show();	//wwj modify
	ap_state_handling_current_charge_icon_show();	//wwj add
#endif
#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	cyclic_record_timerid = 0xFF;
	next_file_info.file_handle = -1;
#endif
#if C_MOTION_DETECTION == CUSTOM_ON
	if (ap_state_config_md_get()) {
		ap_state_handling_icon_show_cmd(ICON_MD_STS_0, NULL, NULL);
		motion_detect_timerid = 0xFF;
		md_check_tick = 0;
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
		ap_video_record_sts_set(VIDEO_RECORD_MD);
	}
#endif
	if (s_usbd_pin) {
		ap_state_handling_icon_show_cmd(ICON_USB_CONNECT, NULL, NULL);
	}
	if (ap_state_handling_storage_id_get() == NO_STORAGE) {
		video_calculate_left_recording_time_disable();	//wwj add
	} else {
		video_calculate_left_recording_time_enable();
	}
}

void ap_video_record_exit(void)
{	
	struct stat_t file_stat;	//wwj add

#if C_MOTION_DETECTION == CUSTOM_ON
	ap_video_record_md_disable();
#endif
	//if (video_encode_status() == VIDEO_CODEC_PROCESSING) {
	if (video_record_sts & VIDEO_RECORD_BUSY) {
#if C_AUTO_DEL_FILE == CUSTOM_ON
		INT8U type = FALSE;		//disable timer for checking free space
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(/*INT32U*/INT8U), MSG_PRI_NORMAL);
#endif
        timer_counter_force_display(0); // dominant add, must disable timer osd
		ap_state_handling_led_on();//wwj add
		audio_effect_play(EFFECT_CLICK); //wwj add
		ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
		if (video_encode_stop() != START_OK) {
			close(curr_file_info.file_handle);
			unlink((CHAR *) curr_file_info.file_path_addr);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &curr_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
			DBG_PRINT("Video Record Stop Fail\r\n");
			//return STATUS_FAIL;
		} else { //wwj add
			curr_file_info.file_handle = open((CHAR *) curr_file_info.file_path_addr, O_RDONLY);
			fstat(curr_file_info.file_handle, &file_stat);
			close(curr_file_info.file_handle);
			if(file_stat.st_size < 512) {
				unlink((CHAR *) curr_file_info.file_path_addr);
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &curr_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
			} else if(g_lock_current_file_flag) {
				_setfattr((CHAR *) curr_file_info.file_path_addr, D_RDONLY);
				g_lock_current_file_flag = 0;
			}
		}
		DBG_PRINT("Video Record Stop\r\n");
		//chdir("C:\\DCIM");  // dominant mark
	}
	ap_video_clear_lock_flag();
#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	if (cyclic_record_timerid != 0xFF) {
		sys_kill_timer(cyclic_record_timerid);
		cyclic_record_timerid = 0xFF;
	}
	if (next_file_info.file_handle >= 0) {
		/*
		INT8U type = FALSE;		//delete file open handle
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_CYCLIC_REQ, &type, sizeof(INT8U), MSG_PRI_NORMAL);
		*/
		close(next_file_info.file_handle);
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &next_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
		unlink((CHAR *) next_file_info.file_path_addr);
		next_file_info.file_handle = -1;
	}
#endif
	if ( (audio_playing_state_get() == STATE_IDLE) || (audio_playing_state_get() == STATE_PAUSED) ){
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	}
	video_record_sts = 0;
	video_calculate_left_recording_time_disable();
	ap_peripheral_auto_off_force_disable_set(0); //wwj add
}

void ap_video_record_power_off_handle(void)
{
	ap_video_record_error_handle();
	video_calculate_left_recording_time_disable();	//wwj add
	ap_state_handling_power_off();
}

void ap_video_record_sts_set(INT8S sts)
{
	if (sts > 0) {
		video_record_sts |= sts;
	} else {
		video_record_sts &= sts;
	}
}

INT8S ap_video_record_sts_get(void)
{
	return	video_record_sts;
}

#if C_MOTION_DETECTION == CUSTOM_ON
void ap_video_record_md_tick(void)
{
	md_check_tick++;
	if (md_check_tick > 4) {
		md_check_tick = 0;
		if (motion_detect_timerid != 0xFF) {
			sys_kill_timer(motion_detect_timerid);
			motion_detect_timerid = 0xFF;
		}
		ap_video_record_error_handle();
	}
}

INT8S ap_video_record_md_active(void)
{
	if (video_record_sts & VIDEO_RECORD_UNMOUNT) {
		return TRUE;
	}
    
    if ((ap_state_config_record_time_get()==0) && (vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20)<=CARD_FULL_MB_SIZE) 
    {
        DBG_PRINT ("Card full, MD Auto Disable2!!!\r\n");
        ap_state_config_md_set(1);    
        ap_video_record_md_icon_clear_all();
    	ap_video_record_md_disable();
    	ap_state_config_md_set(0); 
        ap_video_record_error_handle();  // super stop MD record
        video_rec_card_full_handle();
        return TRUE;  // return true will stop md detect (Task level)
    }

	md_check_tick = 0;

	if (motion_detect_timerid == 0xFF) {
		motion_detect_timerid = VIDEO_PREVIEW_TIMER_ID;
		sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_MOTION_DETECT_TICK, motion_detect_timerid, MOTION_DETECT_CHECK_TIME_INTERVAL);
	}
	if ((video_record_sts & VIDEO_RECORD_BUSY) || !(video_record_sts & VIDEO_RECORD_MD)) {
		return TRUE;
	} else {
		return FALSE;
	}	
}

void ap_video_record_md_disable(void)
{
	if (ap_state_config_md_get()) {
		if (motion_detect_timerid != 0xFF) {
			sys_kill_timer(motion_detect_timerid);
			motion_detect_timerid = 0xFF;
		}
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_STOP, NULL, NULL, MSG_PRI_NORMAL);
		ap_video_record_sts_set(~VIDEO_RECORD_MD);
	}
}

void ap_video_record_md_icon_update(INT8U sts)
{
	if (ap_state_config_md_get()) {
		//ap_video_record_md_icon_clear_all();
		//ap_state_handling_icon_show_cmd(ICON_MD_STS_0 + sts, NULL, NULL);
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MD_ICON_SHOW | (ICON_MD_STS_0 + sts)));
	}
}

void ap_video_record_md_icon_clear_all(void)
{
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | (ICON_MD_STS_0 | (ICON_MD_STS_1<<8) | (ICON_MD_STS_2<<16))));
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | (ICON_MD_STS_3 | (ICON_MD_STS_4<<8) | (ICON_MD_STS_5<<16))));
}
#endif


void ap_video_record_func_key_active(INT32U event)
{
	struct stat_t file_stat;	//wwj add

    if (video_record_sts & VIDEO_RECORD_BUSY) 
    {
		if(event == MSG_APQ_FUNCTION_KEY_ACTIVE) { //wwj add
			if((curr_file_info.file_handle == -1)||(next_file_info.file_handle == -1)) {
				return;
			}
		}

#if C_MOTION_DETECTION == CUSTOM_ON
        if (ap_state_config_md_get() && (event == MSG_APQ_FUNCTION_KEY_ACTIVE)) 
        {
			ap_video_record_md_icon_clear_all();
			ap_video_record_md_disable();
			ap_state_config_md_set(0);
        }
#endif

        timer_counter_force_display(0); // dominant add,function key event, must disable timer OSD 
		ap_state_handling_led_on();//wwj add
		ap_peripheral_auto_off_force_disable_set(0);	//wwj add
		ap_state_handling_icon_clear_cmd(ICON_REC, ICON_LOCKED, NULL);
        video_calculate_left_recording_time_enable();
        if (video_encode_stop() != START_OK)
        {
			close(curr_file_info.file_handle);
			unlink((CHAR *) curr_file_info.file_path_addr);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &curr_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
			DBG_PRINT("Video Record Stop Fail\r\n");
			//return STATUS_FAIL;
		} else {  //wwj add
			curr_file_info.file_handle = open((CHAR *) curr_file_info.file_path_addr, O_RDONLY);
			fstat(curr_file_info.file_handle, &file_stat);
			close(curr_file_info.file_handle);
			if(file_stat.st_size < 512) {
				unlink((CHAR *) curr_file_info.file_path_addr);
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &curr_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
			}
			if(g_lock_current_file_flag) {
				_setfattr((CHAR *) curr_file_info.file_path_addr, D_RDONLY);
				g_lock_current_file_flag = 0;
			}
		}

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
		if (cyclic_record_timerid != 0xFF)
		{
			sys_kill_timer(cyclic_record_timerid);
			cyclic_record_timerid = 0xFF;
		}
		if (next_file_info.file_handle >= 0)
		{
			/*
			INT8U type = FALSE;		//delete file open handle
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_CYCLIC_REQ, &type, sizeof(INT8U), MSG_PRI_NORMAL);
			*/
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &next_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
			close(next_file_info.file_handle);
			unlink((CHAR *) next_file_info.file_path_addr);
			next_file_info.file_handle = -1;
		}
#endif
		ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
		if ( (audio_playing_state_get() == STATE_IDLE) || (audio_playing_state_get() == STATE_PAUSED) ){
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		}
		DBG_PRINT("Video Record Stop\r\n");
		//chdir("C:\\DCIM");  // dominant mark
    } else {
		if (video_record_sts == 0 || video_record_sts == VIDEO_RECORD_MD)   // Video record start
        {   
			/*
			if ((ap_state_config_record_time_get()==0)&&(vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20)<=CARD_FULL_MB_SIZE) 
			{
				DBG_PRINT ("Card full, key action avoid!!!\r\n");
				video_rec_card_full_handle();
				return;
			} */

			INT64U  disk_free_size;

			disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
			if (disk_free_size <= CARD_FULL_MB_SIZE) {
				if (ap_state_config_record_time_get()==0) {
					ap_state_handling_str_draw_exit();
					ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
					video_calculate_left_recording_time_disable();
				} else {
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREE_FILESIZE_CHECK, NULL, NULL, MSG_PRI_NORMAL);
				}
				return;
			}

			ap_video_record_sts_set(VIDEO_RECORD_BUSY);
			ap_peripheral_auto_off_force_disable_set(0/*1*/);	//wwj add, disable auto off
			curr_file_info.file_handle = -1;				//wwj add
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_REQ, NULL, NULL, MSG_PRI_NORMAL);
#if C_MOTION_DETECTION == CUSTOM_ON
    	    if (ap_state_config_md_get()) 
            {
				md_check_tick = 0;
                if (!(video_record_sts & VIDEO_RECORD_MD)) 
                {
					msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
					ap_video_record_sts_set(VIDEO_RECORD_MD);
				}
                if (motion_detect_timerid == 0xFF) 
                {
					motion_detect_timerid = VIDEO_PREVIEW_TIMER_ID;
					sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_MOTION_DETECT_TICK, motion_detect_timerid, MOTION_DETECT_CHECK_TIME_INTERVAL);
				}
			}
        }
        else if (video_record_sts & VIDEO_RECORD_UNMOUNT) 
        {
			if (ap_state_config_md_get()) {
				if (video_record_sts & VIDEO_RECORD_MD) {
					ap_video_record_md_icon_clear_all();
					ap_video_record_md_disable();
			    	ap_state_config_md_set(0); 		//wwj add
				} else {
					ap_state_handling_icon_show_cmd(ICON_MD_STS_0, NULL, NULL);
					msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
					ap_video_record_sts_set(VIDEO_RECORD_MD);
				}
			}
			ap_state_handling_str_draw_exit();
			ap_state_handling_str_draw(STR_NO_SD, 0xF800);
			ap_peripheral_auto_off_force_disable_set(0);	//wwj add
#endif		
		}
	}
}

void ap_video_record_start(void)
{
	ap_state_handling_str_draw_exit();
	ap_video_record_sts_set(VIDEO_RECORD_BUSY);
	ap_peripheral_auto_off_force_disable_set(0/*1*/);
	curr_file_info.file_handle = -1;
	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_REQ, NULL, NULL, MSG_PRI_NORMAL);
#if C_MOTION_DETECTION == CUSTOM_ON
    if (ap_state_config_md_get())
    {
		md_check_tick = 0;
        if (!(video_record_sts & VIDEO_RECORD_MD)) 
        {
			msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
			ap_video_record_sts_set(VIDEO_RECORD_MD);
		}
        if (motion_detect_timerid == 0xFF) 
        {
			motion_detect_timerid = VIDEO_PREVIEW_TIMER_ID;
			sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_MOTION_DETECT_TICK, motion_detect_timerid, MOTION_DETECT_CHECK_TIME_INTERVAL);
		}
	}
#endif
}


#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
void ap_video_record_cycle_reply_open(STOR_SERV_FILEINFO *file_info_ptr)
{
	STOR_SERV_FILEINFO temp_file_info;

	if (file_info_ptr->file_handle >= 0) {
		gp_memcpy((INT8S *) &temp_file_info, (INT8S *) &next_file_info, sizeof(STOR_SERV_FILEINFO));
		gp_memcpy((INT8S *) &next_file_info, (INT8S *) file_info_ptr, sizeof(STOR_SERV_FILEINFO));
		if (temp_file_info.file_handle >= 0) 
		{
			/*
			INT8U type = FALSE;		//delete file open handle
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_CYCLIC_REQ, &type, sizeof(INT8U), MSG_PRI_NORMAL);
			*/
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &next_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
			close(next_file_info.file_handle);
			unlink((CHAR *) next_file_info.file_path_addr);
			next_file_info.file_handle = -1;

			gp_memcpy((INT8S *) &next_file_info, (INT8S *)&temp_file_info, sizeof(STOR_SERV_FILEINFO));
		} else {
			gp_memcpy((INT8S *) g_next_file_name, (INT8S *)next_file_info.file_path_addr, sizeof(g_next_file_name));
			next_file_info.file_path_addr = (INT32U)g_next_file_name;
		}

	    if ((video_record_sts & VIDEO_RECORD_BUSY) == 0) {
			if (cyclic_record_timerid != 0xFF) 
			{
				sys_kill_timer(cyclic_record_timerid);
				cyclic_record_timerid = 0xFF;
			}
			if (next_file_info.file_handle >= 0) {
				/*
				INT8U type = FALSE;		//delete file open handle
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_CYCLIC_REQ, &type, sizeof(INT8U), MSG_PRI_NORMAL);
				*/
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &next_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
				close(next_file_info.file_handle);
				unlink((CHAR *) next_file_info.file_path_addr);
				next_file_info.file_handle = -1;
			}
		}
	} else {
		DBG_PRINT("Cyclic video record open file Fail\r\n");
	}
}

#if C_AUTO_DEL_FILE == CUSTOM_OFF
void ap_video_cyclic_record_del_reply(INT32S ret)
{
	INT8U temp[5] = {0, 1, 3, 5, 10};	//wwj add
	MEDIA_SOURCE src;
	INT32U time_interval, tag;
	INT8U type = TRUE;
	if (!ret) {
		video_calculate_left_recording_time_disable();
		if (ap_state_config_record_time_get() == 0) {
			tag = 255;
		} else {
			//tag = ap_state_config_record_time_get();
			tag = temp[ap_state_config_record_time_get()];	//wwj modify
		}
		time_interval = tag * VIDEO_RECORD_CYCLE_TIME_INTERVAL + 112;  // dominant add 64 to record more
		src.type_ID.FileHandle = next_file_info.file_handle;
		src.type = SOURCE_TYPE_FS;
		src.Format.VideoFormat = MJPEG;
		ap_state_handling_icon_show_cmd(ICON_REC, NULL, NULL);
		if (video_encode_start(src) != START_OK) {
            timer_counter_force_display(0);  // dominant add, vdo start fail
			ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
			close(src.type_ID.FileHandle);
			unlink((CHAR *) next_file_info.file_path_addr);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &next_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
			DBG_PRINT("Video Record Fail1\r\n");
            ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
            video_calculate_left_recording_time_enable();
            msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		} else {
			if (time_interval) {
				if (cyclic_record_timerid == 0xFF) {
					cyclic_record_timerid = VIDEO_RECORD_CYCLE_TIMER_ID;	//cyclic record reply action
					sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_CYCLIC_VIDEO_RECORD, cyclic_record_timerid, time_interval);
				}
			}
			next_file_info.file_handle = -1;	//open file handle
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_CYCLIC_REQ, &type, sizeof(/*INT32U*/INT8U), MSG_PRI_NORMAL);
		}
	} else {
		
	}
}
#else
  #if 0  // mark by dominant
   extern INT8U del_lock;		// "extern" isn't good...   Neal
   #endif

#endif

void ap_video_record_cycle_reply_action(void)
{
	MEDIA_SOURCE src;
#if C_AUTO_DEL_FILE == CUSTOM_OFF
	INT8U temp[5] = {0, 1, 3, 5, 10};	//wwj add
	INT32U size, tag, total_size;
#else
	INT8U type = TRUE;
#endif
	if (next_file_info.file_handle >= 0) {
		src.type_ID.FileHandle = next_file_info.file_handle;
		src.type = SOURCE_TYPE_FS;
		src.Format.VideoFormat = MJPEG;

		/*	//wwj mark
        if (ap_state_config_record_time_get()!=0) 
        {
            // always keep timer OSD Display and record red-spot
            timer_counter_force_display(1);  // dominant add, cyclic mode, no need stop timer count
        }
        else 
        {
	    	ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
        } */

#if C_AUTO_DEL_FILE == CUSTOM_ON
	#if C_FAST_SWITCH_FILE == CUSTOM_ON
		video_encode_fast_switch_stop_and_start(src);

		/*
		if(g_lock_current_file_flag > 0) {
			_setfattr((CHAR *) curr_file_info.file_path_addr, D_RDONLY);
			g_lock_current_file_flag--;
			if(!g_lock_current_file_flag){
				ap_state_handling_icon_clear_cmd(ICON_LOCKED, NULL, NULL);
			}
		} */

		if(g_lock_current_file_flag) {
			_setfattr((CHAR *) curr_file_info.file_path_addr, D_RDONLY);
			g_lock_current_file_flag = 0;
			ap_state_handling_icon_clear_cmd(ICON_LOCKED, NULL, NULL);
		}			

		gp_memcpy((INT8S *) &curr_file_info, (INT8S *) &next_file_info, sizeof(STOR_SERV_FILEINFO));
		gp_memcpy((INT8S *) g_curr_file_name, (INT8S *) curr_file_info.file_path_addr, sizeof(g_curr_file_name));
		curr_file_info.file_path_addr = (INT32U)g_curr_file_name;
	#else
		if (video_encode_stop() != START_OK) {
			close(curr_file_info.file_handle);
			unlink((CHAR *) curr_file_info.file_path_addr);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &curr_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
			DBG_PRINT("Video Record Stop Fail\r\n");
            ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
		} else {
			if(g_lock_current_file_flag > 0) {
				_setfattr((CHAR *) curr_file_info.file_path_addr, D_RDONLY);
				g_lock_current_file_flag--;
				if(!g_lock_current_file_flag){
					ap_state_handling_icon_clear_cmd(ICON_LOCKED, NULL, NULL);
				}
			}
			if (video_encode_start(src) != START_OK) {
				ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
				close(src.type_ID.FileHandle);
				unlink((CHAR *) next_file_info.file_path_addr);
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &next_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
				DBG_PRINT("Video Record Fail1\r\n");
	            ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
	            video_calculate_left_recording_time_enable();
	            msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
			}
		}
	#endif
		ap_state_handling_icon_show_cmd(ICON_REC, NULL, NULL); 
		next_file_info.file_handle = -1;	//open file handle
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_CYCLIC_REQ, &type, sizeof(/*INT32U*/INT8U), MSG_PRI_NORMAL);
#else
		if (cyclic_record_timerid != 0xFF) {
			sys_kill_timer(cyclic_record_timerid);
			cyclic_record_timerid = 0xFF;
		}
		if (ap_state_config_record_time_get() == 0) {
			tag = 255;
		} else {
			//tag = ap_state_config_record_time_get();
			tag = temp[ap_state_config_record_time_get()];	//wwj modify
		}
		total_size = vfsTotalSpace(MINI_DVR_STORAGE_TYPE) >> 20;
//		if( (total_size <= 1024) && ((ap_state_config_record_time_get() == 4) || (ap_state_config_record_time_get() == 5)) ){
		if( (total_size <= 1024) && ((ap_state_config_record_time_get() == 3) || (ap_state_config_record_time_get() == 4)) ){	//wwj modify
			size = 300;
		}else{
			size = (FRAME_SIZE*30*60*tag) >> 10;			
		}
		if (video_record_sts & VIDEO_RECORD_BUSY) {
			if (video_encode_stop() != START_OK) {
				close(curr_file_info.file_handle);
				unlink((CHAR *) curr_file_info.file_path_addr);
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &curr_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
				DBG_PRINT("Video Record Stop Fail\r\n");
                ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
			}
            msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, &size, sizeof(INT32U), MSG_PRI_NORMAL);
		}
#endif
	}
}
#endif

INT32S ap_video_record_del_reply(INT32S ret)
{
	if (ret == STATUS_FAIL) {
       #if C_AUTO_DEL_FILE == CUSTOM_OFF
		ap_state_handling_str_draw_exit();
		ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
		ap_video_record_error_handle();  // dominant midify
       #endif
		//ap_video_record_error_handle(); dominant, if del fail, no need stop del, background del will continue
		return STATUS_FAIL;
	} else {
		return STATUS_OK;
	}
}

void ap_video_record_error_handle(void)
{
	DBG_PRINT("Video record error handle.\r\n");
	if (video_record_sts & VIDEO_RECORD_BUSY) {   // dominant mark, in this function has better process 
		ap_video_record_func_key_active(NULL);
	}
}

#if C_AUTO_DEL_FILE == CUSTOM_ON
void ap_video_record_reply_action(STOR_SERV_FILEINFO *file_info_ptr)
{
	INT8U temp[5] = {0, 1, 3, 5, 10};	//wwj add
	MEDIA_SOURCE src;
	INT32U time_interval, time_interval_tag;
    INT32U disk_free_size;
    INT32U  disk_free_thread;
    INT8U type = TRUE;

    //time_interval_tag = ap_state_config_record_time_get();
    time_interval_tag = temp[ap_state_config_record_time_get()];	//wwj modify

	if (file_info_ptr) {
		gp_memcpy((INT8S *) &curr_file_info, (INT8S *) file_info_ptr, sizeof(STOR_SERV_FILEINFO));
		gp_memcpy((INT8S *) g_curr_file_name, (INT8S *) curr_file_info.file_path_addr, sizeof(g_curr_file_name));	//wwj add
		curr_file_info.file_path_addr = (INT32U)g_curr_file_name;													//wwj add
	} else {
		curr_file_info.storage_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
	}

	src.type_ID.FileHandle = curr_file_info.file_handle;
	src.type = SOURCE_TYPE_FS;
	src.Format.VideoFormat = MJPEG;
    
    if (time_interval_tag==0)
    {
        time_interval = 255 * VIDEO_RECORD_CYCLE_TIME_INTERVAL + 112;  // dominant add 64 to record more
    // FAT32:60min(<4GB), FAT16:30min(<2GB), but FAT16 SDC is always less than 2GB      
        bkground_del_disable(1);  // back ground auto delete disable      
    } else {
        bkground_del_disable(0);  // back ground auto delete enable
        time_interval = time_interval_tag * VIDEO_RECORD_CYCLE_TIME_INTERVAL + 112;  // dominant add 64 to record more
    }

	if (src.type_ID.FileHandle >=0) 
	{
		video_calculate_left_recording_time_disable();

		//if (ap_state_config_record_time_get()!=0)  //wwj mark
		{
		    timer_counter_force_display(1); // dominant add
		}

		ap_state_handling_icon_show_cmd(ICON_REC, NULL, NULL);
		if (video_encode_start(src) != START_OK) 
		{
		    timer_counter_force_display(0); // dominant add
		    ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
			ap_peripheral_auto_off_force_disable_set(0); //wwj add
		    close(src.type_ID.FileHandle);
		    unlink((CHAR *) curr_file_info.file_path_addr);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &curr_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
		    DBG_PRINT("Video Record Fail2\r\n");
		    ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
		    video_calculate_left_recording_time_enable();
		    msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		} 
		else 
		{
			ap_state_handling_led_blink_on();//wwj add

		    //if (time_interval) 
		    //{
				//open file handle
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_CYCLIC_REQ, &type, sizeof(/*INT32U*/INT8U), MSG_PRI_NORMAL);
				if (cyclic_record_timerid == 0xFF) {
					cyclic_record_timerid = VIDEO_RECORD_CYCLE_TIMER_ID;	//cyclic record reply action
					sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_CYCLIC_VIDEO_RECORD, cyclic_record_timerid, time_interval);
				}
		    //}

		//enable timer for checking free space
		    msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(/*INT32U*/INT8U), MSG_PRI_NORMAL);
		}
	} 

    if(curr_file_info.storage_free_size < bkground_del_thread_size_get())    
    {
        disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
        DBG_PRINT("\r\n[11Bkground Del Detect (DskFree: %d MB)]\r\n", disk_free_size);

        if (src.type_ID.FileHandle < 0)
        {
            DBG_PRINT("Video File Open Fail\r\n");
            if(drvl2_sdc_live_response() == 0) {
                disk_free_thread = CARD_FULL_MB_SIZE; // card exist
            } else {
                disk_free_thread = 0; // card not exist
            }
        } 
        else 
        {
            disk_free_thread = bkground_del_thread_size_get();
        }

    	if (disk_free_size < disk_free_thread)  // 當 free size 不夠, 就開始砍 
        {
            if (time_interval_tag!=0)  // 若現在是循環錄模式才能真的砍下去 
            {
                //msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, &free_size_check, sizeof(FREE_SIZE_CHECK), MSG_PRI_NORMAL);
        		time_interval_tag = 0xFFFFFFFF;
    	        msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VIDEO_FILE_DEL, &time_interval_tag, sizeof(INT32U), MSG_PRI_NORMAL);
            } 
            else  // size 不夠, 但又沒開循環錄影, 就是卡滿
            {
                close(src.type_ID.FileHandle);
                video_rec_card_full_handle();  
            }
        }
        else if (src.type_ID.FileHandle < 0) // file 打不開, 但 Size 還夠就是壞卡, 因為 size 夠確還開不起來我們認定為壞卡, 開始做 storage check 吧 
        {
			ap_peripheral_auto_off_force_disable_set(0); //wwj add
            ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
            msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
        }
	} 
    else if (curr_file_info.storage_free_size<=CARD_FULL_MB_SIZE)
    {
        if (bkground_del_disable_status_get()==1)  // size 不夠, 但又沒開循環錄影, 就是卡滿
        {
            close(src.type_ID.FileHandle);
            video_rec_card_full_handle();
        }
    } else if (src.type_ID.FileHandle < 0) {
		ap_peripheral_auto_off_force_disable_set(0); //wwj add
        ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
        msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL); //wwj add
    }
}
#else
void ap_video_record_reply_action(STOR_SERV_FILEINFO *file_info_ptr)
{
	MEDIA_SOURCE src;
	INT32U time_interval, tag;
    INT32U disk_free_size;
    INT32U  disk_free_thread;
	#if C_AUTO_DEL_FILE == CUSTOM_OFF
	INT32U total_size;
	FREE_SIZE_CHECK free_size_check;
	#endif

//	if (ap_state_config_record_time_get() == 6) {	//wwj modify
	if (ap_state_config_record_time_get() == 4) {
		tag = 10;
//	} else if (ap_state_config_record_time_get() == 7) {	//wwj modify
	} else if (ap_state_config_record_time_get() == 3) {
		tag = 5;//15;	//wwj modify
	} else if (ap_state_config_record_time_get() == 2) {	//wwj add
		tag = 3;
	} else {
		tag = 1;//ap_state_config_record_time_get();	//wwj modify
	}
	if (tag == 0) {
		time_interval = 0;
	} else {
		time_interval = tag * VIDEO_RECORD_CYCLE_TIME_INTERVAL + 112;  // dominant add 64 to record more
	}
	if (file_info_ptr) {
		gp_memcpy((INT8S *) &curr_file_info, (INT8S *) file_info_ptr, sizeof(STOR_SERV_FILEINFO));
	} else {
		curr_file_info.storage_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
	}
	src.type_ID.FileHandle = curr_file_info.file_handle;
	src.type = SOURCE_TYPE_FS;
	src.Format.VideoFormat = MJPEG;
    
	if (src.type_ID.FileHandle >=0) 
    {
        video_calculate_left_recording_time_disable();
        
        //if (ap_state_config_record_time_get()!=0)	//wwj mark
        {
            timer_counter_force_display(1); // dominant add
        }
            ap_state_handling_icon_show_cmd(ICON_REC, NULL, NULL);
        if (video_encode_start(src) != START_OK) {
            timer_counter_force_display(0); // dominant add
            ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
            close(src.type_ID.FileHandle);
            unlink((CHAR *) curr_file_info.file_path_addr);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_DEL_TABLE_ITEM, &curr_file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
            DBG_PRINT("Video Record Fail2\r\n");
            ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
            video_calculate_left_recording_time_enable();
            msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		} else {
#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
			INT8U type = TRUE;
			if (time_interval) {
				//open file handle
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_CYCLIC_REQ, &type, sizeof(/*INT32U*/INT8U), MSG_PRI_NORMAL);
				if (cyclic_record_timerid == 0xFF) {
					cyclic_record_timerid = VIDEO_RECORD_CYCLE_TIMER_ID;	//cyclic record reply action
					sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_CYCLIC_VIDEO_RECORD, cyclic_record_timerid, time_interval);
				}
			}
#endif
		}
	} 


    if(curr_file_info.storage_free_size < bkground_del_thread_size_get())    
    {
		if( (src.type_ID.FileHandle < 0) || (!time_interval) ){
			ap_state_handling_str_draw_exit();
			ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
			close(src.type_ID.FileHandle);
			//unlink((CHAR *) curr_file_info.file_path_addr);
			DBG_PRINT("Video Record Stop [NO free space]\r\n");
			ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VIDEO_FILE_DEL, &tag, sizeof(INT32U), MSG_PRI_NORMAL);		
	        msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
        } else {
			total_size = vfsTotalSpace(MINI_DVR_STORAGE_TYPE) >> 20;
//			if( (total_size <= 1024) && ((ap_state_config_record_time_get() == 4) || (ap_state_config_record_time_get() == 5)) ){
			if( (total_size <= 1024) && ((ap_state_config_record_time_get() == 3) || (ap_state_config_record_time_get() == 4)) ){ //wwj modify
				free_size_check.size = 300;
			}else{
		       	free_size_check.size = (FRAME_SIZE*30*60*tag) >> 10;
		    }
        	free_size_check.tag = 0x5A5A5A5A;
        	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, &free_size_check, sizeof(FREE_SIZE_CHECK), MSG_PRI_NORMAL);
        }
		
	}
}
#endif

void video_rec_card_full_handle(void)
{
    ap_state_handling_str_draw_exit();
    ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
    ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
	ap_peripheral_auto_off_force_disable_set(0);	//wwj add
    msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
}

void video_calculate_left_recording_time_enable(void)
{
	INT32U freespace, left_recording_time;
	
	freespace = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
	
	left_recording_time = (freespace << 1)/3;	//estimate one frame = 50KB, so one second video takes 50KB*30 = 1.5MB
	
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_LEFT_REC_TIME_DRAW | left_recording_time));
}

void video_calculate_left_recording_time_disable(void)
{	
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_LEFT_REC_TIME_CLEAR));
}

void ap_video_record_lock_current_file(void)
{
	g_lock_current_file_flag = 1;	//set file attribute only after close file
}

void ap_video_clear_lock_flag(void)
{
	g_lock_current_file_flag = 0;
	ap_state_handling_icon_clear_cmd(ICON_REC, ICON_LOCKED, NULL);
}
