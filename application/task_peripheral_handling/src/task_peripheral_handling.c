#include "task_peripheral_handling.h"

MSG_Q_ID PeripheralTaskQ;
void *peripheral_task_q_stack[PERIPHERAL_TASK_QUEUE_MAX];
static __align(4) INT8U peripheral_para[PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN];
INT8U screen_saver_enable = 0;
static INT8U tick;
//	prototypes
void task_peripheral_handling_init(void);

void task_peripheral_handling_init(void)
{
	PeripheralTaskQ = msgQCreate(PERIPHERAL_TASK_QUEUE_MAX, PERIPHERAL_TASK_QUEUE_MAX, PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN);
	ap_peripheral_init();
	turnkey_irkey_resource_register((void*)msgQSend,PeripheralTaskQ,MSG_PERIPHERAL_TASK_IR_KEY);
}

void task_peripheral_handling_entry(void *para)
{
	INT32U msg_id;
	st_key_para *sys_key_para;
	
	task_peripheral_handling_init();
	
	while(1) {
		
		if(msgQReceive(PeripheralTaskQ, &msg_id, peripheral_para, PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN) == STATUS_FAIL) {
			continue;
		}
	
        switch (msg_id & 0xFFFF) {
        	case MSG_PERIPHERAL_TASK_KEY_DETECT:
        		ap_peripheral_key_judge();
        		ap_peripheral_adaptor_out_judge();
        		ap_peripheral_config_store();
        		break;
        	case MSG_PERIPHERAL_TASK_KEY_REGISTER:
        		ap_peripheral_key_register(peripheral_para[0]);
        		break;
        	case MSG_PERIPHERAL_TASK_LED_SET:
        		ap_peripheral_led_set(peripheral_para[0]);
        		break;
			case MSG_PERIPHERAL_TASK_LED_FLASH_SET:	//wwj add
				ap_peripheral_led_flash_set();
				break;
			case MSG_PERIPHERAL_TASK_LED_BLINK_SET:	//wwj add
				ap_peripheral_led_blink_set();
				break;

        	case MSG_PERIPHERAL_TASK_ZOOM_FLAG:
        		ap_peripheral_zoom_key_flag_set(peripheral_para[0]);
        		break;
#if USE_ADKEY_NO        	
        	case MSG_PERIPHERAL_TASK_AD_DETECT_CHECK:
        		ap_peripheral_ad_key_judge();
	#if C_BATTERY_DETECT == CUSTOM_ON        		
        		ap_peripheral_battery_check_calculate();
	#endif        		     		
				ap_TFT_backlight_tmr_check();	//wwj add
				//temporary
        		tick++;
        		if (tick > 22) {
        			msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_TV_DETECT, NULL, NULL, MSG_PRI_NORMAL);
    #if LIGHT_DETECT_USE > LIGHT_DETECT_USE_RESISTOR    			
        			msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LIGHT_DETECT, NULL, NULL, MSG_PRI_NORMAL);
    #endif
        			tick = 0;
        		}        		     		
        		break;
#endif
			case MSG_PERIPHERAL_TASK_TV_DETECT:
				ap_peripheral_tv_detect();
				break;
#if LIGHT_DETECT_USE > LIGHT_DETECT_USE_RESISTOR    			
			case MSG_PERIPHERAL_TASK_LIGHT_DETECT:
				ap_peripheral_light_detect();
				break;
#endif					
#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
			case MSG_PERIPHERAL_TASK_BAT_STS_REQ:
				ap_peripheral_battery_sts_send();
				break;
#endif   		
#if C_MOTION_DETECTION == CUSTOM_ON
        	case MSG_PERIPHERAL_TASK_MOTION_DETECT_JUDGE:
        		ap_peripheral_motion_detect_judge();
        		break;
        	case MSG_PERIPHERAL_TASK_MOTION_DETECT_START:
        		ap_peripheral_motion_detect_start();
        		break;
        	case MSG_PERIPHERAL_TASK_MOTION_DETECT_STOP:
        		ap_peripheral_motion_detect_stop();        	
        		break;
#endif
#if C_SCREEN_SAVER == CUSTOM_ON
			case MSG_PERIPHERAL_TASK_LCD_BACKLIGHT_SET:
        		ap_peripheral_lcd_backlight_set(peripheral_para[0]);        	
        		break;
			case MSG_PERIPHERAL_TASK_SCREEN_SAVER_ENABLE:
        		screen_saver_enable = 1;
        		break;        		
#endif      		

			case MSG_PERIPHERAL_TASK_NIGHT_MODE_SET:	//wwj add
        		ap_peripheral_night_mode_set(peripheral_para[0]);
				break;

        	/*case MSG_PERIPHERAL_TASK_TEST:
        		test_flag ^= 1;
        		gpio_write_io(IO_F12, test_flag);
        		break;*/
        	case MSG_PERIPHERAL_TASK_IR_KEY:
            	sys_key_para = (st_key_para *) peripheral_para;
	            ap_peripheral_irkey_handler_exe(sys_key_para->key_code, sys_key_para->key_type);
            	break;
			case MSG_PERIPHERAL_TASK_STATE_ID:
        		present_state_ID = (msg_id>>16) & 0x1FF;
				if( (present_state_ID == STATE_VIDEO_PREVIEW) || (present_state_ID == STATE_VIDEO_RECORD) ){
					ap_peripheral_irkey_clean();
				}        		
        		break;  
        	case MSG_PERIPHERAL_USBD_EXIT:
        		usbd_exit = 1;       	
        		break;    	
        	default:
        		break;
        }
        
    }
}