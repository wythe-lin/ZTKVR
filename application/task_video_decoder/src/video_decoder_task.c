#include "task_video_decoder.h"

#define C_VIDEO_DECODE_STACK_SIZE		512	
#define C_VIDEO_Q_ACCEPT_MAX			5

/*os task stack */
INT32U	video_decode_stack[C_VIDEO_DECODE_STACK_SIZE];

/* os task queue */
OS_EVENT *vid_dec_q;
OS_EVENT *vid_dec_ack_m;
void *vid_dec_q_buffer[C_VIDEO_Q_ACCEPT_MAX];

INT32S video_decode_task_start(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	SEND_MESSAGE(vid_dec_q, MSG_VID_DEC_START, vid_dec_ack_m, 0, msg, err);
Return:		
	return nRet;
}

INT32S video_decode_task_restart(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	SEND_MESSAGE(vid_dec_q, MSG_VID_DEC_RESTART, vid_dec_ack_m, 0, msg, err);
Return:		
	return nRet;
}

INT32S video_decode_task_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	SEND_MESSAGE(vid_dec_q, MSG_VID_DEC_STOP, vid_dec_ack_m, 0, msg, err);
Return:	
	return nRet;
}

INT32S video_decode_task_nth_frame(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	SEND_MESSAGE(vid_dec_q, MSG_VID_DEC_NTH, vid_dec_ack_m, 0, msg, err);
Return:	
	return nRet;
}

INT32S  video_decode_task_create(INT8U proi)
{
	INT8U  err;
	INT32S nRet;
	
	vid_dec_q = OSQCreate(vid_dec_q_buffer, C_VIDEO_Q_ACCEPT_MAX);
	if(!vid_dec_q) RETURN(STATUS_FAIL);
	
	vid_dec_ack_m = OSMboxCreate(NULL);
	if(!vid_dec_ack_m) RETURN(STATUS_FAIL);
	
	err = OSTaskCreate(video_decode_task_entry, (void *)NULL, &video_decode_stack[C_VIDEO_DECODE_STACK_SIZE - 1], proi);	
	if(err != OS_NO_ERR) RETURN(STATUS_FAIL);

	nRet = STATUS_OK;	
Return:	
	return nRet;		
}

INT32S video_decode_task_del(INT8U proi)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	SEND_MESSAGE(vid_dec_q, MSG_VID_DEC_EXIT, vid_dec_ack_m, 0, msg, err);
Return: 	
	OSQFlush(vid_dec_q);
	OSQDel(vid_dec_q, OS_DEL_ALWAYS, &err);
	OSMboxDel(vid_dec_ack_m, OS_DEL_ALWAYS, &err);	
	return STATUS_OK;
}

static void video_decode_task_end(INT32U deblock_iram_addr)
{
	if(p_vid_dec_para->video_format == C_MJPG_FORMAT)
	{
		mjpeg_decode_stop_all(p_vid_dec_para->scaler_flag);
	}
#if MPEG4_DECODE_ENABLE == 1
	else
	{
		if(p_vid_dec_para->scaler_flag)
			scaler_size_wait_done();

		mpeg4_decode_stop();
		if(p_vid_dec_para->deblock_flag)
		{
			drvl1_mp4_deblock_stop();
			gp_free((void*)deblock_iram_addr);
		}
	}
#endif
}

void video_decode_task_entry(void *parm)
{
	INT8U  success_flag, init_flag, nth_flag;
	INT8U  err, time_inc_len, quant;
	INT16U *pwdata, width, height;
	INT32S coding_type, status, error_id;
	INT32U raw_data_addr, decode_addr, refer_addr, display_addr;
	INT32U deblock_addr, deblock_iram_addr;
	INT32U scaler_in_addr, scaler_out_addr, old_scaler_out_addr;
	INT32U msg_id;
	
	INT64S delta_Tv, old_delta_Tv, temp;
	long Duration, size;
	OS_CPU_SR cpu_sr;
#if MPEG4_DECODE_ENABLE == 1
	INT8U  mpeg4_type;
	INT32U key_word;
#endif
	
	nth_flag = 0;
	while(1)
	{
		msg_id = (INT32U) OSQPend(vid_dec_q, 0, &err);
		if((err != OS_NO_ERR)||	!msg_id)
			continue;
			
		switch(msg_id)
		{
		case MSG_VID_DEC_ONE_FRAME:
			p_vid_dec_para->pend_cnt++;
			success_flag = display_addr = 0;
			if(!raw_data_addr || size <= 0) // read video data speed too slow
			{
				if(MultiMediaParser_IsEOV(p_vid_dec_para->media_handle))
				{
					DEBUG_MSG(DBG_PRINT("VidDecEnd.\r\n"));
					video_decode_task_end(deblock_iram_addr);
					vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
					vid_dec_end_callback();
					p_vid_dec_para->pend_cnt = p_vid_dec_para->post_cnt;
					OSQFlush(vid_dec_q);
					continue;
				}
				goto EndOf_MSG_VID_DEC_ONE_FRAME;
			}
			
			OS_ENTER_CRITICAL();
			temp = (INT64S)(p_vid_dec_para->tv - p_vid_dec_para->Tv);
			OS_EXIT_CRITICAL();
			if(temp > p_vid_dec_para->time_range) // check sync
			{
				if(p_vid_dec_para->fail_cnt < 20)
				{
					p_vid_dec_para->fail_cnt++;
					MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, p_vid_dec_para->fail_cnt);
					DEBUG_MSG(DBG_PRINT("FailCnt = %d\r\n", p_vid_dec_para->fail_cnt));
				}							
			}
			else
			{
				if(p_vid_dec_para->fail_cnt > 0)
				{
					p_vid_dec_para->fail_cnt--;
					MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, p_vid_dec_para->fail_cnt);	
					DEBUG_MSG(DBG_PRINT("FailCnt = %d\r\n", p_vid_dec_para->fail_cnt));
				}
			}
						
			switch(p_vid_dec_para->video_format) 
			{
			case C_MJPG_FORMAT:
				coding_type = C_UNKNOW_VOP;
				display_addr = decode_addr = vid_dec_get_next_vid_buffer();
				if(p_vid_dec_para->scaler_flag)
				{
					status = mjpeg_decode_and_scaler(p_vid_dec_para->scaler_flag, raw_data_addr, size, decode_addr, 
													 p_vid_dec_para->image_output_width, p_vid_dec_para->image_output_height, 
													 p_vid_dec_para->buffer_output_width, p_vid_dec_para->buffer_output_height);
				}
				else
				{
					status = mjpeg_decode_without_scaler(raw_data_addr, size, decode_addr);
				}
				
				if(status < 0) 
				{
					DEBUG_MSG(DBG_PRINT("JpegDecFail = %d\r\n", status));
					goto EndOf_MSG_VID_DEC_ONE_FRAME;
				}
				break;
			
			#if MPEG4_DECODE_ENABLE == 1	
			case C_XVID_FORMAT:
			case C_M4S2_FORMAT:
				coding_type = vid_dec_paser_bit_stream((INT8U*)raw_data_addr, size, &width, &height, &time_inc_len, &quant);
				if(init_flag) break;
				if(((coding_type == C_I_VOP)||(coding_type == C_P_VOP)) && time_inc_len)
				{
					init_flag = 1;
					mpeg4_decode_config(mpeg4_type, p_vid_dec_para->mpeg4_decode_out_format, width, height, time_inc_len - 1);
				}
				else if(coding_type & ERROR_00)
				{
					video_decode_task_end(deblock_iram_addr);
					vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
					vid_dec_end_callback();
					p_vid_dec_para->pend_cnt = p_vid_dec_para->post_cnt;
					OSQFlush(vid_dec_q);
					continue;
				}
				else 
				{
					success_flag = 1;
					goto EndOf_MSG_VID_DEC_ONE_FRAME;
				}
				break;
				
			case C_H263_FORMAT:
				coding_type = vid_dec_paser_h263_bit_stream((INT8U*)raw_data_addr, &width, &height, &key_word);
				drvl1_mp4_decode_match_code_set(0x03, key_word);
				if(init_flag) break;
				if((coding_type & ERROR_00) == 0)
				{
					init_flag = 1;
					mpeg4_decode_config(mpeg4_type, p_vid_dec_para->mpeg4_decode_out_format, width, height, 0);
				}
				else 
				{
					success_flag = 1;
					goto EndOf_MSG_VID_DEC_ONE_FRAME;
				}
				break;
			#endif
			
			#if S263_DECODE_ENABLE == 1	
			case C_SOREN_H263_FORMAT:	
				coding_type = vid_dec_paser_sorenson263_bit_stream((INT8U*)raw_data_addr, &width, &height, &key_word);
				drvl1_mp4_decode_match_code_set(0x03, key_word);
				if(init_flag) break;
				if((coding_type & ERROR_00) == 0)
				{
					init_flag = 1;
					mpeg4_decode_config(mpeg4_type, p_vid_dec_para->mpeg4_decode_out_format, width, height, coding_type);
				}
				else 
				{
					success_flag = 1;
					goto EndOf_MSG_VID_DEC_ONE_FRAME;
				}
				break;
			#endif
			}
				
			#if MPEG4_DECODE_ENABLE == 1
			switch(coding_type)
			{
			case C_I_VOP:
			case C_P_VOP:
				refer_addr = decode_addr;
				old_scaler_out_addr = scaler_out_addr;
				if(p_vid_dec_para->deblock_flag)
				{	
					deblock_addr = vid_dec_get_next_deblock_buffer();
					drvl1_mp4_deblock_set(quant, 1);
					drvl1_mp4_deblock_start(deblock_addr);
				}
				
				decode_addr = vid_dec_get_next_vid_buffer();	
				mpeg4_decode_start(raw_data_addr, decode_addr, refer_addr);
				status = mpeg4_wait_idle(TRUE);
 				if(status != C_MP4_STATUS_END_OF_FRAME)
				{	//re-init
					DEBUG_MSG(DBG_PRINT("Mpeg4DecFail = 0x%x\r\n", status));
					mpeg4_decode_init();
					mpeg4_decode_config(mpeg4_type, p_vid_dec_para->mpeg4_decode_out_format, width, height, time_inc_len - 1);
					success_flag = 1;
					goto EndOf_MSG_VID_DEC_ONE_FRAME;
				}
				else
					mpeg4_decode_stop();

				if(p_vid_dec_para->scaler_flag && p_vid_dec_para->deblock_flag)
				{
					scaler_in_addr = deblock_addr;
					display_addr = old_scaler_out_addr;
				}
				else if(p_vid_dec_para->scaler_flag)
				{
					scaler_in_addr = decode_addr;
					display_addr = old_scaler_out_addr;
				}
				else if(p_vid_dec_para->deblock_flag)
				{
					display_addr = deblock_addr;
				}
				else
				{
					display_addr = decode_addr;
				}
					
				if(p_vid_dec_para->scaler_flag)
				{
					scaler_out_addr = vid_dec_get_next_scaler_buffer();
					scaler_size_wait_done();
					scaler_size_start(p_vid_dec_para->scaler_flag, scaler_in_addr, p_vid_dec_para->video_decode_in_format, 
									width, height,scaler_out_addr, p_vid_dec_para->video_decode_out_format, 
									p_vid_dec_para->image_output_width, p_vid_dec_para->image_output_height, 
									p_vid_dec_para->buffer_output_width, p_vid_dec_para->buffer_output_height);
				}
				break;
				
			case C_N_VOP:
				success_flag = 1;
				goto EndOf_MSG_VID_DEC_ONE_FRAME;
				break;
				
			case C_UNKNOW_VOP:
				break;		
			
			default:
				// B frame of others
				DEBUG_MSG(DBG_PRINT("CodingType = %d\r\n", coding_type));
				success_flag = 1;
				goto EndOf_MSG_VID_DEC_ONE_FRAME;
				break;		
			}
			#endif
			
			success_flag = 1;
			if(display_addr && !nth_flag)	
			{
			#if _PROJ_TYPE == _PROJ_TURNKEY	
				OSQPost(DisplayTaskQ, (void *) (display_addr|MSG_DISPLAY_TASK_MJPEG_DRAW));
			#else
				video_decode_FrameReady((INT8U*)display_addr);
			#endif	
			}
			
EndOf_MSG_VID_DEC_ONE_FRAME:
			if(success_flag)
			{
				if(p_vid_dec_para->scaler_flag && (p_vid_dec_para->video_format != C_MJPG_FORMAT))
				{
					OS_ENTER_CRITICAL();
					p_vid_dec_para->Tv += old_delta_Tv;
					OS_EXIT_CRITICAL();
					old_delta_Tv = delta_Tv;
				}
				else
				{
					OS_ENTER_CRITICAL();
					p_vid_dec_para->Tv += delta_Tv;
					OS_EXIT_CRITICAL();
				}						
			}
			
			// get video raw data
			raw_data_addr = (INT32U)MultiMediaParser_GetVidBuf(p_vid_dec_para->media_handle, &size, &Duration, &error_id);
			size += raw_data_addr & 3;	//align and size increase
			raw_data_addr -= raw_data_addr & 3;	//align addr	
			while(error_id<0);
			
			if(p_wave_info)
			{
           		delta_Tv = (INT64S)Duration * p_wave_info->nSamplesPerSec*2;
       		}
       		else
       		{
       			p_vid_dec_para->n = 1;       			
       			delta_Tv = (INT64S)Duration * TIME_BASE_TICK_RATE;
       		}
			break;
		
		case MSG_VID_DEC_START:
			refer_addr = decode_addr = 0; 
			scaler_in_addr = old_scaler_out_addr = scaler_out_addr = 0; 
			deblock_addr = deblock_iram_addr = 0;
		case MSG_VID_DEC_RESTART:
			time_inc_len = quant = init_flag = 0;
			delta_Tv = old_delta_Tv = 0;
			p_vid_dec_para->fail_cnt = 0;
			// fre-latch video raw data
			while(1)
			{	
				raw_data_addr = (INT32U)MultiMediaParser_GetVidBuf(p_vid_dec_para->media_handle, &size, &Duration, &error_id);
				size += raw_data_addr & 3;	//align and size increase
				raw_data_addr -= raw_data_addr & 3;	//align addr	
				while(error_id<0);
				if(nth_flag) 
				{ 
					if(raw_data_addr && (size >= 10)) break;
				}
				else 
				{
					if(raw_data_addr) break;	
				}
				OSTimeDly(1);
			}
			
			if(p_wave_info)
			{
           		delta_Tv = (INT64S)Duration * p_wave_info->nSamplesPerSec*2;
       		}
       		else
   			{
       			p_vid_dec_para->n = 1;       			
       			delta_Tv = (INT64S)Duration * TIME_BASE_TICK_RATE;
       		}	
       			
			switch(p_vid_dec_para->video_format)
			{
			case C_MJPG_FORMAT:
				gplib_jpeg_default_huffman_table_load(); // Fix if jpeg file have no huffman table. 
				coding_type = mjpeg_decode_get_size(raw_data_addr, size, &width, &height);
				if(coding_type >= 0) init_flag = 1;	
				break;
				
			#if MPEG4_DECODE_ENABLE == 1
			case C_XVID_FORMAT:
			case C_M4S2_FORMAT:
				mpeg4_decode_init();
				mpeg4_type = C_MPEG4_CODEC;
				vid_dec_parser_bit_stream_init();
				if(p_vid_dec_para->video_format == C_XVID_FORMAT)
				{
					coding_type = vid_dec_paser_bit_stream((INT8U*)raw_data_addr, size, &width, &height, &time_inc_len, &quant);
				}
				else if(p_vid_dec_para->video_format == C_M4S2_FORMAT)
				{
					coding_type = vid_dec_paser_bit_stream((INT8U*)((INT32U)p_bitmap_info + sizeof(GP_BITMAPINFOHEADER)), 
														g_bitmap_info_len - sizeof(GP_BITMAPINFOHEADER), 
														&width, &height, &time_inc_len, &quant);	
				}
				if(((coding_type == C_I_VOP)||(coding_type == C_P_VOP)) && time_inc_len)
				{
					init_flag = 1;
					mpeg4_decode_config(mpeg4_type, p_vid_dec_para->mpeg4_decode_out_format, width, height, time_inc_len - 1);
				}
				else if(coding_type & ERROR_00)
				{
					goto VID_DEC_START_FAIL;
				}
				else if(nth_flag && !time_inc_len)
				{
					goto VID_DEC_START_FAIL;
				}
				break;
			
			case C_H263_FORMAT:
				mpeg4_decode_init();
				mpeg4_type = C_H263_CODEC;
				coding_type = vid_dec_paser_h263_bit_stream((INT8U*)raw_data_addr, &width, &height, &key_word);
				if((coding_type & ERROR_00) == 0)
				{
					init_flag = 1;
					mpeg4_decode_config(mpeg4_type, p_vid_dec_para->mpeg4_decode_out_format, width, height, 0);
				}
				break;
			#endif
						
			#if S263_DECODE_ENABLE == 1	
			case C_SOREN_H263_FORMAT:
				mpeg4_decode_init();	
				mpeg4_type = C_SORENSON_H263_CODEC;
				coding_type = vid_dec_paser_sorenson263_bit_stream((INT8U*)raw_data_addr, &width, &height, &key_word);
				if((coding_type & ERROR_00) == 0)
				{
					init_flag = 1;
					mpeg4_decode_config(mpeg4_type, p_vid_dec_para->mpeg4_decode_out_format, width, height, coding_type);
				}
				break;
			#endif
			
			default:
				while(1);
			}	
			
			//check header resolution is equal raw data size or not.
			if(init_flag && (p_bitmap_info->biWidth != width || p_bitmap_info->biHeight != height))
			{
				pwdata = (INT16U*)&p_bitmap_info->biWidth;
				*pwdata = width;
				pwdata = (INT16U*)&p_bitmap_info->biHeight;
				*pwdata = height;
				if(vid_dec_memory_realloc() < 0)
					goto VID_DEC_START_FAIL;
			}
			
		#if MPEG4_DECODE_ENABLE == 1	
			if(p_vid_dec_para->deblock_flag && (p_vid_dec_para->video_format != C_MJPG_FORMAT))
			{
				deblock_iram_addr = (INT32U) gp_iram_malloc_align(width*2*2, 4); 
				if(!deblock_iram_addr)
					p_vid_dec_para->deblock_flag = 0;
				else
					drvl1_mp4_deblock_iram_set(deblock_iram_addr);
			}
		#endif
			DEBUG_MSG(DBG_PRINT("VidDecDeblock = 0x%x\r\n", p_vid_dec_para->deblock_flag));
			DEBUG_MSG(DBG_PRINT("VidDecScaler = 0x%x\r\n", p_vid_dec_para->scaler_flag));
			if(nth_flag) break;
			OSMboxPost(vid_dec_ack_m, (void*)C_ACK_SUCCESS);
			break;
VID_DEC_START_FAIL:
			DEBUG_MSG(DBG_PRINT("VideoDecodeStartFail!!!\r\n"));
			nth_flag = 0;
			OSQFlush(vid_dec_q);
			OSMboxPost(vid_dec_ack_m, (void*)C_ACK_FAIL);
			break;
			
		case MSG_VID_DEC_STOP:
			if(nth_flag) nth_flag = 0;
			video_decode_task_end(deblock_iram_addr);
			OSQFlush(vid_dec_q);
			OSMboxPost(vid_dec_ack_m, (void*)C_ACK_SUCCESS);
			break;	
			
		case MSG_VID_DEC_NTH:
			if(OSQPost(vid_dec_q, (void*)MSG_VID_DEC_START) != OS_NO_ERR)
				goto MSG_VID_DEC_NTH_Fail;
			
			if(OSQPost(vid_dec_q, (void*)MSG_VID_DEC_ONE_FRAME) != OS_NO_ERR)
				goto MSG_VID_DEC_NTH_Fail;
			
			if(OSQPost(vid_dec_q, (void*)MSG_VID_DEC_STOP) != OS_NO_ERR)
				goto MSG_VID_DEC_NTH_Fail;
			nth_flag = 1;
			continue;
MSG_VID_DEC_NTH_Fail:
			nth_flag = 0;
			OSQFlush(vid_dec_q);
			OSMboxPost(vid_dec_ack_m, (void*)C_ACK_FAIL);	
			break;
			
		case MSG_VID_DEC_EXIT:
			OSMboxPost(vid_dec_ack_m, (void*)C_ACK_SUCCESS);
			OSTaskDel(OS_PRIO_SELF);
			break;	
		}
	}
}		
