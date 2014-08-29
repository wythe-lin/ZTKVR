#include "ap_video_preview.h"

static INT8S video_preview_sts;
static void video_preview_card_full_handle(void);

#if DYNAMIC_QVALUE==1
static INT32U dynamicQ_bak;
#endif


void ap_video_preview_init(void)
{	
	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_QUEUE_INIT);
//	OSQPost(scaler_task_q, (void *) MSG_SCALER_TASK_PREVIEW_ON);	
	if (ap_state_handling_storage_id_get() == NO_STORAGE) {
		ap_state_handling_icon_show_cmd(ICON_CAPTURE, ICON_NO_SD_CARD, NULL);
	} else {
		ap_state_handling_icon_show_cmd(ICON_CAPTURE, NULL, NULL);
	}

#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
//	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_BAT_STS_REQ, NULL, NULL, MSG_PRI_NORMAL);
	ap_state_handling_current_bat_lvl_show();
	ap_state_handling_current_charge_icon_show();
#endif
    #if DYNAMIC_QVALUE==1
        dynamicQ_bak = video_encode_get_jpeg_quality(); // dominant add
    #endif

	if (ap_state_config_quality_get() == 0) {
		video_encode_set_jpeg_quality(90);
	} else if (ap_state_config_quality_get() == 1) {
		video_encode_set_jpeg_quality(80);
	} else {
		video_encode_set_jpeg_quality(50);
	}
}

void ap_video_preview_exit(void)
{
	video_preview_sts = 0;

    #if DYNAMIC_QVALUE==0  // dominant add    
	    video_encode_set_jpeg_quality(QUALITY_FACTOR);
    #else
        video_encode_set_jpeg_quality(dynamicQ_bak); // dominant add
    #endif
}

void ap_video_preview_sts_set(INT8S sts)
{
	if (sts > 0) {
		video_preview_sts |= sts;
	} else {
		video_preview_sts &= sts;
	}
}

INT8S ap_video_preview_sts_get(void)
{
	return	video_preview_sts;
}


void ap_video_preview_func_key_active(void)
{
    if (ap_state_handling_storage_id_get() == NO_STORAGE) {
    	ap_state_handling_str_draw(STR_NO_SD, 0xF800);
    	return;
    }
    
    if ((vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20)<=CARD_FULL_MB_SIZE)
    {
        DBG_PRINT ("Card full, key action avoid!!!\r\n");
        video_preview_card_full_handle();
        return;
    }

	if (video_preview_sts == 0) {
		ap_video_preview_sts_set(VIDEO_PREVIEW_BUSY);
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_PIC_REQ, NULL, NULL, MSG_PRI_NORMAL);
	}
}

void ap_video_preview_power_off_handle(void)
{
	if (!(video_preview_sts & VIDEO_PREVIEW_BUSY)) {
		ap_state_handling_power_off();
	}
}

void ap_video_preview_reply_action(STOR_SERV_FILEINFO *file_info_ptr)
{
	MEDIA_SOURCE src;
	INT32U file_path_addr = file_info_ptr->file_path_addr;
	INT16S free_size = file_info_ptr->storage_free_size;
	
	src.type_ID.FileHandle = file_info_ptr->file_handle;
	src.type = SOURCE_TYPE_FS;
	src.Format.VideoFormat = MJPEG;
	if (src.type_ID.FileHandle >=0 && free_size > 1) {
		if (ap_state_config_preview_get()) {
			OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_PIC_EFFECT);
		} else {
			OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_PIC_PREVIEW_EFFECT);
		}
		if (video_encode_capture_picture(src) != START_OK) {
			unlink((CHAR *) file_path_addr);
			DBG_PRINT("Capture Fail\r\n");
			//return STATUS_FAIL;
		} else {			
			ap_state_handling_led_flash_on();
			DBG_PRINT("Capture OK\r\n");
		}
	} else {
		ap_state_handling_str_draw_exit();
		if (src.type_ID.FileHandle >=0) {
			ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
			close(src.type_ID.FileHandle);
		} else {
			ap_state_handling_str_draw(STR_NO_SD, 0xF800);
		}
		ap_video_preview_sts_set(~VIDEO_PREVIEW_BUSY);
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	}
//	ap_video_preview_sts_set(~VIDEO_PREVIEW_BUSY);
	if ( (audio_playing_state_get() == STATE_IDLE) || (audio_playing_state_get() == STATE_PAUSED) ){
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
	}
}

void video_preview_card_full_handle(void)
{
    ap_state_handling_str_draw_exit();
    ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
    ap_video_preview_sts_set(~VIDEO_PREVIEW_BUSY);
    msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
}


