#include "ztkconfigs.h"
#include "task_display.h"

/* for debug */
#define DEBUG_TASK_DISPLAY		1
#if DEBUG_TASK_DISPLAY
    #include "gplib.h"
    #define _dmsg(x)			print_string x
#else
    #define _dmsg(x)
#endif

/* definitions */
#define DISPLAY_TASK_QUEUE_MAX	64

OS_EVENT *DisplayTaskQ;
void *display_task_q_stack[DISPLAY_TASK_QUEUE_MAX];

//	prototypes
void task_display_init(void);

void task_display_init(void)
{	
	ap_display_init();
	DisplayTaskQ = OSQCreate(display_task_q_stack, DISPLAY_TASK_QUEUE_MAX);
	msgQSend(ApQ, MSG_APQ_DISPLAY_TASK_READY, NULL, NULL, MSG_PRI_NORMAL);
}

void task_display_entry(void *para)
{
	INT32U msg_id;
	INT8U err;

	task_display_init();
	while (1) {
		msg_id = (INT32U) OSQPend(DisplayTaskQ, 0, &err);
		if ((!msg_id) || (err != OS_NO_ERR)) {
        		continue;
        	}

        	switch (msg_id & 0xFF000000) {
        	case MSG_DISPLAY_TASK_QUEUE_INIT:
        		ap_display_queue_init();
        		break;
        	case MSG_DISPLAY_TASK_PPU_READY:
        		ap_display_scaler_buff_ack();
        		break;
        	case MSG_DISPLAY_TASK_WAV_TIME_DRAW:
        		ap_display_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPALY_BUFF_SRC_WAV_TIME);
        		break;       		
        	case MSG_DISPLAY_TASK_JPEG_DRAW:
        		ap_display_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPALY_BUFF_SRC_JPEG);
        		break;
        	case MSG_DISPLAY_TASK_MJPEG_DRAW:
        		ap_display_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPALY_BUFF_SRC_MJPEG);
        		break;
        	case MSG_DISPLAY_TASK_SETTING_DRAW:
			ap_display_setting_frame_buff_set(msg_id & 0xFFFFFF);
        		break;
        	case MSG_DISPLAY_TASK_SETTING_INIT:
        		
        		break;
        	case MSG_DISPLAY_TASK_SETTING_EXIT:
        		ap_display_setting_frame_buff_set(NULL);
        		break;
        	case MSG_DISPLAY_TASK_ICON_SHOW:
        		ap_display_icon_sts_set(msg_id);
        		break;
        	case MSG_DISPLAY_TASK_ICON_CLEAR:
        		ap_display_icon_sts_clear(msg_id);
        		break;
        	case MSG_DISPLAY_TASK_MD_ICON_SHOW:
        		ap_display_icon_sts_clear(ICON_MD_STS_0 | (ICON_MD_STS_1<<8) | (ICON_MD_STS_2<<16));
        		ap_display_icon_sts_clear(ICON_MD_STS_3 | (ICON_MD_STS_4<<8) | (ICON_MD_STS_5<<16));
        		ap_display_icon_sts_set(msg_id);
        		break;
        	case MSG_DISPLAY_TASK_ICON_MOVE:
        		ap_display_icon_move((DISPLAY_ICONMOVE *) (msg_id & 0xFFFFFF));
        		break;
        	case MSG_DISPLAY_TASK_PIC_EFFECT:
        		ap_display_effect_sts_set(DISPALY_PIC_EFFECT);
        		break;
        	case MSG_DISPLAY_TASK_PIC_PREVIEW_EFFECT:
        		ap_display_effect_sts_set(DISPALY_PIC_PREVIEW_EFFECT);
        		break;
        	case MSG_DISPLAY_TASK_PIC_EFFECT_END:
        		ap_display_video_preview_end();
        		break;
         	case MSG_DISPLAY_TASK_LEFT_REC_TIME_DRAW:
        		ap_display_left_rec_time_draw((INT32U) (msg_id & 0xFFFFFF), 1);
        		break;
         	case MSG_DISPLAY_TASK_LEFT_REC_TIME_CLEAR:
        		ap_display_left_rec_time_draw(0, 0);
        		break;         		        	
        	case MSG_DISPLAY_TASK_STRING_DRAW:
        		ap_display_string_draw((STR_ICON *) (msg_id & 0xFFFFFF));
        		break;
        	case MSG_DISPLAY_TASK_MP3_INDEX_SHOW:
        		ap_display_mp3_index_sts_set(DISPALY_MP3_INDEX_SHOW);
        		display_mp3_play_index = msg_id & 0xFFFFFF;
        		break;
        	case MSG_DISPLAY_TASK_MP3_TOTAL_INDEX_SHOW:
        		ap_display_mp3_index_sts_set(DISPALY_MP3_TOTAL_INDEX_SHOW);
        		display_mp3_total_index = msg_id & 0xFFFFFF;
        		break;
        	case MSG_DISPLAY_TASK_MP3_VOLUME_SHOW:
        		ap_display_mp3_index_sts_set(DISPALY_MP3_VOLUME_SHOW);
        		display_mp3_volume = msg_id & 0xFFFFFF;
        		break;
        	case MSG_DISPLAY_TASK_MP3_FM_CHANNEL_SHOW:
        		ap_display_mp3_index_sts_set(DISPALY_MP3_FM_CHANNEL_SHOW);
        		display_mp3_FM_channel = msg_id & 0xFFFFFF;
        		break;        		        	
        	case MSG_DISPLAY_TASK_MP3_ALL_INDEX_CLEAR:
        		ap_display_mp3_index_sts_set(DISPALY_MP3_INDEX_NONE_EFFECT);
        		break;        		        		        		        		
        	case MSG_DISPLAY_TASK_MP3_INPUT_NUM_SHOW:
        		ap_display_mp3_input_num_sts_set(DISPALY_MP3_INPUT_NUM_SHOW);
        		display_mp3_input_num = msg_id & 0xFFFFFF;
        		break;        		        	
        	case MSG_DISPLAY_TASK_MP3_INPUT_NUM_CLEAR:
        		ap_display_mp3_input_num_sts_set(DISPALY_MP3_INPUT_NUM_NONE_EFFECT);
        		break;         	
        	default:
        		ap_display_buff_copy_and_draw(msg_id, DISPALY_BUFF_SRC_SENSOR);
        		break;
		}
	}
}
