#include "ztkconfigs.h"
#include "avi_encoder_state.h"
 
/* for debug */
#define DEBUG_AVI_ENCODER_STATE		1
#if DEBUG_AVI_ENCODER_STATE
    #include "gplib.h"
    #define _dmsg(x)			print_string x
#else
    #define _dmsg(x)
#endif

/* */
#define C_AVI_ENCODE_STATE_STACK_SIZE   512
#define AVI_ENCODE_QUEUE_MAX_LEN    	200  //5->200 avoid frame insert fail

#define C_AVI_DELETE_STATE_STACK_SIZE   1024
#define C_DELETE_QUEUE_MAX    	5

/* OS stack */
INT32U      AVIEncodeStateStack[C_AVI_ENCODE_STATE_STACK_SIZE];

/* global varaible */
OS_EVENT *AVIEncodeApQ;
OS_EVENT *avi_encode_ack_m;
void *AVIEncodeApQ_Stack[AVI_ENCODE_QUEUE_MAX_LEN];
#if C_UVC == CUSTOM_ON
OS_EVENT *usbwebcam_frame_q;
void *usbwebcam_frame_q_stack[AVI_ENCODE_VIDEO_BUFFER_NO];
#endif

/* function point */
INT32S  (*pfn_avi_encode_put_data)(void *workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag);

//================================================================================================================== 
INT32U avi_enc_packer_start(AviEncPacker_t *pAviEncPacker)
{
	INT32S nRet; 
	INT32U bflag;
	
	if(pAviEncPacker == pAviEncPacker0)
	{
		bflag = C_AVI_ENCODE_PACKER0;
		pAviEncPacker->task_prio = AVI_PACKER0_PRIORITY;
	}
	else if(pAviEncPacker == pAviEncPacker1)
	{
		bflag = C_AVI_ENCODE_PACKER1;
		pAviEncPacker->task_prio = AVI_PACKER1_PRIORITY;
	}
	else
	{
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
	if((avi_encode_get_status() & bflag) == 0)
	{
		switch(pAviEncPara->source_type)
		{
		case SOURCE_TYPE_FS:
			avi_encode_set_avi_header(pAviEncPacker);
			nRet = AviPackerV3_Open(pAviEncPacker->avi_workmem,
									pAviEncPacker->file_handle, 
									pAviEncPacker->index_handle,
									pAviEncPacker->p_avi_vid_stream_header,
									pAviEncPacker->bitmap_info_cblen,
									pAviEncPacker->p_avi_bitmap_info,
									pAviEncPacker->p_avi_aud_stream_header,
									pAviEncPacker->wave_info_cblen,
									pAviEncPacker->p_avi_wave_info, 
									pAviEncPacker->task_prio,
									pAviEncPacker->file_write_buffer, 
									pAviEncPacker->file_buffer_size, 
									pAviEncPacker->index_write_buffer, 
									pAviEncPacker->index_buffer_size);
			AviPackerV3_SetErrHandler(pAviEncPacker->avi_workmem, avi_packer_err_handle);
			pfn_avi_encode_put_data = AviPackerV3_PutData;
			break;
		case SOURCE_TYPE_USER_DEFINE:
			pfn_avi_encode_put_data = video_encode_frame_ready;
			break;
		}		
		avi_encode_set_status(bflag);
		_dmsg((PURPLE "a.AviPackerOpen[0x%x] = 0x%x\r\n"  NONE, bflag, nRet));
	}
	else
	{
		RETURN(STATUS_FAIL);
	}
Return:	
	return nRet;
}

INT32U avi_enc_packer_stop(AviEncPacker_t *pAviEncPacker)
{
	INT32S nRet;
	INT32U bflag;
	
	if(pAviEncPacker == pAviEncPacker0)
	{		
		bflag = C_AVI_ENCODE_PACKER0;
	}
	else if(pAviEncPacker == pAviEncPacker1)
	{
		bflag = C_AVI_ENCODE_PACKER1;
	}
	else
	{
		RETURN(STATUS_FAIL);
	}
	
	if(avi_encode_get_status() & bflag)
	{
		switch(pAviEncPara->source_type)
        {
		case SOURCE_TYPE_FS:
			video_encode_end(pAviEncPacker->avi_workmem);
           	nRet = AviPackerV3_Close(pAviEncPacker->avi_workmem); 
           	avi_encode_close_file(pAviEncPacker);
        	break;		
        case SOURCE_TYPE_USER_DEFINE:
        	nRet = STATUS_OK;
        	break;
        } 
        
        if(nRet < 0) RETURN(STATUS_FAIL);
		avi_encode_clear_status(bflag);
		_dmsg((PURPLE "c.AviPackerClose[0x%x] = 0x%x\r\n" NONE, bflag, nRet));
	}
	nRet = STATUS_OK;
Return:	
	return nRet;
}

INT32S vid_enc_preview_start(void)
{
	INT8U  err;
	INT32S nRet, msg;

	nRet = STATUS_OK;
	// start scaler
	if ((avi_encode_get_status() & C_AVI_ENCODE_SCALER) == 0) {
		if (scaler_task_start() < 0)
			return(STATUS_FAIL);
		avi_encode_set_status(C_AVI_ENCODE_SCALER);
		_dmsg((WHITE "a.scaler start\r\n" NONE)); 
	}
	// start video
#if AVI_ENCODE_VIDEO_ENCODE_EN == 1 
	if ((avi_encode_get_status() & C_AVI_ENCODE_VIDEO) == 0) {	
		if (video_encode_task_start() < 0)
			RETURN(STATUS_FAIL);
		avi_encode_set_status(C_AVI_ENCODE_VIDEO);
		_dmsg((WHITE "b.video start\r\n" NONE));
	}
#endif
	// start sensor
	if ((avi_encode_get_status() & C_AVI_ENCODE_SENSOR) == 0) {
// ### BEGIN - by xyz for zt31xx firmware update
//		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_START_SENSOR, avi_encode_ack_m, 5000, msg, err);	
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_START_SENSOR, avi_encode_ack_m, 5000*100, msg, err);	
// ### END   - by xyz
		avi_encode_set_status(C_AVI_ENCODE_SENSOR);
		_dmsg((WHITE "c.sensor start\r\n" NONE));
	}
	// start audio 
#if AVI_ENCODE_PRE_ENCODE_EN == 1
	if (pAviEncAudPara->audio_format && ((avi_encode_get_status() & C_AVI_ENCODE_AUDIO) == 0)) {
		if (avi_audio_record_start() < 0)
			RETURN(STATUS_FAIL);
		avi_encode_set_status(C_AVI_VID_ENC_START); 
		avi_encode_set_status(C_AVI_ENCODE_AUDIO);
		avi_encode_audio_timer_start();
		_dmsg((WHITE "d.audio start\r\n" NONE));
	}
#endif	
Return:	
	return nRet;
}

INT32S vid_enc_preview_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	// stop audio	
	if(avi_encode_get_status()&C_AVI_ENCODE_AUDIO)
	{
		if(avi_audio_record_stop() < 0) RETURN(STATUS_FAIL);
		avi_encode_clear_status(C_AVI_ENCODE_AUDIO);
		avi_encode_audio_timer_stop();
		DEBUG_MSG(DBG_PRINT("a.audio stop\r\n"));
	}
	// stop sensor
	if(avi_encode_get_status()&C_AVI_ENCODE_SENSOR)
	{
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_STOP_SENSOR, avi_encode_ack_m, 5000, msg, err);	
		avi_encode_clear_status(C_AVI_ENCODE_SENSOR);
		DEBUG_MSG(DBG_PRINT("b.sensor stop\r\n"));
	}	
	// stop video
	if(avi_encode_get_status()&C_AVI_ENCODE_VIDEO)
	{	
		if(video_encode_task_stop() < 0) RETURN(STATUS_FAIL);
		avi_encode_clear_status(C_AVI_ENCODE_VIDEO);
		DEBUG_MSG(DBG_PRINT("c.video stop\r\n"));
	}
	// stop scaler
	if(avi_encode_get_status()&C_AVI_ENCODE_SCALER)
	{
		if(scaler_task_stop() < 0) RETURN(STATUS_FAIL); 
		avi_encode_clear_status(C_AVI_ENCODE_SCALER);
		DEBUG_MSG(DBG_PRINT("d.scaler stop\r\n"));  
	}
Return:	
	return nRet;
}

INT32S avi_enc_start(void)
{
	INT8U  err;
	INT32S nRet, msg;

	_dmsg((GREEN "[S]: avi_enc_start()\r\n" NONE, mm_dump()));

	nRet = STATUS_OK;
	// start audio 
	if (pAviEncAudPara->audio_format && ((avi_encode_get_status() & C_AVI_ENCODE_AUDIO) == 0)) {
		if (avi_audio_record_start() < 0) {
			_dmsg((GREEN "[E]: avi_enc_start() - audio start fail\r\n" NONE));
			return (STATUS_FAIL);
		}
		avi_encode_set_status(C_AVI_ENCODE_AUDIO);
		DEBUG_MSG(DBG_PRINT(BLUE "b.audio start\r\n" NONE));
	}
	// restart audio 
#if AVI_ENCODE_PRE_ENCODE_EN == 1
	if (pAviEncAudPara->audio_format && (avi_encode_get_status() & C_AVI_ENCODE_AUDIO)) {
		if (avi_audio_record_restart() < 0) {
			_dmsg((GREEN "[E]: avi_enc_start() - audio restart fail\r\n" NONE));
			return (STATUS_FAIL);
		}
		DEBUG_MSG(DBG_PRINT("b.audio restart\r\n"));
	}
#endif
	// start avi encode
	if ((avi_encode_get_status() & C_AVI_ENCODE_START) == 0) {
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_START_ENCODE, avi_encode_ack_m, 5000, msg, err);	
		avi_encode_set_status(C_AVI_ENCODE_START);
		avi_encode_set_status(C_AVI_VID_ENC_START);
		DEBUG_MSG(DBG_PRINT(BLUE "c.encode start\r\n" NONE)); 
	}
Return:
	_dmsg((GREEN "[E]: avi_enc_start() - pass (%0x)\r\n" NONE, nRet));
	return nRet;
}

INT32S avi_enc_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
#if AVI_ENCODE_PRE_ENCODE_EN == 0	
	// stop audio
	if(avi_encode_get_status()&C_AVI_ENCODE_AUDIO)
	{
		if(avi_audio_record_stop() < 0) RETURN(STATUS_FAIL);
		avi_encode_clear_status(C_AVI_ENCODE_AUDIO);
		DEBUG_MSG(DBG_PRINT(BLUE "a.audio stop\r\n" NONE));
	}
#endif
	// stop avi encode
	if(avi_encode_get_status()&C_AVI_ENCODE_START)
	{
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_STOP_ENCODE, avi_encode_ack_m, 5000, msg, err);	
		avi_encode_clear_status(C_AVI_ENCODE_START);
#if AVI_ENCODE_PRE_ENCODE_EN == 0			
		avi_encode_clear_status(C_AVI_VID_ENC_START); 
#endif		
		DEBUG_MSG(DBG_PRINT(BLUE "b.encode stop\r\n" NONE)); 
	}
Return:	
	return nRet;
}

INT32S avi_enc_pause(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	if(avi_encode_get_status()&C_AVI_ENCODE_START)
    {
    	POST_MESSAGE(AVIEncodeApQ, MSG_AVI_PAUSE_ENCODE, avi_encode_ack_m, 5000, msg, err);	
    	avi_encode_set_status(C_AVI_ENCODE_PAUSE);
		DEBUG_MSG(DBG_PRINT("encode pause\r\n")); 
    }
Return:    
	return nRet;
}

INT32S avi_enc_resume(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	if(avi_encode_get_status()&(C_AVI_ENCODE_START|C_AVI_ENCODE_PAUSE))
    {
    	POST_MESSAGE(AVIEncodeApQ, MSG_AVI_RESUME_ENCODE, avi_encode_ack_m, 5000, msg, err);	
    	avi_encode_clear_status(C_AVI_ENCODE_PAUSE);
		DEBUG_MSG(DBG_PRINT("encode resume\r\n"));  
    }
Return:    
	return nRet;
}

static INT32U  pic_width,pic_height,pic_quality;

INT32S avi_enc_save_jpeg(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	avi_encode_set_status(C_AVI_VID_ENC_START);  

	pic_width = 1280;
	pic_height = 960;
	pic_quality = pAviEncVidPara->quality_value;

	POST_MESSAGE(AVIEncodeApQ, MSG_AVI_CAPTURE_PICTURE, avi_encode_ack_m, 5000, msg, err);		
Return: 
#if AVI_ENCODE_PRE_ENCODE_EN == 0   
//	avi_encode_clear_status(C_AVI_VID_ENC_START);  
#endif	
	return nRet;
}

#if C_UVC == CUSTOM_ON
INT32S usb_webcam_start(void)
{

	INT8U  err;
	INT32S nRet, msg;
	INT32S nTemp;
	
	if(avi_encode_get_status()&C_AVI_ENCODE_START)
	{
		if(avi_encode_get_status() & C_AVI_ENCODE_PAUSE)
		 	nRet = avi_enc_resume();
		 
    	//stop avi encode	
    	nRet = avi_enc_stop();
   		 //stop avi packer
    	nTemp = avi_enc_packer_stop(pAviEncPara->AviPackerCur);
    
    	if(nRet < 0 || nTemp < 0)
    		return CODEC_START_STATUS_ERROR_MAX;
	}
	nRet = STATUS_OK;
	POST_MESSAGE(AVIEncodeApQ, MSG_AVI_START_USB_WEBCAM, avi_encode_ack_m, 5000, msg, err);
	avi_encode_set_status(C_AVI_ENCODE_USB_WEBCAM);
Return:    
	return nRet;
}
INT32S usb_webcam_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	avi_encode_clear_status(C_AVI_ENCODE_USB_WEBCAM);
	nRet = STATUS_OK;
	POST_MESSAGE(AVIEncodeApQ, MSG_AVI_STOP_USB_WEBCAM, avi_encode_ack_m, 5000, msg, err);
Return:    
	return nRet;
}
#endif
//======================================================================================================
INT32S avi_encode_state_task_create(INT8U pori)
{
	INT8U err;
	INT32S nRet;
	
	AVIEncodeApQ = OSQCreate(AVIEncodeApQ_Stack, AVI_ENCODE_QUEUE_MAX_LEN);
    if(!AVIEncodeApQ)  RETURN(STATUS_FAIL);
#if C_UVC == CUSTOM_ON
    usbwebcam_frame_q = OSQCreate(usbwebcam_frame_q_stack, AVI_ENCODE_VIDEO_BUFFER_NO);
    if(!usbwebcam_frame_q)  RETURN(STATUS_FAIL);
#endif
    avi_encode_ack_m = OSMboxCreate(NULL);
	if(!avi_encode_ack_m) RETURN(STATUS_FAIL);
	
	err = OSTaskCreate(avi_encode_state_task_entry, (void*) NULL, &AVIEncodeStateStack[C_AVI_ENCODE_STATE_STACK_SIZE - 1], pori);
	if(err != OS_NO_ERR) RETURN(STATUS_FAIL);
	
    nRet = STATUS_OK;
Return:
    return nRet;
}

INT32S avi_encode_state_task_del(void)
{
    INT8U   err;
    INT32S  nRet, msg;
   
    nRet = STATUS_OK; 
 	POST_MESSAGE(AVIEncodeApQ, MSG_AVI_ENCODE_STATE_EXIT, avi_encode_ack_m, 5000, msg, err);	
 Return:	
   	OSQFlush(AVIEncodeApQ);
   	OSQDel(AVIEncodeApQ, OS_DEL_ALWAYS, &err);
	OSMboxDel(avi_encode_ack_m, OS_DEL_ALWAYS, &err);
    return nRet;
}

INT32S avi_enc_storage_full(void)
{
	DEBUG_MSG(DBG_PRINT("avi encode storage full!!!\r\n"));
	avi_encode_video_timer_stop();
	if(OSQPost(AVIEncodeApQ, (void*)MSG_AVI_STORAGE_FULL) != OS_NO_ERR)
		return STATUS_FAIL;

	return STATUS_OK;
} 

INT32S avi_packer_err_handle(INT32S ErrCode)
{
	DEBUG_MSG(DBG_PRINT("AviPacker-ErrID = 0x%x!!!\r\n", ErrCode));	
	avi_encode_video_timer_stop();
	if (ErrCode == AVIPACKER_RESULT_FILE_WRITE_ERR || ErrCode == AVIPACKER_RESULT_IDX_FILE_WRITE_ERR) {
		msgQSend(ApQ, MSG_APQ_AVI_PACKER_ERROR, NULL, NULL, MSG_PRI_NORMAL);
	}
	//if(OSQPost(AVIEncodeApQ, (void*)MSG_AVI_STORAGE_ERR) != OS_NO_ERR)
		//return 0;
			
	return 1; 	//return 1 to close task
}

INT8U take_pic;
void avi_encode_state_task_entry(void *para)
{
	INT8U   err, success_flag, key_flag;
	//INT8U	*src;
    INT32S  nRet, N;
    INT32U  msg_id, video_frame, video_stream, encode_size, decode_stream;
    INT64S  dwtemp;	
	OS_CPU_SR cpu_sr;
	
    while(1)
    {      
        msg_id = (INT32U) OSQPend(AVIEncodeApQ, 0, &err);
        if((!msg_id) || (err != OS_NO_ERR))
        	continue;
        	
        switch(msg_id)
        {
        case MSG_AVI_START_SENSOR:	//sensor
          	OSQFlush(cmos_frame_q);
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
        	video_frame = avi_encode_get_csi_frame();
		video_stream = 0;
       		for (nRet = 1; nRet<AVI_ENCODE_CSI_BUFFER_NO; nRet++)
			OSQPost(cmos_frame_q, (void *) avi_encode_get_csi_frame());
#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
		video_frame = avi_encode_get_csi_frame();
		video_stream = avi_encode_get_csi_frame();	
		for (nRet = 0; nRet<pAviEncPara->vid_post_cnt; nRet++)
			OSQPost(cmos_frame_q, (void *) avi_encode_get_csi_frame());		
#endif
		nRet = video_encode_sensor_start(video_frame, video_stream);
		if (nRet >= 0)
			OSMboxPost(avi_encode_ack_m, (void*)C_ACK_SUCCESS);
        	else
			OSMboxPost(avi_encode_ack_m, (void*)C_ACK_FAIL);
		break;
        
        case MSG_AVI_STOP_SENSOR: 
            nRet = video_encode_sensor_stop();
            OSQFlush(cmos_frame_q);
            if(nRet >= 0)
            	OSMboxPost(avi_encode_ack_m, (void*)C_ACK_SUCCESS);
        	else
            	OSMboxPost(avi_encode_ack_m, (void*)C_ACK_FAIL);
            break;   
#if C_UVC == CUSTOM_ON            
        case MSG_AVI_START_USB_WEBCAM:	//sensor
          	OSQFlush(usbwebcam_frame_q);
			for(nRet = 0; nRet<AVI_ENCODE_VIDEO_BUFFER_NO; nRet++)
				OSQPost(usbwebcam_frame_q, (void *) avi_encode_get_video_frame());	
				
	        if(nRet >= 0)
            	OSMboxPost(avi_encode_ack_m, (void*)C_ACK_SUCCESS);
        	else
            	OSMboxPost(avi_encode_ack_m, (void*)C_ACK_FAIL);
            break;
        
        case MSG_AVI_STOP_USB_WEBCAM: 
        
            OSQFlush(usbwebcam_frame_q);
            if(nRet >= 0)
            	OSMboxPost(avi_encode_ack_m, (void*)C_ACK_SUCCESS);
        	else
            	OSMboxPost(avi_encode_ack_m, (void*)C_ACK_FAIL);
            break;
#endif             
        case MSG_AVI_START_ENCODE: //avi encode start
			N = 0;
			nRet = pAviEncVidPara->dwRate/pAviEncVidPara->dwScale;
			while(nRet >>= 1) N++; 
			pAviEncPara->ta = 0;
			pAviEncPara->tv = 0;
			pAviEncPara->Tv = 0;
			pAviEncPara->pend_cnt = 0;
			pAviEncPara->post_cnt = 0;
			pAviEncPara->ready_frame = 0;
			pAviEncPara->ready_size = 0;
			if(pAviEncPara->AviPackerCur->p_avi_wave_info)
			{
				pAviEncPara->freq_div = pAviEncAudPara->audio_sample_rate/AVI_ENCODE_TIME_BASE;
				pAviEncPara->tick = (INT64S)pAviEncVidPara->dwRate * pAviEncPara->freq_div;
				pAviEncPara->delta_Tv = pAviEncVidPara->dwScale * pAviEncAudPara->audio_sample_rate;
			}
			else
			{
				pAviEncPara->freq_div = 1;
				pAviEncPara->tick = (INT64S)pAviEncVidPara->dwRate * pAviEncPara->freq_div;
				pAviEncPara->delta_Tv = pAviEncVidPara->dwScale * AVI_ENCODE_TIME_BASE;
			}
		case MSG_AVI_RESUME_ENCODE:
		#if AVI_ENCODE_PRE_ENCODE_EN == 0
			if(pAviEncPara->AviPackerCur->p_avi_wave_info) 
				avi_encode_audio_timer_start();
        #endif	
        	avi_encode_video_timer_start();
            OSMboxPost(avi_encode_ack_m, (void*)C_ACK_SUCCESS);
            break;
            
        case MSG_AVI_STOP_ENCODE:  
        case MSG_AVI_PAUSE_ENCODE:
        #if AVI_ENCODE_PRE_ENCODE_EN == 0
        	if(pAviEncPara->AviPackerCur->p_avi_wave_info) 
        		avi_encode_audio_timer_stop();
        #endif	
        	avi_encode_video_timer_stop();
            OSMboxPost(avi_encode_ack_m, (void*)C_ACK_SUCCESS);
            break;   
        
        case MSG_AVI_CAPTURE_PICTURE:
        	pAviEncPara->ready_frame = 0;
        	pAviEncPara->ready_size = 0;
        	take_pic = 1;
        	for(nRet=0; nRet<150 ; nRet++)
        	{
        		if(pAviEncPara->ready_frame) 
        			break;
        		OSTimeDly(1);
        	}
        	if(nRet == 150) goto AVI_CAPTURE_PICTURE_FAIL;

        	//vid_enc_buffer_lock(pAviEncPara->ready_frame);
			/*
        	video_frame = pAviEncPara->ready_frame;
			encode_size = pAviEncPara->ready_size;
			key_flag = pAviEncPara->key_flag;
			*/

            video_frame = (INT32U) pAviEncPara->ready_frame;
			encode_size = pAviEncPara->ready_size;
			decode_stream = ((INT32U)pAviEncPacker0->file_write_buffer+0xF) & (~0xF);
		
			nRet = avi_mjpeg_decode_without_scaler(video_frame, encode_size, decode_stream);
			if (nRet != STATUS_OK) {
			    goto AVI_CAPTURE_PICTURE_FAIL;
			} 

			jpeg_picture_q50_header[0x7] = (pic_height >> 8) & 0xFF;
			jpeg_picture_q50_header[0x8] = pic_height & 0xFF;
			jpeg_picture_q50_header[0x9] = (pic_width >> 8) & 0xFF;
			jpeg_picture_q50_header[0xA] = pic_width & 0xFF;
			
			jpeg_picture_q50_header[0xB0] = ((pic_width>>1) >> 8) & 0xFF;
			jpeg_picture_q50_header[0xB1] = (pic_width>>1) & 0xFF;

			/* copy Quantization Table from sensor jpeg data */
			gp_memcpy((INT8S*)&jpeg_picture_q50_header[0x27],(INT8S*)(pAviEncPara->ready_frame+0x14),64);
			gp_memcpy((INT8S*)&jpeg_picture_q50_header[0x6C],(INT8S*)(pAviEncPara->ready_frame+0x59),64);

			write(pAviEncPara->AviPackerCur->file_handle, (INT32U) jpeg_picture_q50_header, 624);

			scale_up_and_encode(decode_stream, decode_stream + 640*480*2, pic_width, pic_height, ((INT32U)pAviEncPacker1->file_write_buffer+0xF)&(~0xF), pic_quality);
			close(pAviEncPara->AviPackerCur->file_handle);

			/*
			nRet = save_jpeg_file(pAviEncPara->AviPackerCur->file_handle, 
								video_frame, 
								encode_size);
			*/
			vid_enc_buffer_unlock();
			if(nRet < 0) goto AVI_CAPTURE_PICTURE_FAIL;
            OSMboxPost(avi_encode_ack_m, (void*)C_ACK_SUCCESS);
       		break;
AVI_CAPTURE_PICTURE_FAIL: 
        	OSMboxPost(avi_encode_ack_m, (void*)C_ACK_FAIL);
        	break;

       	case MSG_AVI_STORAGE_FULL:
       	case MSG_AVI_STORAGE_ERR:
       	#if AVI_ENCODE_PRE_ENCODE_EN == 0
       		if(pAviEncPara->AviPackerCur->p_avi_wave_info) 
       			avi_audio_record_stop();
	       	video_encode_task_stop();    
	       	
	    	if(pAviEncPara->source_type == SOURCE_TYPE_FS)
        	{
        		AviPackerV3_Close(pAviEncPara->AviPackerCur->avi_workmem);     
        		avi_encode_close_file(pAviEncPara->AviPackerCur);
        	}
        	
        	msg_id = C_AVI_VID_ENC_START|C_AVI_ENCODE_AUDIO|C_AVI_ENCODE_VIDEO|C_AVI_ENCODE_START|C_AVI_ENCODE_PAUSE;
       		if(pAviEncPara->AviPackerCur == pAviEncPacker0)
       			msg_id |= C_AVI_ENCODE_PACKER0; 
       		else
       			msg_id |= C_AVI_ENCODE_PACKER1; 
       		
        	avi_encode_clear_status(msg_id);
	    #else
	    	msg_id = C_AVI_ENCODE_START|C_AVI_ENCODE_PAUSE;
       		if(pAviEncPara->AviPackerCur == pAviEncPacker0)
       			msg_id |= C_AVI_ENCODE_PACKER0; 
       		else
       			msg_id |= C_AVI_ENCODE_PACKER1; 
       		
       		if(pAviEncPara->source_type == SOURCE_TYPE_FS)
        	{
        		AviPackerV3_Close(pAviEncPara->AviPackerCur->avi_workmem);     
        		avi_encode_close_file(pAviEncPara->AviPackerCur);
        	}
       			
	    	avi_encode_clear_status(msg_id);
		#endif
            OSQFlush(AVIEncodeApQ);
            OSQFlush(avi_encode_ack_m);
            break;
            
       	case MSG_AVI_ENCODE_STATE_EXIT:
       		OSMboxPost(avi_encode_ack_m, (void*)C_ACK_SUCCESS); 
   			OSTaskDel(OS_PRIO_SELF);
       		break;
       	
       	case MSG_AVI_ONE_FRAME_ENCODE:
        	success_flag = 0;
        	OS_ENTER_CRITICAL();
			dwtemp = (INT64S)(pAviEncPara->tv - pAviEncPara->Tv);
			OS_EXIT_CRITICAL();
			if(dwtemp > (pAviEncPara->delta_Tv << N))
				goto EncodeNullFrame;
				
			if(pAviEncPara->ready_frame == 0) 
				goto EndofEncodeOneFrame;
			
			video_stream = pAviEncPara->ready_size + 8 + 2*16;
/*			if(!avi_encode_disk_size_is_enough(video_stream))
			{
				avi_enc_storage_full();
				goto EndofEncodeOneFrame;
			}*/
			
			
			vid_enc_buffer_lock(pAviEncPara->ready_frame);
			video_frame = pAviEncPara->ready_frame;
			encode_size = pAviEncPara->ready_size;
			key_flag = pAviEncPara->key_flag;
			
			
			nRet = pfn_avi_encode_put_data(	pAviEncPara->AviPackerCur->avi_workmem,
											*(long*)"00dc", 
											encode_size, 
											(void *)video_frame, 
											1, 
											key_flag);
			pAviEncPara->ready_frame = 0;
			pAviEncPara->ready_size = 0;
			pAviEncPara->key_flag = 0;
			vid_enc_buffer_unlock();
			if(nRet >= 0)
			{	
				DEBUG_MSG(DBG_PRINT("."));
				success_flag = 1;
				goto EndofEncodeOneFrame;
			}
			else
			{ 	
				avi_encode_disk_size_is_enough(-video_stream);
				DEBUG_MSG(DBG_PRINT("VidPutData = %x, size = %d !!!\r\n", nRet-0x80000000, encode_size));
			}
EncodeNullFrame:
			avi_encode_disk_size_is_enough(8+2*16);
			nRet = pfn_avi_encode_put_data(	pAviEncPara->AviPackerCur->avi_workmem,
											*(long*)"00dc", 
											0, 
											(void *)NULL, 
											1, 
											0x00);
			if(nRet >= 0)
			{
				DEBUG_MSG(DBG_PRINT("N"));
				success_flag = 1;
			}
			else
			{
				avi_encode_disk_size_is_enough(-(8+2*16));
				DEBUG_MSG(DBG_PRINT("VidPutDataNULL = %x!!!\r\n", nRet-0x80000000));
			}
			
EndofEncodeOneFrame:
			if(success_flag)
			{
				OS_ENTER_CRITICAL();
				pAviEncPara->Tv += pAviEncPara->delta_Tv;
				OS_EXIT_CRITICAL();
			}
			pAviEncPara->pend_cnt++;
        	break;
        } 
    }
}