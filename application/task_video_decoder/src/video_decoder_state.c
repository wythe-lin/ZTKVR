#include "task_video_decoder.h"

#define C_VID_DEC_STATE_STACK_SIZE		512
#define C_VID_DEC_QUEUE_MAX_LEN    		5

/* OS stack */
INT32U	vid_dec_state_stack[C_VID_DEC_STATE_STACK_SIZE];

/* global varaible */
OS_EVENT *vid_dec_state_q;
OS_EVENT *vid_dec_state_ack_m;
void *vid_dec_state_q_buffer[C_VID_DEC_QUEUE_MAX_LEN];
video_decode_para_struct vid_dec_para;
video_decode_para_struct *p_vid_dec_para;

const GP_BITMAPINFOHEADER *p_bitmap_info;
const GP_WAVEFORMATEX *p_wave_info;
long  g_bitmap_info_len, g_wave_info_len;

INT32S vid_dec_entry(void)
{
	INT32S nRet;

	nRet = video_decode_state_task_create(TSK_PRI_VID_VID_STATE);
	if(nRet < 0) RETURN(STATUS_FAIL);
	nRet = video_decode_task_create(TSK_PRI_VID_VID_DEC);
	if(nRet < 0) RETURN(STATUS_FAIL);
	nRet = audio_decode_task_create(TSK_PRI_VID_AUD_DEC);
	if(nRet < 0) RETURN(STATUS_FAIL);

	nRet = STATUS_OK;
Return:	
	return nRet;
}

INT32S vid_dec_exit(void)
{
	INT32S nRet;

	nRet = video_decode_state_task_del(TSK_PRI_VID_VID_STATE);
	if(nRet < 0) RETURN(STATUS_FAIL);
	nRet = video_decode_task_del(TSK_PRI_VID_VID_DEC);
	if(nRet < 0) RETURN(STATUS_FAIL);
	nRet = audio_decode_task_del(TSK_PRI_VID_AUD_DEC);
	if(nRet < 0) RETURN(STATUS_FAIL);
	
	nRet = STATUS_OK;
Return:	
	return nRet;
}

INT32S vid_dec_parser_start(INT16S fd, INT32S FileTpye, INT64U FileSize)
{
	INT8U  err;
	INT32S nRet, msg;

	if(fd < 0) RETURN(STATUS_FAIL);
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);	
		
	nRet = STATUS_OK;	
	p_vid_dec_para->file_handle = fd;
	p_vid_dec_para->FileType = FileTpye;
	p_vid_dec_para->FileSize = FileSize;
	if(!(vid_dec_get_status() & C_VIDEO_DECODE_PARSER))
    {	
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_START, vid_dec_state_ack_m, 10000, msg, err); //wait 10 sec
		vid_dec_clear_status(C_VIDEO_DECODE_ALL);
		vid_dec_set_status(C_VIDEO_DECODE_PARSER);	
	}	
Return:	
	return nRet;
} 

INT32S vid_dec_parser_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);	
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER_NTH)
		RETURN(STATUS_FAIL);
	
	nRet = STATUS_OK;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
    {
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_STOP, vid_dec_state_ack_m, 5000, msg, err);
		vid_dec_clear_status(C_VIDEO_DECODE_PARSER);	
	}
Return:	
	return nRet;
}

static void vid_dec_parser_end(void)
{
	MultiMediaParser_Delete((void*)p_vid_dec_para->media_handle);
    p_vid_dec_para->media_handle = NULL;
    close(p_vid_dec_para->file_handle);
	p_vid_dec_para->file_handle = -1;
	p_bitmap_info = NULL;
	p_wave_info = NULL;
} 

INT32S vid_dec_start(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER_NTH)
		RETURN(STATUS_FAIL);
		
	nRet = STATUS_OK;
	if(!(vid_dec_get_status() & C_VIDEO_DECODE_PLAY)) 
    {	
    	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_START, vid_dec_state_ack_m, 5000, msg, err);
		vid_dec_set_status(C_VIDEO_DECODE_PLAY);	
	}
Return:	
	return nRet;
}

INT32S vid_dec_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);
		
	nRet = STATUS_OK;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY) 
	{
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_STOP, vid_dec_state_ack_m, 5000, msg, err);
		vid_dec_clear_status(C_VIDEO_DECODE_PLAY);	
	}
Return:
	return nRet;
}

INT32S vid_dec_pause(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);
	
	nRet = STATUS_OK;
	if(!(vid_dec_get_status() & C_VIDEO_DECODE_PAUSE))
    {
    	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PAUSE, vid_dec_state_ack_m, 5000, msg, err);
		vid_dec_set_status(C_VIDEO_DECODE_PAUSE);		
	}
Return:	
	return nRet;
}

INT32S vid_dec_resume(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);
	
	nRet = STATUS_OK;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PAUSE)
    {
    	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_RESUME, vid_dec_state_ack_m, 5000, msg, err);
    	vid_dec_clear_status(C_VIDEO_DECODE_PAUSE);			
	}
Return:	
	return nRet;
}

INT32S vid_dec_end_callback(void)
{
	if(vid_dec_get_video_flag() && (vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) == 0)
		vid_dec_stop_timer();
	
	if(vid_dec_get_audio_flag() && (vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) == 0)
		vid_dec_set_audio_flag(FALSE);
	
	if((vid_dec_get_status() & (C_AUDIO_DECODE_PLAYING|C_VIDEO_DECODE_PLAYING)) == 0)
	{	
		OSQPost(vid_dec_state_q, (void*)MSG_VIDEO_DECODE_FORCE_STOP);
		vid_dec_clear_status(C_VIDEO_DECODE_ALL);
	#if _PROJ_TYPE == _PROJ_TURNKEY	
		msgQSend(ApQ, MSG_APQ_MJPEG_DECODE_END, NULL, NULL, MSG_PRI_NORMAL);
	#else	
		video_decode_end();	
	#endif
	}

	return STATUS_OK;
}

INT32S vid_dec_get_total_samples(void)
{
	return MultiMediaParser_GetVidTotalSamples(p_vid_dec_para->media_handle);
}

INT32S vid_dec_get_total_time(void)
{
	if (p_vid_dec_para->media_handle == 0) {
	    return -1;
	} else { 
	    return MultiMediaParser_GetVidTrackDur(p_vid_dec_para->media_handle) >> 8;
	}	
}

INT32S vid_dec_get_current_time(void)
{
	if (p_vid_dec_para->media_handle == 0) {
	    return -1;
	} else { 
	    return MultiMediaParser_GetVidCurTime(p_vid_dec_para->media_handle) >> 8;
	}
}

INT32S vid_dec_set_play_time(INT32U second)
{
	void   *hMedia;
	INT8U  err;
	INT32S nRet, msg;	
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);
	
	if(second >= vid_dec_get_total_time())
		RETURN(STATUS_FAIL);
		
	nRet = STATUS_OK;
	hMedia = p_vid_dec_para->media_handle;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY)
	{
		// stop play
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_STOP, vid_dec_state_ack_m, 5000, msg, err);
		vid_dec_clear_status(C_VIDEO_DECODE_PLAY);
		
		// seek
		nRet = MultiMediaParser_SeekTo(hMedia, second<<8);
		if(nRet < 0)	
			RETURN(STATUS_FAIL);
			
	    if(MultiMediaParser_IsEOA(hMedia))	
			vid_dec_set_audio_flag(FALSE);
		
		if(MultiMediaParser_IsEOV(hMedia))	
			vid_dec_set_video_flag(FALSE);
		
		// start play
		if(vid_dec_get_audio_flag() || vid_dec_get_video_flag())
		{	
			SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_PLAY_TIME, vid_dec_state_ack_m, 5000, msg, err);
			vid_dec_set_status(C_VIDEO_DECODE_PLAY);
		}
		else if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
		{
			SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_STOP, vid_dec_state_ack_m, 5000, msg, err);
			vid_dec_clear_status(C_VIDEO_DECODE_PARSER);		
		}
	}
	else
	{
		nRet = MultiMediaParser_SeekTo(hMedia, second<<8);
	}
		
Return:	
	if(nRet < 0)
		return nRet;
		
	return nRet>>8;
}

// play speed = speed / 0x10000, normal speed = 0x10000 
INT32S vid_dec_set_play_speed(INT32U speed)
{	
	INT32S nRet; 
	OS_CPU_SR cpu_sr;

	if(speed == 0) RETURN(STATUS_FAIL);
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);
		
	p_vid_dec_para->play_speed = speed;	
	if(p_vid_dec_para->play_speed == 0x10000 && p_vid_dec_para->reverse_play_flag == 0)
	{
		if(p_vid_dec_para->ff_audio_flag)
		{
			p_vid_dec_para->ff_audio_flag = FALSE;
			vid_dec_set_audio_flag(TRUE);
			MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 1);
   			if(audio_decode_task_start() >= 0)	
   				vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
		}
	}
	else
	{
		if(vid_dec_get_audio_flag())
    	{
   			p_vid_dec_para->ff_audio_flag = TRUE;
   			vid_dec_set_audio_flag(FALSE);
   			if(audio_decode_task_stop() >= 0)	
   				vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);

			MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 0);   				
		}
	}
	
	OS_ENTER_CRITICAL();
	MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, 0);
	p_vid_dec_para->tick_time2 = (p_vid_dec_para->tick_time * speed) >> 16;
	p_vid_dec_para->fail_cnt = 0;
	p_vid_dec_para->ta = 0;
	p_vid_dec_para->tv = 0;
	p_vid_dec_para->Tv = 0;
	OS_EXIT_CRITICAL(); 
	
	nRet = STATUS_OK;
Return:	
	return nRet;
}

INT32S vid_dec_set_reverse_play(INT32U reverse_play_flag)
{
	INT32S nRet;
	OS_CPU_SR cpu_sr;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);
	
	if(vid_dec_get_video_flag() == 0)
		RETURN(STATUS_FAIL);
	
	if(p_vid_dec_para->reverse_play_flag != reverse_play_flag)
	{
		vid_dec_stop_timer();
		if(video_decode_task_stop() < 0)
			RETURN(STATUS_FAIL);
		
		p_vid_dec_para->reverse_play_flag = reverse_play_flag;
		nRet = MultiMediaParser_SetReversePlay(p_vid_dec_para->media_handle, reverse_play_flag);
		if(nRet<0) RETURN(STATUS_FAIL);
		if(p_vid_dec_para->play_speed == 0x10000 && p_vid_dec_para->reverse_play_flag == 0)
		{
			OS_ENTER_CRITICAL();
			p_vid_dec_para->ta = 0;
			p_vid_dec_para->tv = 0;
			p_vid_dec_para->Tv = 0;
			OS_EXIT_CRITICAL();
			if(p_vid_dec_para->ff_audio_flag)
			{
				p_vid_dec_para->ff_audio_flag = FALSE;
				vid_dec_set_audio_flag(TRUE);
				MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 1);
				if(video_decode_task_restart() < 0)
					RETURN(STATUS_FAIL);	
				
				vid_dec_start_timer();
				if(audio_decode_task_start() >= 0)
					vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
			}
			else
			{
				if(video_decode_task_restart() < 0)
					RETURN(STATUS_FAIL);	
				
				vid_dec_start_timer();
			} 
		}
		else
		{
			if(vid_dec_get_audio_flag())
	    	{
	   			p_vid_dec_para->ff_audio_flag = TRUE;
	   			vid_dec_set_audio_flag(FALSE);
	   			if(audio_decode_task_stop() >= 0)
	   				vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
				
				MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 0);      				
			}
			
			OS_ENTER_CRITICAL();
			p_vid_dec_para->ta = 0;
			p_vid_dec_para->tv = 0;
			p_vid_dec_para->Tv = 0;
			OS_EXIT_CRITICAL(); 
			if(video_decode_task_restart() < 0)
				RETURN(STATUS_FAIL);	
			
			vid_dec_start_timer();
		}
	}
	
	nRet = STATUS_OK;
Return:
	return nRet;
}

INT32S vid_dec_nth_frame(INT32U nth)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) 
		RETURN(STATUS_FAIL);
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY)
		RETURN(STATUS_FAIL);
	
	if(vid_dec_get_video_flag() == 0)
		RETURN(STATUS_FAIL);
	
	nRet = STATUS_OK;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
	{
		vid_dec_set_status(C_VIDEO_DECODE_PARSER_NTH);
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_NTH_FRAME, vid_dec_state_ack_m, 5000, msg, err);
	}
Return:
	vid_dec_clear_status(C_VIDEO_DECODE_PARSER_NTH);
	return nRet;	
}

INT32S video_decode_state_task_create(INT8U pori)
{
	INT8U  err;
	INT32S nRet;
	
	p_vid_dec_para = &vid_dec_para;
	gp_memset((INT8S*)p_vid_dec_para, 0, sizeof(video_decode_para_struct));
    vid_dec_state_q = OSQCreate(vid_dec_state_q_buffer, C_VID_DEC_QUEUE_MAX_LEN);
    if(!vid_dec_state_q) RETURN(STATUS_FAIL);
    	
    vid_dec_state_ack_m = OSMboxCreate(NULL);
	if(!vid_dec_state_ack_m) RETURN(STATUS_FAIL);
	
	err = OSTaskCreate(video_decode_state_task_entry, (void*) NULL, &vid_dec_state_stack[C_VID_DEC_STATE_STACK_SIZE - 1], pori);
	if(err != OS_NO_ERR) RETURN(STATUS_FAIL);
	
	nRet = STATUS_OK;
Return:		
	return nRet;	
}

INT32S video_decode_state_task_del(INT8U pori)
{ 
    INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK; 
    if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY) 
    {	
    	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_STOP, vid_dec_state_ack_m, 5000, msg, err);
   		vid_dec_clear_status(C_VIDEO_DECODE_PLAY);	
    }
    
    if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER)
    {
    	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_STOP, vid_dec_state_ack_m, 5000, msg, err);
    	vid_dec_clear_status(C_VIDEO_DECODE_PARSER);	
    }
    
    SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_EXIT, vid_dec_state_ack_m, 5000, msg, err);
Return:    		
    OSQFlush(vid_dec_state_q);
   	OSQDel(vid_dec_state_q, OS_DEL_ALWAYS, &err);
	OSMboxDel(vid_dec_state_ack_m, 0, &err);
	return nRet;
}

//////////////////////////////////////////////////////////////////////////////////////
void video_decode_error_callback(void)
{
	DEBUG_MSG(DBG_PRINT("video_decode_error_callback\r\n"));
}

int video_parser_error_handle(INT32S error_id)
{
	DEBUG_MSG(DBG_PRINT("ParserErrID =0x%x\r\n", error_id));
	vid_dec_set_status(C_VIDEO_DECODE_ERR);
	// stop video 
	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING)
    {
        vid_dec_stop_timer();
        video_decode_task_stop();
    	vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
    }
    
    // wait audio stop
    if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING)
    {
    	while(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING)
			OSTimeDly(1);
  	}
  	
	vid_dec_clear_status(C_VIDEO_DECODE_ALL);
	// stop parser and free memory
	OSQPost(vid_dec_state_q, (void*)MSG_VIDEO_DECODE_ERROR);
	#if _PROJ_TYPE == _PROJ_TURNKEY	
		msgQSend(ApQ, MSG_APQ_MJPEG_DECODE_END, NULL, NULL, MSG_PRI_NORMAL);
	#else	
		video_decode_end();	
	#endif	
	return 1;
}

void video_decode_state_task_entry(void *para)
{
	void    *hMedia;
	INT8U   err;
    INT32S  i, error_id;
    INT32U  msg_id;
 	
    while(1)
    {      
        msg_id = (INT32U) OSQPend(vid_dec_state_q, 0, &err);
        if((!msg_id) || (err != OS_NO_ERR))
        	continue;
        
        switch(msg_id)
        {
        case MSG_VIDEO_DECODE_PARSER_HEADER_START:
        	p_vid_dec_para->pend_cnt = 0;
        	p_vid_dec_para->post_cnt = 0;
        	p_vid_dec_para->video_flag = 0;
        	p_vid_dec_para->user_define_flag = 0;
        	p_vid_dec_para->video_format = 0;
        	p_vid_dec_para->deblock_flag = 0;
        	p_vid_dec_para->play_speed = 0x10000;
        	p_vid_dec_para->reverse_play_flag = 0;
        	p_vid_dec_para->ff_audio_flag = 0;	
        	p_vid_dec_para->audio_flag = 0;
        	p_vid_dec_para->scaler_flag = 0;
        	p_vid_dec_para->image_output_width = 0;
        	p_vid_dec_para->image_output_height = 0;
        	p_vid_dec_para->buffer_output_width = 0;
        	p_vid_dec_para->buffer_output_height = 0;
        	p_vid_dec_para->mpeg4_decode_out_format = C_MP4_OUTPUT_Y1UY0V;
			p_vid_dec_para->video_decode_in_format =  C_SCALER_CTRL_IN_YUYV;
			p_vid_dec_para->video_decode_out_format = C_SCALER_CTRL_OUT_YUYV;
           	
           	for(i=0; i<AUDIO_FRAME_NO; i++)
				p_vid_dec_para->audio_decode_addr[i] = 0;
	
           	for(i=0; i<VIDEO_FRAME_NO; i++)
				p_vid_dec_para->video_decode_addr[i] = 0;
			
			for(i=0; i<SCALER_FRAME_NO; i++)
				p_vid_dec_para->scaler_output_addr[i] = 0;
			
			for(i=0; i<DEBLOCK_FRAME_NO; i++)
				p_vid_dec_para->deblock_addr[i] = 0;
			
           	hMedia = MultiMediaParser_Create(
           		p_vid_dec_para->file_handle,
           		p_vid_dec_para->FileSize,
           		PARSER_AUD_BITSTREAM_SIZE,
           		PARSER_VID_BITSTREAM_SIZE,
           		TSK_PRI_VID_PARSER,
           		AviParser_GetFcnTab(),
           		video_parser_error_handle,
           		&error_id);
           
           	if((error_id & 0x80000000) || !hMedia)
           	{
           		close(p_vid_dec_para->file_handle);
	           	p_vid_dec_para->file_handle = -1;
	           	p_wave_info = NULL;
           		p_bitmap_info = NULL;
           		DEBUG_MSG(DBG_PRINT("ParserErrID = 0x%x\r\n", error_id));
           		OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL);
           	}
           	else
           	{	
           		p_vid_dec_para->media_handle = hMedia;
           		p_vid_dec_para->VidTickRate = MultiMediaParser_GetVidTickRate(p_vid_dec_para->media_handle);
           		p_bitmap_info = MultiMediaParser_GetVidInfo(hMedia, &g_bitmap_info_len);
           		p_wave_info = MultiMediaParser_GetAudInfo(hMedia, &g_wave_info_len);
				
           		switch(p_wave_info->wFormatTag)
           		{
           		case WAVE_FORMAT_PCM:
           		case WAVE_FORMAT_MULAW:
           		case WAVE_FORMAT_ALAW:
           		case WAVE_FORMAT_ADPCM:
           		case WAVE_FORMAT_DVI_ADPCM:
           		#if MP3_DECODE_ENABLE == 1
           		case WAVE_FORMAT_MPEGLAYER3:
           		#endif
           		#if AAC_DECODE_ENABLE == 1
           		case WAVE_FORMAT_RAW_AAC1:
           		case WAVE_FORMAT_MPEG_ADTS_AAC:
           		#endif
	   				DEBUG_MSG(DBG_PRINT("AudFormat = 0x%X\r\n", p_wave_info->wFormatTag));
	   				vid_dec_set_audio_flag(TRUE);
	   				p_vid_dec_para->aud_frame_size = AUDIO_PCM_BUFFER_SIZE;
	   				break;
           			
           		default:
           			DEBUG_MSG(DBG_PRINT("NotSupportAudFormat!!!\r\n"));
           			vid_dec_set_audio_flag(FALSE);
           			p_wave_info = NULL;
           			break;
           		}
           		
           		p_vid_dec_para->video_format = vid_dec_get_video_format(p_bitmap_info->biCompression);
       			switch(p_vid_dec_para->video_format)
       			{
       			case C_MJPG_FORMAT:
       				vid_dec_set_video_flag(TRUE);
       				break;
       				
       			#if MPEG4_DECODE_ENABLE == 1
       			case C_XVID_FORMAT:
       			case C_M4S2_FORMAT:
       			case C_H263_FORMAT:
       				if(p_bitmap_info->biWidth%16 != 0 || p_bitmap_info->biWidth > 768)
       				{
       					DEBUG_MSG(DBG_PRINT("Width is not 16-Align or > 768\r\n"));
       					vid_dec_set_video_flag(FALSE);
       				}
       				else
       				{
       					vid_dec_set_video_flag(TRUE);
       				}
       				break;
				#endif
				
       			default:
       				vid_dec_set_video_flag(FALSE);
       				p_bitmap_info = NULL;
       				break;
       			}
				
				// no audio and no video
           		if(vid_dec_get_audio_flag() == FALSE && vid_dec_get_video_flag() == FALSE)
           		{	
           			vid_dec_parser_end();	
	       			OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL);
	       			continue;
           		}
				
           		if(p_wave_info) 
           		{
           			p_vid_dec_para->n = p_wave_info->nSamplesPerSec*2/TIME_BASE_TICK_RATE; 
           			p_vid_dec_para->tick_time = (INT64S)p_vid_dec_para->VidTickRate * p_vid_dec_para->n;
           			p_vid_dec_para->time_range = p_vid_dec_para->tick_time * TIME_BASE_TICK_RATE >> 2; // about 256ms
           		}
           		else
           		{
           			p_vid_dec_para->n = 1;
           			p_vid_dec_para->tick_time = (INT64S)p_vid_dec_para->VidTickRate * p_vid_dec_para->n;
           			p_vid_dec_para->time_range = p_vid_dec_para->tick_time * TIME_BASE_TICK_RATE >> 2; // about 256ms
           		}
           		p_vid_dec_para->tick_time2 = p_vid_dec_para->tick_time;
           		OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_SUCCESS);
           	}
           	break; 	
           	
        case MSG_VIDEO_DECODE_PARSER_HEADER_STOP:
           	vid_dec_parser_end();
           	vid_dec_memory_free();
           	OSQFlush(vid_dec_state_q);
           	OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_SUCCESS); 
           	break;
           
        case MSG_VIDEO_DECODE_START:
        	if(vid_dec_memory_alloc() < 0)
        	{
        		vid_dec_memory_free();
        		OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL);
        		continue; 
        	}	
        
        	if(vid_dec_get_video_flag())
    		{
    			if(video_decode_task_start() < 0)
    			{
    				OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL); 
    				continue;
    			}		
    			vid_dec_set_status(C_VIDEO_DECODE_PLAYING);
    		}
        		
    		if(vid_dec_get_audio_flag())
    		{
    			if(audio_decode_task_start() < 0)	
    			{
    				OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL); 
    				continue;
    			}	
    			vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
    		}
    			
        	if(vid_dec_get_video_flag())			
        		vid_dec_start_timer();

        	OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_SUCCESS); 
        	break;
        	
        case MSG_VIDEO_DECODE_STOP:
        	if(vid_dec_get_video_flag())
        	{
        		vid_dec_stop_timer();
        		if(video_decode_task_stop() < 0)
        		{
        			OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL); 
    				continue;
        		}
        		vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
        	}
        		
        	if(vid_dec_get_audio_flag())
        	{
        		if(audio_decode_task_stop() < 0)
        		{
        			OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL); 
    				continue;
        		}
        		vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
			}
				
        	OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_SUCCESS); 	
        	break;
        
        case MSG_VIDEO_DECODE_PAUSE:
        	if(vid_dec_get_video_flag())
        		vid_dec_stop_timer();
        				
        	if(vid_dec_get_audio_flag())
        		aud_dec_dac_stop();	
        			
        	OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_SUCCESS); 	
        	break;
        
        case MSG_VIDEO_DECODE_RESUME:
			if(vid_dec_get_video_flag())
				vid_dec_start_timer();
			
			if(vid_dec_get_audio_flag())
				aud_dec_dac_start(p_wave_info->nChannels, p_wave_info->nSamplesPerSec);
   			
        	OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_SUCCESS); 	
        	break;
        	
        case MSG_VIDEO_DECODE_SET_PLAY_TIME:
        	if(vid_dec_get_video_flag())
    		{
    			if(video_decode_task_restart() < 0)
    			{
    				OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL); 
    				continue;
    			}		
    			vid_dec_set_status(C_VIDEO_DECODE_PLAYING);
    		}
        		
        	if(vid_dec_get_audio_flag())
        	{
        		if(audio_decode_task_start() < 0)
        		{
        			OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL); 
    				continue;
        		}	
        		vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
        	}	
        	
        	if(vid_dec_get_video_flag())		
        		vid_dec_start_timer();
        	
        	OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_SUCCESS); 		
        	break;
        	
        case MSG_VIDEO_DECODE_NTH_FRAME:
        	if(vid_dec_memory_alloc() < 0)
        	{
        		vid_dec_memory_free();
        		OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL); 
        		continue;
        	}	
        	
        	if(video_decode_task_nth_frame() < 0)
        	{
        		OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_FAIL); 
        		continue;
        	}
        	
        	OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_SUCCESS); 	
        	break;	
        
        case MSG_VIDEO_DECODE_ERROR:
        	error_id = 0x80000000;	
        case MSG_VIDEO_DECODE_FORCE_STOP:
           	video_decode_task_stop();
        	vid_dec_parser_end();
           	vid_dec_memory_free();
           	OSQFlush(vid_dec_state_q);
           	if(error_id) video_decode_error_callback();
        	break;
        			 	
        case MSG_VIDEO_DECODE_EXIT:
        	OSMboxPost(vid_dec_state_ack_m, (void*)C_ACK_SUCCESS); 
   			OSTaskDel(OS_PRIO_SELF);
        	break;
        }  
    }
}
