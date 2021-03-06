/*
* Purpose: Audio task for sending data to DAC.
*
* Author: Allen Lin
*
* Date: 2008/5/22
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.00
* History :
*/
#include "ztkconfigs.h"
#include "turnkey_audio_dac_task.h"

/* for debug */
#define DEBUG_TURNKEY_AUDIO_DAC_TASK	1
#if DEBUG_TURNKEY_AUDIO_DAC_TASK
    #include "gplib.h"
    #define _dmsg(x)			print_string x
#else
    #define _dmsg(x)
#endif

/* definitions */
#define AUDIO_DAC_QUEUE_MAX		5
#define AUDIO_DAC_SENDQ_SIZE		MAX_DAC_BUFFERS
#define AUDIO_AVI_Q_SIZE		1
#define AUDIO_RIGHT_Q_SIZE		1

#define SPU_LEFT_CH_BIT			(1 << SPU_LEFT_CH)
#define SPU_RIGHT_CH_BIT		(1 << SPU_RIGHT_CH)

#define STEP_AUDIO_STOP			4

// Variables defined in this file
/* Task Q declare */
OS_EVENT	*hAudioDacTaskQ;
OS_EVENT    *aud_send_q;
void        *AudioDacTaskQ_area[AUDIO_DAC_QUEUE_MAX];
void		*send_q[AUDIO_DAC_SENDQ_SIZE];
INT32U      last_send_idx;

#if AUDIO_BG_DECODE_EN == 1
OS_EVENT    *aud_right_q;
OS_EVENT    *aud_bg_send_q;
void		*send_bg_q[AUDIO_DAC_SENDQ_SIZE];
void		*aud_right[AUDIO_RIGHT_Q_SIZE];
#endif

#if 0
OS_EVENT    *aud_avi_q;
void		*avi_q[AUDIO_AVI_Q_SIZE];
FP64		g_avi_synchroframe;
FP64		g_avi_synchrocnt;
INT32S		g_avi_datalength;
#endif
extern INT32U dac_buf_nums;

#if (defined AUDIO_PROGRESS_SUPPORT) && (AUDIO_PROGRESS_SUPPORT == CUSTOM_ON)
ST_AUDIO_PLAY_TIME aud_time;
#endif

/* Proto types */
void audio_dac_task_init(void);
void audio_dac_task_entry(void *p_arg);
void audio_spu_softch_start(void);
void audio_spu_loopaddr_set(INT8U r_idx);
void audio_spu_fiq_isr(void);
void audio_dac_ramp_isr(void);

void audio_dac_task_init(void)
{
    /* Create MsgQueue/MsgBox for TASK */
    hAudioDacTaskQ = OSQCreate(AudioDacTaskQ_area, AUDIO_DAC_QUEUE_MAX);
    aud_send_q = OSQCreate(send_q, AUDIO_DAC_SENDQ_SIZE);
    
    #if AUDIO_BG_DECODE_EN == 1
    aud_bg_send_q = OSQCreate(send_bg_q, AUDIO_DAC_SENDQ_SIZE);
	aud_right_q = OSQCreate(aud_right, AUDIO_RIGHT_Q_SIZE);
	#endif
	
	#if 0
	aud_avi_q = OSQCreate(avi_q, AUDIO_AVI_Q_SIZE);
	g_avi_synchroframe = 0;
	g_avi_synchrocnt = 0;
	g_avi_datalength = 4000;
	#endif
}

void audio_dac_task_entry(void *p_arg)
{
	INT8U		err = 0;
	INT8U           pause = 0;
	INT32S		audio_dac_msg;
	INT32U          r_idx = 0;
	INT32U          w_idx = 0;
#if AUDIO_BG_DECODE_EN == 1
	INT32U          r_idx_bg = 0;
	INT32U          w_idx_bg = 0;
#endif
#if (defined AUDIO_PROGRESS_SUPPORT) && (AUDIO_PROGRESS_SUPPORT == CUSTOM_ON)
	INT32U		total_samples = 0;
	INT32U		pcmlen;
	double		times;
#endif
#if AUDIO_PRIOR_DYNAMIC_SW == 1
	INT32U		hiprio_en = 0;
	INT8U		audio_playing = 0;
	OS_Q		*pq;
#endif
	INT8U		reverse = 0;

	audio_dac_task_init();

	/* Task Star */
	while (1) {
	    /* Pend task message */
        audio_dac_msg = (INT32S) OSQPend(hAudioDacTaskQ, 0, &err);
        switch(audio_dac_msg) {
			case C_DMA_STATUS_DONE:
			    if (pause != 1) {
			      #if AUDIO_PRIOR_DYNAMIC_SW == 1
			    	pq = (OS_Q *)aud_send_q->OSEventPtr;
			    	if ((pq->OSQEntries < 3) && !hiprio_en && audio_playing) {
						OSTaskChangePrio(AUD_DEC_PRIORITY,AUD_DEC_HIGH_PRIORITY);
						hiprio_en = 1;
						//DBG_PRINT("hi prio\r\n");
					}
					else if (hiprio_en && (pq->OSQEntries > 4)) {
						OSTaskChangePrio(AUD_DEC_HIGH_PRIORITY,AUD_DEC_PRIORITY);
						hiprio_en = 0;
						//DBG_PRINT("low prio\r\n");
					}
				  #endif
			        r_idx = (INT32U)OSQAccept(aud_send_q, &err);
				    if (err == OS_NO_ERR) {
					    dac_cha_dbf_set(pcm_out[r_idx], pcm_len[r_idx]);
					    last_send_idx = r_idx;
				    }
			    }
			    
			  #if (defined AUDIO_PROGRESS_SUPPORT) && (AUDIO_PROGRESS_SUPPORT == CUSTOM_ON)
				pcmlen = pcm_len[w_idx];
	            if (channel == 2) {
		            pcmlen >>= 1;
	            }
	            if (reverse == 0) {
				    total_samples += pcmlen;
				} else {
				    if (total_samples >= pcmlen) {
				        total_samples -= pcmlen;
			        } else {
			            total_samples = 0;
			        }
			    }
				times = (double)total_samples/(double)g_audio_sample_rate;
				if (((INT32U) times > aud_time.curr_play_time) || ((INT32U) times < aud_time.curr_play_time)) {
				    aud_time.curr_play_time = (INT32U) times;
//				    if (start_time_send && aud_time.curr_play_time <=  aud_time.total_play_time) {
//				        msgQSend(ApQ, MSG_AUD_PLAY_TIME_RES, (void *)&aud_time, sizeof(STAudioConfirm), MSG_PRI_NORMAL);
//				    }    
				}
			  #endif
				
				OSQPost(audio_wq, (void *)w_idx);
				#if 0
				g_avi_datalength -= (INT32S)pcm_len[w_idx];
				if(g_avi_datalength <= 0)
				{
					g_avi_synchroframe +=g_avi_synchrocnt;
					g_avi_datalength += 4000;
					OSQPost(aud_avi_q, NULL);
				}
				#endif
				//w_idx = (w_idx+1) % MAX_DAC_BUFFERS;
				if (++w_idx == dac_buf_nums) {
				    w_idx = 0;
				}
				break;
		  #if AUDIO_PRIOR_DYNAMIC_SW == 1
			case MSG_AUD_LOW_PRIORITY_SET:
				if (hiprio_en) {
					OSTaskChangePrio(AUD_DEC_HIGH_PRIORITY,AUD_DEC_PRIORITY);
					hiprio_en = 0;
					//DBG_PRINT("recovery to low prio\r\n");
				}
				break;
			case MSG_AUD_DAC_STOP:
				audio_playing = 0;
				break;
		  #endif
			case MSG_AUD_DMA_DBF_START:
				#if 0
				OSQPost(aud_avi_q, NULL);
				#endif
				r_idx = (INT32U)OSQAccept (aud_send_q, &err);
				dac_cha_dbf_put(pcm_out[r_idx], pcm_len[r_idx], hAudioDacTaskQ);
				r_idx = (INT32U)OSQAccept (aud_send_q, &err);
				dac_cha_dbf_set(pcm_out[r_idx], pcm_len[r_idx]);
				pause = 0;
				audio_playing = 1;
				//DBG_PRINT("dma start\r\n");
				break;
			case MSG_AUD_DMA_DBF_RESTART:
				r_idx = (INT32U)OSQAccept(aud_send_q, &err);
				if (err == OS_NO_ERR) {
					dac_cha_dbf_set(pcm_out[r_idx], pcm_len[r_idx]);
				}
				DBG_PRINT("D..\r\n");
				break;
			case MSG_AUD_DMA_WIDX_CLEAR:
				w_idx = 0;
				break;
			case MSG_AUD_DMA_PAUSE:
			    pause = 1;
			    //DBG_PRINT("dma pause\r\n");
			    break;
			#if AUDIO_BG_DECODE_EN == 1
			case MSG_AUD_SPU_SOFTCH_START:
				audio_spu_softch_start();
				break;
			case MSG_AUD_SPU_LEFT_DONE:
				if ((SPU_GetChannelEnableStatus() & SPU_LEFT_CH_BIT) == 0) {
					break;
				}	
				r_idx_bg = (INT32U)OSQAccept(aud_bg_send_q, &err);
				if (err == OS_NO_ERR) {
					audio_spu_loopaddr_set(r_idx_bg);
				}
				else {
					DBG_PRINT("SPU underrun\r\n");
					SPU_StopChannel(SPU_LEFT_CH);
					SPU_StopChannel(SPU_RIGHT_CH);
					SPU_Disable_FIQ_Channel(SPU_LEFT_CH);
					SPU_Disable_FIQ_Channel(SPU_RIGHT_CH);
					msgQSend(AudioBGTaskQ, MSG_AUD_SPU_RESTART, NULL, 0, MSG_PRI_URGENT);
				}
				OSQPost(audio_bg_wq, (void *)w_idx_bg);
				//w_idx_bg = (w_idx_bg+1) % MAX_DAC_BUFFERS;
				if (++w_idx_bg == dac_buf_nums) {
				    w_idx_bg = 0;
				}
				break;
			case MSG_AUD_SPU_WIDX_CLEAR:
				w_idx_bg = 0;
				break;
			case MSG_AUD_SPU_WIDX_SET:
				w_idx_bg = (INT32U)OSQAccept(aud_bg_send_q, &err);
				if (err == OS_NO_ERR) {
					OSQPostFront(aud_bg_send_q, (void *)w_idx_bg);
				}
				else {
					w_idx_bg = 0;
				}	
				break;
			case MSG_AUD_RAMP_DOWN_START:
				sys_registe_timer_isr(audio_dac_ramp_isr);
				break;
			#endif
			#if (defined AUDIO_PROGRESS_SUPPORT) && (AUDIO_PROGRESS_SUPPORT == CUSTOM_ON)
			case MSG_AUD_START_SAMPLES_COUNT:
			    total_samples = 0;
			    aud_time.total_play_time = total_play_time;
			    aud_time.curr_play_time = 0;
			    if (start_time_send) {
			        msgQSend(ApQ, MSG_AUD_PLAY_TIME_RES, (void *)&aud_time, sizeof(STAudioConfirm), MSG_PRI_NORMAL);
			    }
			    break;
			#endif
			case MSG_AUD_DAC_REVRESE:
				reverse = 1;
				break;
			case MSG_AUD_DAC_REVRESE_CLEAR:
				reverse = 0;
				break;
			default:
				break;
		}
	}
}
INT32S audio_dac_get_curr_play_time(void)
{
	return aud_time.curr_play_time;
}

INT32S audio_dac_get_total_time(void)
{
	return aud_time.total_play_time;
}

#if AUDIO_BG_DECODE_EN == 1
void audio_spu_softch_start(void)
{
	INT8U   pitch,velocity,err;
	INT16S  *buff_addr;
	INT32U  l_addr_a;
	INT32U  r_addr_a;
	INT32U  l_addr_b;
	INT32U  r_addr_b;

	INT8U   EDD;
	INT8U   r_idx;
	INT32U  pcmlen;
	INT32U  phase;

	pitch = 60;
	velocity = 127;

	r_idx = (INT32U)OSQAccept (aud_bg_send_q, &err);
	buff_addr = pcm_bg_out[r_idx];
	pcmlen = pcm_bg_len[r_idx];

	l_addr_a = (INT32U)buff_addr;

	if (channel == 2) {
		pcmlen >>= 1;
	}
	r_addr_a = (INT32U)(buff_addr + pcmlen + 1);

	r_idx = (INT32U)OSQAccept (aud_bg_send_q, &err);
	buff_addr = pcm_bg_out[r_idx];
	pcmlen = pcm_bg_len[r_idx];

	l_addr_b = (INT32U)buff_addr;

	if (channel == 2) {
		pcmlen >>= 1;
	}
	r_addr_b = (INT32U)(buff_addr + pcmlen + 1);


	SPU_PlayTone1(pitch, 0, velocity, SPU_LEFT_CH);
	SPU_PlayTone1(pitch, 127, velocity, SPU_RIGHT_CH);

	l_addr_a = l_addr_a / 2;
	SPU_SetStartAddress(l_addr_a, SPU_LEFT_CH);

	r_addr_a = r_addr_a / 2;
	SPU_SetStartAddress(r_addr_a, SPU_RIGHT_CH);
	//DBG_PRINT("r_idx = %d, l_addr = 0x%x, r_addr = 0x%x\r\n",0,l_addr_a,r_addr_a);

	l_addr_b = l_addr_b / 2;
	SPU_SetLoopAddress(l_addr_b, SPU_LEFT_CH);

	r_addr_b = r_addr_b / 2;
	SPU_SetLoopAddress(r_addr_b, SPU_RIGHT_CH);
	//DBG_PRINT("r_idx = %d, l_addr = 0x%x, r_addr = 0x%x\r\n",1,l_addr_b,r_addr_b);

	phase = ((sample_rate << 13)/281250) << 6;

	SPU_SetPhase(phase, SPU_LEFT_CH);
	SPU_SetPhase(phase, SPU_RIGHT_CH);

	SPU_EnvelopeManualMode(SPU_LEFT_CH);
	SPU_EnvelopeManualMode(SPU_RIGHT_CH);

	EDD = 0x7F;
	SPU_SetEnvelopeData(EDD, SPU_LEFT_CH);
	SPU_SetEnvelopeData(EDD, SPU_RIGHT_CH);

	SPU_SetToneColorMode(0x02, SPU_LEFT_CH);
	SPU_SetToneColorMode(0x02, SPU_RIGHT_CH);
	
	SPU_SingleChVolumeSelect(0x03);
	//SPU_Set_MainVolume(0x3F);
	OSQFlush(aud_right_q);

	SPU_AttachISR(C_SPU_FIQ, audio_spu_fiq_isr);
	SPU_Enable_Channel(SPU_LEFT_CH);
	SPU_Enable_Channel(SPU_RIGHT_CH);

	SPU_Clear_Ch_StopFlag(SPU_LEFT_CH);
	SPU_Clear_Ch_StopFlag(SPU_RIGHT_CH);

	SPU_Enable_FIQ_Channel(SPU_LEFT_CH);
	SPU_Enable_FIQ_Channel(SPU_RIGHT_CH);
}

void audio_spu_loopaddr_set(INT8U r_idx)
{
	INT16S *buff_addr;
	INT32U pcmlen;
	INT32U l_addr,r_addr;
	INT8U  err;
	INT32U  loopaddr;
	INT32S  i;

	buff_addr = pcm_bg_out[r_idx];
	pcmlen = pcm_bg_len[r_idx];

	OSQPend(aud_right_q, 0, &err); /* wait right channel done */

	l_addr = (INT32U)buff_addr;

	if (channel == 2) {
		pcmlen >>= 1;
	}
	r_addr = (INT32U)(buff_addr + pcmlen +1);

	l_addr = l_addr / 2;
	SPU_SetLoopAddress(l_addr, SPU_LEFT_CH);

	for (i=0;i<10;i++) {
		SPU_GetLoopAddress(&loopaddr,SPU_LEFT_CH);
		if (loopaddr != l_addr) { /* set agian */
			//DBG_PRINT("set left agian,%d\r\n",i);
			SPU_SetLoopAddress(l_addr, SPU_LEFT_CH);
        }
		else {
			break;
	    }
	}

	r_addr = r_addr/2;
	SPU_SetLoopAddress(r_addr, SPU_RIGHT_CH);

	for (i=0;i<10;i++) {
		SPU_GetLoopAddress(&loopaddr,SPU_RIGHT_CH);
		if (loopaddr != r_addr) { /* set agian */
			//DBG_PRINT("set right agian,%d\r\n",i);
			SPU_SetLoopAddress(r_addr, SPU_RIGHT_CH);
		}
		else {
			break;
		}
	 }

	//if (cnt < 4) {
	//	DBG_PRINT("r_idx = %d, l_addr = 0x%x, r_addr = 0x%x\r\n",r_idx,l_addr,r_addr);
	//	cnt++;
	//}

}

void audio_spu_fiq_isr(void)
{
	INT32U status;

	status = SPU_Get_FIQ_Status();
	if (status & SPU_LEFT_CH_BIT) {
		SPU_Clear_FIQ_Status(SPU_LEFT_CH_BIT);
		OSQPost(hAudioDacTaskQ, (void *)MSG_AUD_SPU_LEFT_DONE);
	}

	if (status & SPU_RIGHT_CH_BIT) {
		SPU_Clear_FIQ_Status(SPU_RIGHT_CH_BIT);
		OSQPost(aud_right_q, (void *)MSG_AUD_SPU_RIGHT_DONE);
	}
}

void audio_dac_ramp_isr(void)
{
	INT8S edd1 = 0;
	INT8S edd2 = 0;

	edd1 = SPU_ReadEnvelopeData(SPU_LEFT_CH);
	edd1 -= edd_step;
	if (edd1 < 0) {
		edd1 = 0;
	}
		
	SPU_SetEnvelopeData(edd1, SPU_LEFT_CH);
	
	edd2 = SPU_ReadEnvelopeData(SPU_RIGHT_CH);
	edd2 -= edd_step;
	if (edd2 < 0) {
		edd2 = 0;
	}
	
	SPU_SetEnvelopeData(edd2, SPU_RIGHT_CH);
	
	if (edd1 == 0 && edd2 == 0) {
		sys_release_timer_isr(audio_dac_ramp_isr);
		DBG_PRINT("edd to 0\r\n");
	}		
}
#endif