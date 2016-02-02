#include "ztkconfigs.h"
#include "task_state_handling.h"

/* for debug */
#define DEBUG_TASK_STATE_HANDLING	1
#if DEBUG_TASK_STATE_HANDLING
    #include "gplib.h"
    #define _dmsg(x)			print_string x
#else
    #define _dmsg(x)
#endif

/* definitions */
#define STATE_HANDLING_QUEUE_MAX	5
#define AP_QUEUE_MAX			1024

MSG_Q_ID ApQ;
OS_EVENT *StateHandlingQ;
ALIGN16 INT8U ApQ_para[AP_QUEUE_MSG_MAX_LEN];  // dominant modify, APQ Parameter must alignment to at least 4 Byte
void *state_handling_q_stack[STATE_HANDLING_QUEUE_MAX];

//	prototypes
void state_handling_init(void);
void state_handling_apq_flush(void);

void state_handling_init(void)
{
	INT32S config_load_flag;
	
	StateHandlingQ = OSQCreate(state_handling_q_stack, STATE_HANDLING_QUEUE_MAX);
	ApQ = msgQCreate(AP_QUEUE_MAX, AP_QUEUE_MAX, AP_QUEUE_MSG_MAX_LEN);
	nvmemory_init();
	
//	OSTimeDly(1);
	config_load_flag = ap_state_config_load();
	ap_state_resource_init();
	ap_state_config_initial(config_load_flag);
//	ap_state_config_initial(STATUS_FAIL);
	ap_state_handling_calendar_init();
	ap_state_ir_key_init();
}

void state_handling_entry(void *para)
{
	INT32U msg_id, prev_state;
	INT8U err;

	msg_id = 0;
	state_handling_init();
	OSQPost(StateHandlingQ, (void *) STATE_STARTUP);
	while (1) {
		state_handling_apq_flush();
		prev_state = msg_id & 0xFFFF;
		msg_id = (INT32U) OSQPend(StateHandlingQ, 0, &err);
		if((!msg_id) || (err != OS_NO_ERR)) {
        		continue;
		}
		present_state = msg_id & 0x1FF;
		switch(msg_id & 0x1FF) {
			case STATE_STARTUP:
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_STATE_ID | (present_state<<16), NULL, NULL, MSG_PRI_NORMAL);
				state_startup_entry((void *) &prev_state);
				break;
			case STATE_VIDEO_PREVIEW:
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_STATE_ID | (present_state<<16), NULL, NULL, MSG_PRI_NORMAL);
				state_video_preview_entry((void *) &prev_state);
				break;
			case STATE_VIDEO_RECORD:
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_STATE_ID | (present_state<<16), NULL, NULL, MSG_PRI_NORMAL);
				state_video_record_entry((void *) &prev_state);
				break;
#if 0//C_MOTION_DETECTION == CUSTOM_ON
			case STATE_MOTION_DETECTION:
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_STATE_ID | (present_state<<16), NULL, NULL, MSG_PRI_NORMAL);
				state_motion_detect_entry((void *) &prev_state);
				break;
#endif

#if (defined AUD_RECORD_EN) && (AUD_RECORD_EN==1)
			case STATE_AUDIO_RECORD:
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_STATE_ID | (present_state<<16), NULL, NULL, MSG_PRI_NORMAL);
				state_audio_record_entry((void *) &prev_state);
				break;
#endif

			case STATE_BROWSE:
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_STATE_ID | (present_state<<16), NULL, NULL, MSG_PRI_NORMAL);
				state_browse_entry((void *) &prev_state, (msg_id & 0xFFFF0000)>>16 );
				break;
			case STATE_THUMBNAIL:
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_STATE_ID | (present_state<<16), NULL, NULL, MSG_PRI_NORMAL);
				state_thumbnail_entry((void *) &prev_state);
				break;
			case STATE_SETTING:
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_STATE_ID | (present_state<<16), NULL, NULL, MSG_PRI_NORMAL);
				state_setting_entry((void *) &prev_state);
				break;
			default:
				break;
		}
	}
}

void state_handling_apq_flush(void)
{
	ap_state_handling_str_draw_exit();
}