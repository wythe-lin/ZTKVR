#include "video_encoder.h"

/* for debug */
#define DEBUG_VIDEO_ENCODER	1
#if DEBUG_VIDEO_ENCODER
    #include "gplib.h"
    #define _dmsg(x)		print_string x
#else
    #define _dmsg(x)
#endif

/* */
void video_encode_entrance(void)
{
	INT32S nRet;

	avi_encode_init();
	nRet = avi_encode_state_task_create(AVI_ENC_PRIORITY);
	if(nRet < 0)
    		DBG_PRINT("avi_encode_state_task_create fail !!!");
    	
	nRet = scaler_task_create(SCALER_PRIORITY);
	if(nRet < 0)
    		DBG_PRINT("scaler_task_create fail !!!");
    
	nRet = video_encode_task_create(JPEG_ENC_PRIORITY);
	if(nRet < 0)
    		DBG_PRINT("video_encode_task_create fail !!!");
    	
	nRet = avi_adc_record_task_create(AUD_ENC_PRIORITY);
	if(nRet < 0)
    		DBG_PRINT("avi_adc_record_task_create fail !!!");
    	
	DBG_PRINT("avi encode all task create success!!!\r\n");
}

void video_encode_exit(void)
{   
	INT32S nRet;
	
        if(video_encode_status() == VIDEO_CODEC_PROCESSING)
        {   
            video_encode_stop();
        }
   	
   	video_encode_preview_stop();
   	
    nRet = avi_encode_state_task_del();
	if(nRet < 0)
		DBG_PRINT("avi_encode_state_task_del fail !!!");
		
	nRet = scaler_task_del();
	if(nRet < 0)
		DBG_PRINT("scaler_task_del fail !!!");
		
	nRet = video_encode_task_del();
	if(nRet < 0)
		DBG_PRINT("video_encode_task_del fail !!!");
		
	nRet = avi_adc_record_task_del();
	if(nRet < 0)
		DBG_PRINT("avi_adc_record_task_del fail !!!");
	
	DBG_PRINT("avi encode all task delete success!!!\r\n");	
}


CODEC_START_STATUS video_encode_preview_start(VIDEO_ARGUMENT arg)
{
	INT32S nRet;

	_dmsg((GREEN "[S]: video_encode_preview_start()\r\n" NONE));

// ### for debug - xyz #########################
#if 1 //(zt_resolution() < ZT_HD_SCALED)
	if (ap_state_config_voice_record_switch_get() == 0) {	//wwj add
	    pAviEncAudPara->audio_format = AVI_ENCODE_AUDIO_FORMAT;
	} else {
	    pAviEncAudPara->audio_format = 0;
	}
#else
	pAviEncAudPara->audio_format = 0;
#endif
// ### for debug - xyz #########################

	pAviEncAudPara->channel_no	  = 1; //mono
	pAviEncAudPara->audio_sample_rate = arg.AudSampleRate;
    
	pAviEncVidPara->video_format	  = AVI_ENCODE_VIDEO_FORMAT;
	pAviEncVidPara->dwScale		  = arg.bScaler;
	pAviEncVidPara->dwRate		  = arg.VidFrameRate;
    
	avi_encode_set_display_format(arg.OutputFormat);
	avi_encode_set_sensor_format(BITMAP_YUYV);
    
	pAviEncVidPara->sensor_capture_width  = arg.SensorWidth;
	pAviEncVidPara->sensor_capture_height = arg.SensorHeight;
	pAviEncVidPara->encode_width	      = arg.TargetWidth;
	pAviEncVidPara->encode_height	      = arg.TargetHeight;
	pAviEncVidPara->display_width	      = arg.DisplayWidth;
	pAviEncVidPara->display_height	      = arg.DisplayHeight;
    
	_dmsg((GREEN "[D]: width & height - ")); 
	if (arg.DisplayBufferWidth == 0) {
		arg.DisplayBufferWidth = arg.DisplayWidth;
		_dmsg(("1 "));
	} else if (arg.DisplayWidth > arg.DisplayBufferWidth) {
		arg.DisplayWidth = arg.DisplayBufferWidth;
		_dmsg(("2 "));
	}
	_dmsg(("3 "));

	if (arg.DisplayBufferHeight == 0) {
    		arg.DisplayBufferHeight = arg.DisplayHeight;
		_dmsg(("4 "));
	} else if (arg.DisplayHeight > arg.DisplayBufferHeight) {
    		arg.DisplayHeight = arg.DisplayBufferHeight;
		_dmsg(("5 "));
	}
	_dmsg(("6\r\n" NONE)); 
	_dmsg((GREEN "[D]: display buffer = %0d x %0d\r\n" NONE, arg.DisplayBufferWidth, arg.DisplayBufferHeight));

	pAviEncVidPara->display_buffer_width  = arg.DisplayBufferWidth;
	pAviEncVidPara->display_buffer_height = arg.DisplayBufferHeight;
	avi_encode_set_display_scaler(); 
	nRet = vid_enc_preview_start();
   	if (nRet < 0) {
		_dmsg((GREEN "[E]: video_encode_preview_start(), fail\r\n" NONE));
		return CODEC_START_STATUS_ERROR_MAX;
	}

	_dmsg((GREEN "[E]: video_encode_preview_start(), pass\r\n" NONE));
	return START_OK;
}

CODEC_START_STATUS video_encode_preview_stop(void)
{
    INT32S result;
    
    result = vid_enc_preview_stop();   
    if(result < 0)
    	return CODEC_START_STATUS_ERROR_MAX;
    	
    return START_OK; 	
}

CODEC_START_STATUS video_encode_start(MEDIA_SOURCE src)
{
	INT32S			nRet;
	CODEC_START_STATUS	ret_status;
	
	ret_status = START_OK;
    
	if (src.type == SOURCE_TYPE_FS) {
		pAviEncPara->source_type = SOURCE_TYPE_FS;
	} else if (src.type == SOURCE_TYPE_USER_DEFINE) {
		pAviEncPara->source_type = SOURCE_TYPE_USER_DEFINE;
	} else { 
		ret_status =  RESOURCE_WRITE_ERROR;
		goto VDO_START_END; 
	}
  	
	if (src.type_ID.FileHandle < 0) {       
		ret_status =  RESOURCE_NO_FOUND_ERROR;
		goto VDO_START_END;
	}
    
	if (src.Format.VideoFormat == MJPEG) {
		pAviEncVidPara->video_format = C_MJPG_FORMAT;
#if MPEG4_ENCODE_ENABLE == 1 
	} else if (src.Format.VideoFormat == MPEG4) {
		pAviEncVidPara->video_format = C_XVID_FORMAT;
#endif
	} else {
		ret_status =  RESOURCE_WRITE_ERROR;
		goto VDO_START_END;

	}
    
	avi_encode_set_curworkmem((void *) pAviEncPacker0); 
	nRet = avi_encode_set_file_handle_and_caculate_free_size(pAviEncPara->AviPackerCur, src.type_ID.FileHandle);
	if (nRet < 0) {
		ret_status = RESOURCE_WRITE_ERROR;
		goto VDO_START_END;
	}
    	
	// start avi packer
	nRet = avi_enc_packer_start(pAviEncPara->AviPackerCur);
	if (nRet < 0) {
		_dmsg((GREEN "[D]: video_encode_start() - start avi packer fail\r\n" NONE));
		ret_status = CODEC_START_STATUS_ERROR_MAX;
		goto VDO_START_END;
	}
   
	// start avi encode
	nRet = avi_enc_start();
	if (nRet < 0) {
		_dmsg((GREEN "[D]: video_encode_start() - start avi encode fail\r\n" NONE));
		ret_status = CODEC_START_STATUS_ERROR_MAX;
		goto VDO_START_END;
	}

VDO_START_END:
	if (ret_status != START_OK) {
		DBG_PRINT("AVI Packer Err:%d\r\n",ret_status);
		video_encode_stop();
	}
	return ret_status;
}

CODEC_START_STATUS video_encode_stop(void)
{
    INT32S nRet, nTemp;

    #if C_AUTO_DEL_FILE==CUSTOM_ON	
      unlink_step_flush();
    #endif
	if(avi_encode_get_status() & C_AVI_ENCODE_PAUSE)
		 nRet = avi_enc_resume();
		 
    //stop avi encode	
    nRet = avi_enc_stop();
    //stop avi packer
    nTemp = avi_enc_packer_stop(pAviEncPara->AviPackerCur);
    
    if(nRet < 0 || nTemp < 0)
    {
    	return CODEC_START_STATUS_ERROR_MAX;
    }
    	
    return START_OK;
}

CODEC_START_STATUS video_encode_Info(VIDEO_INFO *info)
{
	switch(pAviEncAudPara->audio_format)
	{
	case WAVE_FORMAT_PCM:
		info->AudFormat = WAV;
		gp_strcpy((INT8S*)info->AudSubFormat, (INT8S *)"pcm");
		info->AudBitRate = pAviEncAudPara->audio_sample_rate * 16;
		break;
	
	case WAVE_FORMAT_ALAW:	
	case WAVE_FORMAT_MULAW:
		info->AudFormat = WAV;
		gp_strcpy((INT8S*)info->AudSubFormat, (INT8S *)"adpcm");
		info->AudBitRate = pAviEncAudPara->audio_sample_rate * 16 / 4;
		break;
			
	case WAVE_FORMAT_ADPCM:
		info->AudFormat = MICROSOFT_ADPCM;
		gp_strcpy((INT8S*)info->AudSubFormat, (INT8S *)"adpcm");
		info->AudBitRate = pAviEncAudPara->audio_sample_rate * 16 / 4;
		break;
	
	case WAVE_FORMAT_IMA_ADPCM:
		info->AudFormat = IMA_ADPCM;
		gp_strcpy((INT8S*)info->AudSubFormat, (INT8S *)"adpcm");
		info->AudBitRate = pAviEncAudPara->audio_sample_rate * 16 / 4;
		break;
		
	default:
		while(1);
	}
	info->AudSampleRate = pAviEncAudPara->audio_sample_rate;
	info->AudChannel = 1;//mono	
	
	switch(pAviEncVidPara->video_format)
	{
	case C_MJPG_FORMAT:
		info->VidFormat = MJPEG;
		gp_strcpy((INT8S*)info->VidSubFormat, (INT8S *)"jpg");
		break;
	
	case C_XVID_FORMAT:
		info->VidFormat = MPEG4;
		gp_strcpy((INT8S*)info->VidSubFormat, (INT8S *)"mp4");
		break;
		
	default:
		while(1);
	}
	
	info->VidFrameRate = pAviEncVidPara->dwRate;
	info->Width = pAviEncVidPara->encode_width;
	info->Height = pAviEncVidPara->encode_height;
	return	START_OK;
}

VIDEO_CODEC_STATUS video_encode_status(void)
{
	INT32U status;
	
	status = avi_encode_get_status();
	if(status & C_AVI_ENCODE_PAUSE) 
		return VIDEO_CODEC_PROCESS_PAUSE;
	
	if(status & C_AVI_ENCODE_START)
		return VIDEO_CODEC_PROCESSING;
	 
	return VIDEO_CODEC_PROCESS_END;	
}

CODEC_START_STATUS video_encode_auto_switch_csi_frame(void)
{
	avi_encode_switch_csi_frame_buffer();
	return START_OK;
}

CODEC_START_STATUS video_encode_auto_switch_csi_fifo_end(void)
{
	vid_enc_csi_fifo_end();
	return START_OK;
}

CODEC_START_STATUS video_encode_auto_switch_csi_fifo_frame_end(void)
{
	vid_enc_csi_frame_end();
	return START_OK;
}

CODEC_START_STATUS video_encode_set_zoom_scaler(FP32 zoom_ratio)
{
	pAviEncVidPara->scaler_zoom_ratio = zoom_ratio;
	gplib_ppu_text_rotate_zoom_set(&p_register_set, C_PPU_TEXT1, 0, zoom_ratio/2);
	avi_encode_set_jpeg_quality(pAviEncVidPara->quality_value);
	return START_OK;
}

CODEC_START_STATUS video_encode_pause(void)
{
    INT32S nRet;
    
	nRet = avi_enc_pause();
    if(nRet < 0)
    	return CODEC_START_STATUS_ERROR_MAX;
    	
	return START_OK;
}

CODEC_START_STATUS video_encode_resume(void)
{
    INT32S nRet;
    
    nRet = avi_enc_resume();
	if(nRet < 0)
    	return CODEC_START_STATUS_ERROR_MAX;
    	
	return START_OK;
}

CODEC_START_STATUS video_encode_set_jpeg_quality(INT8U quality_value)
{
	if(quality_value == pAviEncVidPara->quality_value)
		return START_OK;
		
	if(avi_encode_set_jpeg_quality(quality_value))
		return START_OK;
	
	return CODEC_START_STATUS_ERROR_MAX;
}

INT32U video_encode_get_jpeg_quality(void)
{
	return pAviEncVidPara->quality_value;
}

CODEC_START_STATUS video_encode_capture_picture(MEDIA_SOURCE src)
{
	INT16S disk_no;
	INT32S size;
	
	if(src.type_ID.FileHandle < 0)
		return CODEC_START_STATUS_ERROR_MAX;
	
	pAviEncPara->AviPackerCur->file_handle = src.type_ID.FileHandle;	
	size = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height;
	disk_no = GetDiskOfFile(src.type_ID.FileHandle);	
	if(vfsFreeSpace(disk_no) < size) 
	{
		close(src.type_ID.FileHandle);
		return CODEC_START_STATUS_ERROR_MAX;
	}
		
	if(avi_enc_save_jpeg() < 0)
	{
		close(src.type_ID.FileHandle);
		return CODEC_START_STATUS_ERROR_MAX;
	}
	
	return START_OK;
}

CODEC_START_STATUS video_encode_fast_switch_stop_and_start(MEDIA_SOURCE src)
{
#if AVI_ENCODE_FAST_SWITCH_EN == 1	
	INT8U temp[5] = {0, 1, 3, 5, 10};	//wwj add
	INT32S nRet;
	AviEncPacker_t *pNewAviEncPacker, *pOldAviEncPacker;
	
	if(pAviEncPara->AviPackerCur == pAviEncPacker0)
    {
    	pOldAviEncPacker = pAviEncPacker0;
    	pNewAviEncPacker = pAviEncPacker1;
    }
    else
    {
    	pOldAviEncPacker = pAviEncPacker1;
    	pNewAviEncPacker = pAviEncPacker0;
    }

	// creak new packer
	if(src.type == SOURCE_TYPE_FS)
    	pAviEncPara->source_type = SOURCE_TYPE_FS;
    else if(src.type == SOURCE_TYPE_USER_DEFINE)
    	pAviEncPara->source_type = SOURCE_TYPE_USER_DEFINE;
    else 
        goto AVI_PACKER_FAIL;
  	
    if(src.type_ID.FileHandle < 0)        
        goto AVI_PACKER_FAIL;
    
    if(src.Format.VideoFormat == MJPEG)
    	pAviEncVidPara->video_format = C_MJPG_FORMAT;
	else
		goto AVI_PACKER_FAIL;
     
    nRet = avi_encode_set_file_handle_and_caculate_free_size(pNewAviEncPacker, src.type_ID.FileHandle);
    if(nRet < 0) goto AVI_PACKER_FAIL;
    
    //start new avi packer 
    nRet = avi_enc_packer_start(pNewAviEncPacker);
    if(nRet < 0) goto AVI_PACKER_FAIL;
    	
    //stop current avi encode
  	nRet = avi_enc_stop();
	if(nRet < 0) goto AVI_PACKER_FAIL;
	
    OSTaskChangePrio(pOldAviEncPacker->task_prio, AVI_PACKER_PRIORITY);
    
	//start new avi encode
	avi_encode_set_curworkmem((void *)pNewAviEncPacker);  
   	nRet = avi_enc_start();
  	if(nRet < 0) goto AVI_PACKER_FAIL;
    
    //start old avi packer
    //DBG_PRINT ("\r\nidea frame:%d\r\n",temp[ap_state_config_record_time_get()]*60*AVI_FRAME_RATE);
//    VdoFramNumsLowBoundReg(ap_state_config_record_time_get()*60*video_encode_frame_rate_get()); 
    VdoFramNumsLowBoundReg(temp[ap_state_config_record_time_get()]*60*video_encode_frame_rate_get());	//wwj modify
    nRet = avi_enc_packer_stop(pOldAviEncPacker);
    
    if(nRet < 0) goto AVI_PACKER_FAIL;		
    return START_OK;
    
AVI_PACKER_FAIL:
	avi_enc_stop();
	avi_enc_packer_stop(pNewAviEncPacker);
	avi_enc_packer_stop(pOldAviEncPacker);
#endif
	return CODEC_START_STATUS_ERROR_MAX;
}

void video_encode_frame_rate_set(INT8U fps)
{
    pAviEncVidPara->dwRate = fps;
}

INT8U video_encode_frame_rate_get(void)
{
    return pAviEncVidPara->dwRate;
}

