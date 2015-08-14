#include "ap_peripheral_handling.h"

#define ENABLE				1
#define DISABLE				0

/*
 * for debug
 */
#define DEBUG_NONE			0x0000
#define DEBUG_MESSAGE			0x0001
#define PRINT_ADKEY_VAL			0x0002	// print ad key value
#define PRINT_BAT_VAL			0x0004	// print battery value

#define DEBUG_OPTION		(	\
	DEBUG_MESSAGE		|	\
/*	PRINT_ADKEY_VAL		|*/	\
/*	PRINT_BAT_VAL		|*/	\
	DEBUG_NONE)

#if (DEBUG_OPTION & DEBUG_MESSAGE)
    #define _msg(x)		(x)
#else
    #define _msg(x)
#endif

#if (DEBUG_OPTION & PRINT_ADKEY_VAL)
    #define _adkey(x)		(x)
#else
    #define _adkey(x)
#endif

#if (DEBUG_OPTION & PRINT_BAT_VAL)
    #define _bat(x)		(x)
#else
    #define _bat(x)
#endif


#if C_MOTION_DETECTION == CUSTOM_ON
	static INT32U md_work_memory_addr;
	static INT32S motion_detect_cnt;
	static INT8U md_cnt;
#endif
#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
	static INT32U battery_value_sum = 0;
	static INT8U battery_lvl = 3;
	static INT8U bat_ck_cnt = 0;

	static INT8U ad_base_ck_cnt = 0;
	static INT32U ad_base_ck_sum = 0;

	static INT8U ad_18_ck_cnt = 0;
	static INT32U ad_18_value_sum = 0;
	static INT32U ad_18_value_sum_bak = 0;
	static INT16U ad_value_fifo[32] = {0};
#endif
#if USE_ADKEY_NO
	static INT8U ad_detect_timerid;
	static INT16U ad_value=1024;
	static INT16U ad_18_value;
//	INT16U ad_value_bak;		//wwj test
//	INT16U ad_18_value_bak;		//wwj test
	static KEYSTATUS ad_key_map[USE_ADKEY_NO];
	static INT8U  ad_line_select = 0;
	static INT32U ad_time_stamp;
#endif
#if C_SCREEN_SAVER == CUSTOM_ON
	static INT32U key_active_cnt;
	static INT8U lcd_bl_sts;
#endif
static INT8U key_detect_timerid;
static INT8U zoom_key_flag;
static KEYSTATUS key_map[USE_IOKEY_NO];

//wwj add
#define LED_STATUS_FLASH	1
#define LED_STATUS_BLINK	2

static INT8U led_status;	//0: nothing  1: flash	2: blink
static INT8U led_cnt;

#if C_SCREEN_SAVER == CUSTOM_ON
    INT8U auto_off_force_disable = 0;	//wwj add
#endif

void ap_peripheral_auto_off_force_disable_set(INT8U);	//wwj add
//wwj add end

static INT16U adp_out_cnt;
static INT16U usbd_cnt;
#if USB_PHY_SUSPEND == 1
static INT16U phy_cnt;
#endif
static INT16U adp_cnt;
static INT8U  adp_status;
static INT16U low_voltage_cnt;
static INT16U config_cnt;
INT8U  usbd_exit;
extern INT8U s_usbd_pin;

#define C_MAX_IR_KEY_NUM		32
INT8U ir_keycode[C_MAX_IR_KEY_NUM];
INT32U ir_message[C_MAX_IR_KEY_NUM];
INT16U present_state_ID;

#define    KEY_BOUNDARY    66000//52000
#define    RIPPLE_ADJ      8 //25mv

INT32U b1=0, b2=0, b3=0, b4=0, b5=0, b6=0, b7=0;
static INT8U tv= 1;

#if LIGHT_DETECT_USE == LIGHT_DETECT_USE_SENSOR
static INT8U csi_light;
#endif

//	prototypes
void ap_peripheral_key_init(void);
void ap_peripheral_function_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_next_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_prev_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_ok_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_usbd_plug_out_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_pw_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_menu_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_null_key_exe(INT16U *tick_cnt_ptr);
#if C_MOTION_DETECTION == CUSTOM_ON
	void ap_peripheral_motion_detect_isr(void);
#endif
#if USE_ADKEY_NO
	void ap_peripheral_ad_detect_init(INT8U adc_channel, void (*bat_detect_isr)(INT16U data));
	void ap_peripheral_ad_check_isr(INT16U value);
#endif
void ap_peripheral_irkey_clean(void);

void ap_peripheral_init(void)
{
	key_detect_timerid = 0xFF;

	//LED IO init
  	gpio_init_io(LED, GPIO_OUTPUT);
  	gpio_set_port_attribute(LED, ATTRIBUTE_HIGH);
  	//gpio_write_io(LED, DATA_HIGH);
  	gpio_write_io(LED, DATA_LOW);	//wwj modify
  	led_status = 0;		//wwj add
  	led_cnt = 0;		//wwj add

	// camera switch initial
	camswitch_init();

  	/* adpator detect pin */
  	gpio_init_io(ADP_OUT_PIN, GPIO_INPUT);
  	gpio_set_port_attribute(ADP_OUT_PIN, ATTRIBUTE_HIGH);
  	gpio_write_io(ADP_OUT_PIN, DATA_LOW);
	
#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
	//AV_IN_DET IO init
  	gpio_init_io(AV_IN_DET, GPIO_INPUT);
  	gpio_set_port_attribute(AV_IN_DET, ATTRIBUTE_LOW);
  	gpio_write_io(AV_IN_DET, DATA_HIGH);
#endif

#if (LIGHT_DETECT_USE == LIGHT_DETECT_USE_GPIO) && (LIGHT_DETECT == IO_H1)
	R_FUNPOS1 |= (1<<5);	//use sdram_cke as gpio
	gpio_init_io(LIGHT_DETECT, GPIO_INPUT);
  	gpio_set_port_attribute(LIGHT_DETECT, 0);
  	gpio_write_io(LIGHT_DETECT, 1);
#endif
	/*	//wwj modify
	gpio_init_io(IR_CTRL,GPIO_INPUT);
  	gpio_set_port_attribute(IR_CTRL, 1);
  	gpio_write_io(IR_CTRL, 0); */
	gpio_init_io(IR_CTRL,GPIO_OUTPUT);
  	gpio_set_port_attribute(IR_CTRL, ATTRIBUTE_HIGH);
  	gpio_write_io(IR_CTRL, 0);

	ap_peripheral_key_init();

#if C_MOTION_DETECTION == CUSTOM_ON
	md_work_memory_addr = (INT32U ) gp_malloc(1200);
	if (!md_work_memory_addr) {
		DBG_PRINT("Motion detcetion working memory allocate fail\r\n");
	} else {
		Sensor_MotionDection_Inital(md_work_memory_addr);
		motion_detect_isr_register(ap_peripheral_motion_detect_isr);
	}
#endif
#if USE_ADKEY_NO
	ad_detect_timerid = 0xFF;
	ap_peripheral_ad_detect_init(AD_DETECT_PIN, ap_peripheral_ad_check_isr);

#endif
	config_cnt = 0;
}


/* #BEGIN# add by xyz - 2014.11.14 */
extern void	state_video_record_init(INT32U prev_state);
extern void	state_video_record_exit(void);
extern void	state_setting_exit(void);
extern void	ap_state_config_pic_size_set(INT8U pic_size);
extern void	ap_state_config_store(void);
extern void	ap_peripheral_irkey_clean(void);

/*
 *
 *
 */
#define MSG_DMSG	1
#define MSG_CAMSM	0
#if (MSG_DMSG)
   #define dmsg(x)	DBG_PRINT x
#else
   #define dmsg(x)
#endif
#if (MSG_CAMSM)
   #define camsg(x)	{ DBG_PRINT("[%s]: ", (camsw[i].io == WATCH_CAM0) ? "cam0" : "cam1");	DBG_PRINT x ; }
#else
   #define camsg(x)
#endif

typedef struct {
	INT32U	io;
	INT32U	bounce;
	INT32U	repeat;
	INT32U	status;		// 0=low level, 1=high level, 2=rising edge, 3=falling edge
	INT32U	new_state;
	INT32U	old_state;
} camswitch_t;

static camswitch_t	camsw[2];
static INT32U		camsw_record;


/*
 *
 *
 */
void camswitch_send_mode(INT8U mode)
{
	if (mode != camsw_record) {
		switch (mode) {
		case 0:		dmsg(("\033[1;32mcamsw\033[0m - 0 & 1\r\n"));	break;
		case 1:		dmsg(("\033[1;32mcamsw\033[0m - 0\r\n"));	break;
		case 2:		dmsg(("\033[1;32mcamsw\033[0m - 1\r\n"));	break;
		default:	dmsg(("\033[1;32mcamsw\033[0m - unknown\r\n"));	break;
		}
		camsw_record = mode;
		msgQSend(ApQ, MSG_APQ_CAMSWITCH_ACTIVE, &mode, sizeof(INT8U), MSG_PRI_NORMAL);
	}
}

void camswitch_set_mode(INT8U mode)
{
	dmsg(("\033[1;32m%s()\033[0m - mode=%02x\r\n", __func__, mode));

	state_video_record_exit();

	ap_state_config_pic_size_set(mode);
	ap_state_config_store();
	state_setting_exit();

	ap_peripheral_irkey_clean();
	state_video_record_init(STATE_SETTING);
}

void camswitch_judge(void)
{
	INT32U	i, state;


	for (i=0; i<2; i++) {
		state = gpio_read_io(camsw[i].io);
		if (state != camsw[i].new_state) {
			camsg(("init bounce\r\n"));
			camsw[i].bounce    = 20;
			camsw[i].new_state = state;
		} else {
			if (camsw[i].bounce != 0) {
				camsg(("debounce...\r\n"));
				camsw[i].bounce--;
			} else {
				if (camsw[i].new_state != camsw[i].old_state) {
					if (camsw[i].new_state) {
						camsg(("rising edge\r\n"));
						camsw[i].status = 2;
					} else {
						camsg(("falling edge\r\n"));
						camsw[i].status = 3;
					}
					camsw[i].repeat = 0;
				} else {
					if (camsw[i].repeat) {
						camsw[i].repeat--;
					} else {
						if (camsw[i].new_state) {
							camsg(("high level\r\n"));
							camsw[i].status = 1;
						} else {
							camsg(("low level\r\n"));
							camsw[i].status = 0;
						}
						camsw[i].repeat = 40;
					}
				}
				camsw[i].old_state = camsw[i].new_state;
			}
		}
	}

	if (camsw[0].status == camsw[1].status) {
		camswitch_send_mode(0);
	} else {
		if (!camsw[0].status) {
			camswitch_send_mode(1);
		}
		if (!camsw[1].status) {
			camswitch_send_mode(2);
		}
	}
}

void camswitch_init(void)
{
	int	i;

	camsw[0].io = WATCH_CAM0;
	camsw[1].io = WATCH_CAM1;
	for (i=0; i<2; i++) {
		camsw[i].bounce    = 0;
		camsw[i].repeat    = 0;
		camsw[i].status    = 0;
		camsw[i].new_state = 0;
		camsw[i].old_state = 0;

		// input with pull high
		gpio_init_io(camsw[i].io,            GPIO_INPUT);
		gpio_set_port_attribute(camsw[i].io, INPUT_WITH_RESISTOR);
		gpio_write_io(camsw[i].io,           DATA_HIGH);
	}

	camsw_record = 0;
	dmsg(("%s()\r\n", __func__));
}
/* #END# add by xyz - 2014.11.14 */


#if C_MOTION_DETECTION == CUSTOM_ON
void ap_peripheral_motion_detect_isr(void)
{
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_JUDGE, NULL, NULL, MSG_PRI_NORMAL);
}

void ap_peripheral_motion_detect_judge(void)
{
	INT32U i, *ptr = (INT32U*) (0x80000000 | md_work_memory_addr), hit, value, threshold;
	INT32S jj;

	jj = motion_detect_cnt;
	hit = 0;
	ptr+=2;
	value = ap_state_config_motion_detect_get();
	if (value == 0) {
		threshold = 70;
	} else if (value == 1) {
		threshold = 150;
	} else {
		threshold = 300;
	}
	for (i=2 ; i<298 ; i++) {
		if (*ptr & 0xF) {
			hit += 4;
		}
		ptr++;
	}
	if (hit > threshold) {
		motion_detect_cnt++;
	} else if (hit == 0) {
		motion_detect_cnt = 0;
	} else {
		motion_detect_cnt--;
		if (motion_detect_cnt < 0) {
			motion_detect_cnt = 0;
		}
	}

	if (motion_detect_cnt > 5) {
		msgQSend(ApQ, MSG_APQ_MOTION_DETECT_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
		motion_detect_cnt = 5;
	}

	if (jj != motion_detect_cnt) {
		md_cnt++;
		if ((motion_detect_cnt == 5) || (motion_detect_cnt == 0) ) {
			md_cnt = 0;
			msgQSend(ApQ, MSG_APQ_MOTION_DETECT_ICON_UPDATE, &motion_detect_cnt, sizeof(INT8U), MSG_PRI_NORMAL);
		}
		else if (md_cnt >= 2) {
			md_cnt = 0;
			msgQSend(ApQ, MSG_APQ_MOTION_DETECT_ICON_UPDATE, &motion_detect_cnt, sizeof(INT8U), MSG_PRI_NORMAL);
		}
	}
}

void ap_peripheral_motion_detect_start(void)
{
	Sensor_MotionDection_start();
	msgQSend(ApQ, MSG_APQ_MOTION_DETECT_ICON_UPDATE, &motion_detect_cnt, sizeof(INT8U), MSG_PRI_NORMAL);
}

void ap_peripheral_motion_detect_stop(void)
{
	Sensor_MotionDection_stop();
}
#endif


void ap_peripheral_led_set(INT8U type)
{
	gpio_write_io(LED, type);
	led_status = 0;		//wwj add
	led_cnt = 0;		//wwj add
}

void ap_peripheral_led_flash_set(void)	//wwj addd
{
	gpio_write_io(LED, DATA_HIGH);
	led_status = LED_STATUS_FLASH;
	led_cnt = 0;
}

void ap_peripheral_led_blink_set(void)	//wwj add
{
	gpio_write_io(LED, DATA_HIGH);
	led_status = LED_STATUS_BLINK;
	led_cnt = 0;
}

void ap_peripheral_zoom_key_flag_set(INT8U flag)
{
	zoom_key_flag = flag;
}

#if USE_ADKEY_NO
void ap_peripheral_ad_detect_init(INT8U adc_channel, void (*ad_detect_isr)(INT16U data))
{
#if C_BATTERY_DETECT == CUSTOM_ON
//	battery_lvl = 3;
#endif
	switch (adc_channel) {
		case ADC_LINE_0:
			gpio_init_io(IO_F6, GPIO_INPUT);
			gpio_set_port_attribute(IO_F6, ATTRIBUTE_HIGH);
			break;

		case ADC_LINE_1:
			gpio_init_io(IO_F7, GPIO_INPUT);
			gpio_set_port_attribute(IO_F7, ATTRIBUTE_HIGH);
			break;

		case ADC_LINE_2:
			gpio_init_io(IO_F8, GPIO_INPUT);
			gpio_set_port_attribute(IO_F8, ATTRIBUTE_HIGH);
			break;

		case ADC_LINE_3:
			gpio_init_io(IO_F9, GPIO_INPUT);
			gpio_set_port_attribute(IO_F9, ATTRIBUTE_HIGH);
			break;
	}
	adc_init();
	adc_manual_ch_set(adc_channel);
	adc_manual_callback_set(ad_detect_isr);
//	adc_manual_sample_start();
	if (ad_detect_timerid == 0xFF) {
		ad_detect_timerid = AD_DETECT_TIMER_ID;
		sys_set_timer((void*)msgQSend, (void*) PeripheralTaskQ, MSG_PERIPHERAL_TASK_AD_DETECT_CHECK, ad_detect_timerid, PERI_TIME_INTERVAL_AD_DETECT);
	}
#if C_BATTERY_DETECT == CUSTOM_ON
//	msgQSend(ApQ, MSG_APQ_BATTERY_LVL_SHOW, &battery_lvl, sizeof(INT8U), MSG_PRI_NORMAL);
#endif
}

void ap_peripheral_ad_check_isr(INT16U value)
{
	INT32U ad_reg;

	ad_reg = R_ADC_MADC_CTRL & 0x7;
	if (ad_reg == AD_DETECT_PIN) {
		ad_value = value;
		//adc_manual_ch_set(ADC_LINE_5);
		//adc_manual_sample_start();
	} else {
		ad_18_value = value;
		//adc_manual_ch_set(AD_DETECT_PIN);
	}
}

void ap_peripheral_ad_key_judge(void)
{
	INT8U i, adkey_lvl;
	INT32U  ad_valid;
	INT32U  t;

	t = OSTimeGet();
	if ((t - ad_time_stamp) < 2) {
		return ;
	}
	ad_time_stamp = t;

	adkey_lvl = 0xFF;
/*
	ad_line_select++;
	if (ad_line_select & 0xF) {
		adc_manual_ch_set(AD_DETECT_PIN);
	} else {
		adc_manual_ch_set(ADC_LINE_5);
		ad_line_select = 0;
	}
*/
	adc_manual_ch_set(AD_DETECT_PIN);
	adc_manual_sample_start();


	ad_valid = ad_value>>6;

	if(ad_valid<C_AD_VALUE_1)
		adkey_lvl = ADKEY_LVL_1;
	else if(ad_valid<C_AD_VALUE_2)
		adkey_lvl = ADKEY_LVL_2;
	else if(ad_valid<C_AD_VALUE_3)
		adkey_lvl = ADKEY_LVL_3;
	else if(ad_valid<C_AD_VALUE_4)
		adkey_lvl = ADKEY_LVL_4;
	else if(ad_valid<C_AD_VALUE_5)
		adkey_lvl = ADKEY_LVL_5;
	
	
	if (adkey_lvl != 0xFF) {
		#if 0
		for (i=0 ; i<USE_ADKEY_NO ; i++) {
			if (i == adkey_lvl) {
				continue;
			}
			ad_key_map[i].key_cnt = 0;
		}
		#endif
		ad_key_map[adkey_lvl].key_cnt += 1;
		/*
		if (zoom_key_flag && ad_key_map[adkey_lvl].key_cnt>2 && (adkey_lvl == PREVIOUS_KEY || adkey_lvl == NEXT_KEY)) {
			ad_key_map[adkey_lvl].key_function(&(ad_key_map[adkey_lvl].key_cnt));
		} */
	} else {
		for (i=0 ; i<USE_ADKEY_NO ; i++) {
			if (ad_key_map[i].key_cnt > 2) {
#if C_SCREEN_SAVER == CUSTOM_ON
				key_active_cnt = 0;
				ap_peripheral_lcd_backlight_set(BL_ON);
#endif
				ad_key_map[i].key_function(&(ad_key_map[i].key_cnt));
			}
			ad_key_map[i].key_cnt = 0;
		}
	}
}
#endif

#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
INT32U previous_direction = 0;
void ap_peripheral_battery_check_calculate(void)
{
	//INT32U vol_gap;
	INT8U bat_lvl_cal;
	INT32U direction;
	INT32U  ad_valid,ad_tmp;
	
	if (adp_status == 0) {
		return;
	}
	else if (adp_status == 1) {
		direction = 1; //low voltage to high voltage
		if(previous_direction != direction){
			msgQSend(ApQ, MSG_APQ_BATTERY_CHARGED_SHOW, NULL, NULL, MSG_PRI_NORMAL);
		}
		previous_direction = direction;
	}
	else {
		direction = 0; //high voltage to low voltage
		if(previous_direction != direction){
			msgQSend(ApQ, MSG_APQ_BATTERY_CHARGED_CLEAR, NULL, NULL, MSG_PRI_NORMAL);
		}
		previous_direction = direction;
	}

	if (ad_line_select == 1) {
		// Only ADC line5 sampling data is updated, not AD Key detection line
		ad_18_value_sum += (ad_18_value&0xFFC0);
		ad_18_ck_cnt++;
		if(ad_18_ck_cnt >= 24) {
			ad_18_value_sum_bak = ad_18_value_sum / 24;
			ad_18_value_sum = 0;
			ad_18_ck_cnt = 0;

			if(ad_18_value_sum_bak == 0) {
				low_voltage_cnt = 0;
			} else if(battery_lvl == 0 && direction == 0 && (ad_18_value_sum_bak > 36900)) {
				low_voltage_cnt++;
				if (low_voltage_cnt > 2) {
					low_voltage_cnt = 25;
					ap_peripheral_pw_key_exe(&low_voltage_cnt);
				}
			} else {
				low_voltage_cnt = 0;
			}
		}
		return;
	} else {
		ad_valid = ad_value>>6;
		if (ad_valid < 135) {
			ad_base_ck_sum += (ad_value&0xFFC0);
			ad_base_ck_cnt++;
			battery_value_sum += (ad_value&0xFFC0);
			//ad_18_value_sum += (ad_18_value&0xFFC0);
			bat_ck_cnt++;
			if (bat_ck_cnt > /*0x3F*/0x7f) { //wwj modify
				//vol_gap = (BATTERY_FULL_LEVEL - BATTERY_LOW_LEVEL) >> 2;
				battery_value_sum >>= 7;//6;
				//ad_18_value_sum >>= 7;//6;

				/*
				ad_value_bak = ad_18_value_sum/10000;	//wwj test
				ad_18_value_bak = ad_18_value_sum - (ad_18_value_sum/10000)*10000;	//wwj test

				ad_value_bak = battery_value_sum >> 7;	//wwj test
				ad_18_value_bak = ad_18_value_sum >> 7;	//wwj test
				*/

				//DBG_PRINT("AD ave[%d]\r\n", battery_value_sum);
				//DBG_PRINT("AD_18 ave[%d]\r\n", ad_18_value_sum);
				if ((ad_18_value_sum_bak < 36400) || (ad_18_value_sum_bak == 0)) {
					if (battery_value_sum > 7270) {
						bat_lvl_cal = 3;
					} else if ((battery_value_sum < 7270) && (battery_value_sum > 6900)) {
						bat_lvl_cal = 2;
					} else if (battery_value_sum < 6900) {
						bat_lvl_cal = 1;
					} else if(battery_value_sum < 6670) {
						bat_lvl_cal = battery_lvl;
					}	
				} else if (ad_18_value_sum_bak < 36900) {
					bat_lvl_cal = 1;
				} else {
					bat_lvl_cal = 0;
				}

				msgQSend(ApQ, MSG_APQ_BATTERY_LVL_SHOW, &bat_lvl_cal, sizeof(INT8U), MSG_PRI_NORMAL);
				battery_lvl = bat_lvl_cal;
					
				/*
				if(bat_lvl_cal == 0 && direction == 0 && (ad_18_value_sum_bak > 36900)) {
					low_voltage_cnt++;
					if (low_voltage_cnt > 3) {	//wwj modify
						low_voltage_cnt = 25;	//wwj add
						ap_peripheral_pw_key_exe(&low_voltage_cnt);
					}
				} else {
					low_voltage_cnt = 0;
				} */
				
				/*DBG_PRINT("AD[%d]\r\n", battery_value_sum);
				DBG_PRINT("AD_18[%d]\r\n", ad_18_value_sum);
				bat_lvl_cal = (battery_value_sum - BATTERY_LOW_LEVEL)/vol_gap;
				if (bat_lvl_cal <= battery_lvl) {
					msgQSend(ApQ, MSG_APQ_BATTERY_LVL_SHOW, &bat_lvl_cal, sizeof(INT8U), MSG_PRI_NORMAL);
					//DBG_PRINT("PH[%d]\r\n", bat_lvl_cal);
					battery_lvl = bat_lvl_cal;
				}
				DBG_PRINT("PH[%d]\r\n", battery_lvl);*/

				bat_ck_cnt = 0;
				battery_value_sum = 0;
				//ad_18_value_sum = 0;
			}
			
			if(ad_base_ck_cnt >= 32) {
				ad_base_ck_cnt = 0;
	//			ad_valid = battery_value_sum >> 6;
				ad_valid = ad_base_ck_sum >> 11;
				ad_valid += RIPPLE_ADJ;

				//ad_value_bak = ad_valid/10000;	//wwj test
				//ad_18_value_bak = ad_valid - (ad_valid/10000)*10000;	//wwj test

			/*
				if (adp_status == 1) {
					ad_valid = 134; //0.44v
				}	
			*/
				//DBG_PRINT("pre ad valid = %d\r\n",ad_valid);

				ad_tmp = ad_valid * 8529;
				if (ad_tmp > 1023000) {
					b1 = 1023000 - KEY_BOUNDARY;
				}
				else {
					b1 = ad_tmp - KEY_BOUNDARY;
				}
				b2 = (ad_valid * 5500) - KEY_BOUNDARY;
				b3 = (ad_valid * 5500) + KEY_BOUNDARY;
				b4 = (ad_valid * 3440) - KEY_BOUNDARY;
				b5 = (ad_valid * 3440) + KEY_BOUNDARY;
				b6 = (ad_valid * 1967) - KEY_BOUNDARY;
				b7 = (ad_valid * 1967) + KEY_BOUNDARY;


				ad_base_ck_cnt = 0;
				ad_base_ck_sum = 0;
			}
		} else {
				//wwj add
				bat_ck_cnt = 0;
				battery_value_sum = 0;
				//ad_18_value_sum = 0;
				ad_base_ck_cnt = 0;
				ad_base_ck_sum = 0;
				/*
				if((b1==0) && (b2==0) && (b3==0) && (b4==0) && (b5==0) && (b6==0) && (b7==0)) { //wwj add
					ad_valid = 120;	//default value
					ad_tmp = ad_valid * 8529;
					if (ad_tmp > 1023000) {
						b1 = 1023000 - KEY_BOUNDARY;
					}
					else {
						b1 = ad_tmp - KEY_BOUNDARY;
					}
					b2 = (ad_valid * 5500) - KEY_BOUNDARY;
					b3 = (ad_valid * 5500) + KEY_BOUNDARY;
					b4 = (ad_valid * 3440) - KEY_BOUNDARY;
					b5 = (ad_valid * 3440) + KEY_BOUNDARY;
					b6 = (ad_valid * 1967) - KEY_BOUNDARY;
					b7 = (ad_valid * 1967) + KEY_BOUNDARY;
				} */
				//wwj add end
			
			//DBG_PRINT("[Battery Detect Nothing]\r\n");
		}
	}
}

/*		// For measure "BATTERY_FULL_LEVEL" and "BATTERY_LOW_LEVEL".
void ap_peripheral_battery_check_calculate(void)
{
	adc_manual_sample_start();
	battery_value_sum += battery_value;
	bat_ck_cnt++;
	if (bat_ck_cnt > 0x1F) {
		battery_value_sum >>= 5;
		DBG_PRINT("[%d]\r\n", battery_value_sum);
		bat_ck_cnt = 0;
		battery_value_sum = 0;
	}	
}
*/

void ap_peripheral_battery_sts_send(void)
{
	msgQSend(ApQ, MSG_APQ_BATTERY_LVL_SHOW, &battery_lvl, sizeof(INT8U), MSG_PRI_NORMAL);
	if(previous_direction){
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_SHOW | ICON_BATTERY_CHARGED));
	}else{
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | ICON_BATTERY_CHARGED));
	}
}
#endif

#if C_SCREEN_SAVER == CUSTOM_ON
void ap_peripheral_lcd_backlight_set(INT8U type)
{
	if (type == BL_ON) {
		if (lcd_bl_sts) {
			lcd_bl_sts = 0;
			tft_backlight_en_set(TRUE);
			DBG_PRINT("LCD ON\r\n");
		}
	} else {
		if (!lcd_bl_sts) {
			lcd_bl_sts = 1;
			tft_backlight_en_set(FALSE);
			DBG_PRINT("LCD OFF\r\n");
		}
	}
}
#endif

void ap_peripheral_night_mode_set(INT8U type)	//wwj add
{
	if(type) {
	  	gpio_write_io(IR_CTRL, 1);
	} else {
	  	gpio_write_io(IR_CTRL, 0);
	}
}


void ap_peripheral_key_init(void)
{
	INT32U i;

	gp_memset((INT8S *) &key_map, NULL, sizeof(KEYSTATUS));
	ap_peripheral_key_register(GENERAL_KEY);
	for (i=0 ; i<USE_IOKEY_NO ; i++) {
		if (key_map[i].key_io) {
			gpio_init_io(key_map[i].key_io, GPIO_INPUT);
			gpio_set_port_attribute(key_map[i].key_io, ATTRIBUTE_LOW);
	  		gpio_write_io(key_map[i].key_io, KEY_ACTIVE^1);
	  		if (key_map[i].key_io == PW_KEY) {
#if KEY_ACTIVE
				while (gpio_read_io(key_map[i].key_io)) {
#else
				while (!gpio_read_io(key_map[i].key_io)) {
#endif
					OSTimeDly(5);
				}
	  		}
	  	}
  	}
  	if (key_detect_timerid == 0xFF) {
		key_detect_timerid = PERIPHERAL_KEY_TIMER_ID;
  		sys_set_timer((void*)msgQSend, (void*) PeripheralTaskQ, MSG_PERIPHERAL_TASK_KEY_DETECT, key_detect_timerid, PERI_TIME_INTERVAL_KEY_DETECT);
  	}
}

void ap_peripheral_key_register(INT8U type)
{
	INT32U i;

	if (type == GENERAL_KEY) {
#if USE_IOKEY_NO == 5 && USE_ADKEY_NO == 0
		key_map[0].key_io = FUNCTION_KEY;
		key_map[0].key_function = (KEYFUNC) ap_peripheral_function_key_exe;
		key_map[1].key_io = NEXT_KEY;
		key_map[1].key_function = (KEYFUNC) ap_peripheral_next_key_exe;
		key_map[2].key_io = PREVIOUS_KEY;
		key_map[2].key_function = (KEYFUNC) ap_peripheral_prev_key_exe;
		key_map[3].key_io = OK_KEY;
		key_map[3].key_function = (KEYFUNC) ap_peripheral_ok_key_exe;
		key_map[4].key_io = MENU_KEY;
		key_map[4].key_function = (KEYFUNC) ap_peripheral_menu_key_exe;
#elif USE_IOKEY_NO == 2 && USE_ADKEY_NO == 4
		key_map[0].key_io = OK_KEY;
		key_map[0].key_function = (KEYFUNC) ap_peripheral_ok_key_exe;
		key_map[1].key_io = PW_KEY;
		key_map[1].key_function = (KEYFUNC) ap_peripheral_pw_key_exe;

		//AD key
		ad_key_map[MENU_KEY].key_io = MENU_KEY;
		ad_key_map[MENU_KEY].key_function = (KEYFUNC) ap_peripheral_menu_key_exe;

		ad_key_map[FUNCTION_KEY].key_io = FUNCTION_KEY;
		ad_key_map[FUNCTION_KEY].key_function = (KEYFUNC) ap_peripheral_function_key_exe;
		ad_key_map[NEXT_KEY].key_io = NEXT_KEY;
		ad_key_map[NEXT_KEY].key_function = (KEYFUNC) ap_peripheral_next_key_exe;
		ad_key_map[PREVIOUS_KEY].key_io = PREVIOUS_KEY;
		ad_key_map[PREVIOUS_KEY].key_function = (KEYFUNC) ap_peripheral_prev_key_exe;
#elif USE_IOKEY_NO == 1 && USE_ADKEY_NO == 5
		key_map[0].key_io = PW_KEY;
		key_map[0].key_function = (KEYFUNC) ap_peripheral_pw_key_exe;

		ad_key_map[MENU_KEY].key_io = MENU_KEY;
		ad_key_map[MENU_KEY].key_function = (KEYFUNC) ap_peripheral_menu_key_exe;
		ad_key_map[FUNCTION_KEY].key_io = FUNCTION_KEY;
		ad_key_map[FUNCTION_KEY].key_function = (KEYFUNC) ap_peripheral_function_key_exe;
		ad_key_map[NEXT_KEY].key_io = NEXT_KEY;
		ad_key_map[NEXT_KEY].key_function = (KEYFUNC) ap_peripheral_next_key_exe;
		ad_key_map[PREVIOUS_KEY].key_io = PREVIOUS_KEY;
		ad_key_map[PREVIOUS_KEY].key_function = (KEYFUNC) ap_peripheral_prev_key_exe;
		ad_key_map[OK_KEY].key_io = OK_KEY;
		ad_key_map[OK_KEY].key_function = (KEYFUNC) ap_peripheral_ok_key_exe;
#else
		#error "Key define error, you can redefine in <customer.h> or create a new set in here."
#endif
	} else if (type == USBD_DETECT) {
//		key_map[0].key_io = C_USBDEVICE_PIN;
//		key_map[0].key_function = (KEYFUNC) ap_peripheral_usbd_plug_out_exe;
#if USE_IOKEY_NO
		for (i=0 ; i<USE_IOKEY_NO ; i++) {
			key_map[i].key_io = NULL;
		}
#endif
#if USE_ADKEY_NO
		for (i=0 ; i<USE_ADKEY_NO ; i++) {
			ad_key_map[i].key_function = ap_peripheral_null_key_exe;
		}
#endif
	} else if (type == DISABLE_KEY) {
		for (i=0 ; i<USE_IOKEY_NO ; i++) {
			key_map[i].key_io = NULL;
		}
	}
}

void LED_service(void)	//wwj add
{
	if(led_cnt)
	{
		led_cnt--;
	}

	switch(led_status & 0x000f)
	{
	case LED_STATUS_FLASH:	//1
		if(led_cnt == 0)
		{
			led_status ^= 0x0010;
			if(led_status & 0x0010)
			{
				gpio_write_io(LED, DATA_HIGH);
				led_cnt = PERI_TIME_LED_FLASH_ON;
			} else {
				gpio_write_io(LED, DATA_LOW);
				led_status = 0;
			}
		}
		break;

	case LED_STATUS_BLINK:	//2
		if(led_cnt == 0)
		{
			led_status ^= 0x0010;
			if(led_status & 0x0010)
			{
				gpio_write_io(LED, DATA_HIGH);
				led_cnt = PERI_TIME_LED_BLINK_ON;
			} else {
				gpio_write_io(LED, DATA_LOW);
				led_cnt = PERI_TIME_LED_BLINK_OFF;
			}
		}
		break;

	default:
		break;
	}
}


//#define SA_TIME	60	//seconds, for screen saver time. Temporary use "define" before set in "STATE_SETTING".
void ap_peripheral_key_judge(void)
{
	INT32U i;
	static INT8U long_func_key_active_flag = 0;

	LED_service();	//wwj add

	for (i=0 ; i<USE_IOKEY_NO ; i++) {
		if (key_map[i].key_io) {
#if KEY_ACTIVE
			if (gpio_read_io(key_map[i].key_io)) {
#else
			if (!gpio_read_io(key_map[i].key_io)) {
#endif
#if C_SCREEN_SAVER == CUSTOM_ON
				key_active_cnt = 0;
				//ap_peripheral_lcd_backlight_set(BL_ON);	//wwj mark
#endif
				key_map[i].key_cnt += 1;
				if (key_map[i].key_io == PW_KEY) {
					if (key_map[i].key_cnt > 24) {
						key_map[i].key_function(&(key_map[i].key_cnt));
					}
				} else {
					if(long_func_key_active_flag == 0) {
						if (key_map[i].key_cnt > 56) {
							long_func_key_active_flag = 1;
							key_map[i].key_function(&(key_map[i].key_cnt));
						}
					} else {
						key_map[i].key_cnt = 0;
					}
				}
				if (key_map[i].key_cnt == 65535) {
					key_map[i].key_cnt = 16;
				}
			} else {
				if (key_map[i].key_io != PW_KEY) {
					long_func_key_active_flag = 0;
				}

				if (key_map[i].key_cnt) {
					if (key_map[i].key_io != PW_KEY) {
						key_map[i].key_function(&(key_map[i].key_cnt));
					} else {
							//wwj mark
						if(key_map[i].key_cnt > 6) {	//wwj add
							key_map[i].key_function(&(key_map[i].key_cnt));
						}
						key_map[i].key_cnt = 0;
					}
				} else {
#if C_SCREEN_SAVER == CUSTOM_ON
					INT32U cnt_sec;
					//INT8U screen_auto_off;
					INT32U screen_auto_off;	//wwj modify

					screen_auto_off = ap_state_config_auto_off_get();//ap_state_config_auto_off_get();	//wwj modify
					if ((screen_auto_off != 0) && !auto_off_force_disable && !s_usbd_pin && !ap_state_config_md_get()) { //don't auto off under conditions:
																														 //1. recording & playing avi files(by auto_off_force_disable)
																														 //2. usb connecting 
																														 //3. motion detect on
						//wwj add
						if(screen_auto_off == 2) {	//3 min
							screen_auto_off = 3;
						} else if (screen_auto_off == 3) { //5min
							screen_auto_off = 5;
						}
						//wwj add end

						if (!lcd_bl_sts && i == 0) {	//wwj mark
							key_active_cnt += PERI_TIME_INTERVAL_KEY_DETECT;
							cnt_sec = (key_active_cnt >> 7) / USE_IOKEY_NO;
							if (cnt_sec > screen_auto_off*60) {
								key_active_cnt = 0;
								msgQSend(ApQ, MSG_APQ_KEY_IDLE, NULL, NULL, MSG_PRI_NORMAL);	//wwj mark
								msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_SCREEN_SAVER_ENABLE, NULL, NULL, MSG_PRI_NORMAL);

								//wwj add
								if(screen_saver_enable) {
									screen_saver_enable = 0;	
									msgQSend(ApQ, MSG_APQ_KEY_WAKE_UP, NULL, NULL, MSG_PRI_NORMAL);
								}
								msgQSend(ApQ, MSG_APQ_POWER_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
								DBG_PRINT("Auto Power off\r\n");
								//wwj add end
							}
						}
					} else {
						key_active_cnt = 0;
					}
#endif
				}
			}
		}
	}
}

#if C_SCREEN_SAVER == CUSTOM_ON
void ap_peripheral_auto_off_force_disable_set(INT8U auto_off_disable)	//wwj add
{
	auto_off_force_disable = auto_off_disable;
}
#else
void ap_peripheral_auto_off_force_disable_set(INT8U auto_off_disable)	//wwj add
{
	
}
#endif
void ap_peripheral_adaptor_out_judge(void)
{
	adp_out_cnt++;
	switch(adp_status) {
		case 0: //unkown state
			if (gpio_read_io(ADP_OUT_PIN)) {
				adp_cnt++;
				if (adp_cnt > 16) {
					adp_out_cnt = 0;
					adp_cnt = 0;
				#if USB_PHY_SUSPEND == 1
					phy_cnt = 0;
				#endif
					adp_status = 1;
				#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
					battery_lvl = 1;
				#endif
				}
			}
			else {
				adp_cnt = 0;
			}
			if (adp_out_cnt > 17) {
				adp_out_cnt = 0;
				adp_status = 3;
	#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
				battery_lvl = 2;
				low_voltage_cnt = 0;
	#endif
			}
			break;
		case 1: //adaptor in state
			if (!gpio_read_io(ADP_OUT_PIN)) {
				if (adp_out_cnt > 8) {
					adp_status = 2;
					low_voltage_cnt = 0;
				}
			}
			else {
				adp_out_cnt = 0;
			}
	#if USB_PHY_SUSPEND == 1
			if (phy_cnt != 0xFFFF) {
				phy_cnt++;
			}
			if ((phy_cnt == PERI_USB_PHY_SUSPEND_TIME) && (s_usbd_pin == 0)) {
				usb_uninitial();
				*P_USBD_CONFIG |= 0x800;	//Switch to USB20PHY
				*P_USBD_CONFIG1 |= 0x100; //[8],SW Suspend For PHY
				phy_cnt = 0xFFFF;
			}
	#endif
			break;
		case 2: //adaptor out state
			if (!gpio_read_io(ADP_OUT_PIN)) {
				if ((adp_out_cnt > PERI_ADP_OUT_PWR_OFF_TIME)/* && (usbd_exit == 0)*/) {	//wwj modify
					ap_peripheral_pw_key_exe(&adp_out_cnt);
				}
				adp_cnt = 0;
			}
			else {
				adp_cnt++;
				if (adp_cnt > 3) {
	#if USB_PHY_SUSPEND == 1
					//*P_USBD_CONFIG &= ~0x800;	//Switch to full speed
					*P_USBD_CONFIG1 &= ~0x100; //phy wakeup
					phy_cnt = 0;
	#endif
					adp_out_cnt = 0;
					adp_status = 1;
					usbd_exit = 0;
					OSQPost(USBAPPTaskQ, (void *) MSG_USBD_INITIAL);
				}
			}
			break;
		case 3://adaptor initial out state
			if (gpio_read_io(ADP_OUT_PIN)) {
				if (adp_out_cnt > 3) {
	#if USB_PHY_SUSPEND == 1
					//*P_USBD_CONFIG &= ~0x800;	//Switch to full speed
					*P_USBD_CONFIG1 &= ~0x100; //phy wakeup
					phy_cnt = 0;
	#endif
					adp_out_cnt = 0;
					adp_status = 1;
					OSQPost(USBAPPTaskQ, (void *) MSG_USBD_INITIAL);
				}
			}
			else {
				adp_out_cnt = 0;
			}
			break;
		default:
			break;
	}

	if (s_usbd_pin == 1) {
		usbd_cnt++;
		if (!gpio_read_io(C_USBDEVICE_PIN)) {
			if (usbd_cnt > 3) {
				usb_uninitial();
	#if USB_PHY_SUSPEND == 1
				*P_USBD_CONFIG |= 0x800;	//Switch to USB20PHY
				*P_USBD_CONFIG1 |= 0x100; //[8],SW Suspend For PHY
	#endif
				ap_peripheral_usbd_plug_out_exe(&usbd_cnt);
			}
		}
		else {
			usbd_cnt = 0;
		}
	}


}

void ap_peripheral_function_key_exe(INT16U *tick_cnt_ptr)
{
	DBG_PRINT("respond function key \r\n");
/*	if (*tick_cnt_ptr < 16) {
		msgQSend(ApQ, MSG_APQ_MODE, NULL, NULL, MSG_PRI_NORMAL);
		*tick_cnt_ptr = 0;
	} else {
		msgQSend(ApQ, MSG_APQ_MENU_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
		*tick_cnt_ptr = 0;
	}
*/
	msgQSend(ApQ, MSG_APQ_AUDIO_EFFECT_MODE, NULL, NULL, MSG_PRI_NORMAL); //wwj add
	if(screen_saver_enable){
		screen_saver_enable = 0;
		msgQSend(ApQ, MSG_APQ_KEY_WAKE_UP, NULL, NULL, MSG_PRI_NORMAL);
	}else{
		msgQSend(ApQ, MSG_APQ_MODE, NULL, NULL, MSG_PRI_NORMAL);
	}
	*tick_cnt_ptr = 0;
}

void ap_peripheral_next_key_exe(INT16U *tick_cnt_ptr)
{
	INT8U data = 0;

	DBG_PRINT("respond next key \r\n");

	msgQSend(ApQ, MSG_APQ_AUDIO_EFFECT_DOWN, NULL, NULL, MSG_PRI_NORMAL); //wwj add
	if(screen_saver_enable){
		screen_saver_enable = 0;
		msgQSend(ApQ, MSG_APQ_KEY_WAKE_UP, NULL, NULL, MSG_PRI_NORMAL);
	}else{
		msgQSend(ApQ, MSG_APQ_NEXT_KEY_ACTIVE, &data, sizeof(INT8U), MSG_PRI_NORMAL);
	}
	*tick_cnt_ptr = 0;
}

void ap_peripheral_prev_key_exe(INT16U *tick_cnt_ptr)
{
	INT8U data = 0;

	DBG_PRINT("respond prev key \r\n");
	msgQSend(ApQ, MSG_APQ_AUDIO_EFFECT_UP, NULL, NULL, MSG_PRI_NORMAL); //wwj add
	if(screen_saver_enable){
		screen_saver_enable = 0;
		msgQSend(ApQ, MSG_APQ_KEY_WAKE_UP, NULL, NULL, MSG_PRI_NORMAL);
	}else{
		msgQSend(ApQ, MSG_APQ_PREV_KEY_ACTIVE, &data, sizeof(INT8U), MSG_PRI_NORMAL);
	}
	*tick_cnt_ptr = 0;
}

void ap_peripheral_ok_key_exe(INT16U *tick_cnt_ptr)
{
	DBG_PRINT("respond OK key \r\n");
	msgQSend(ApQ, MSG_APQ_AUDIO_EFFECT_OK, NULL, NULL, MSG_PRI_NORMAL); //wwj add
	if(screen_saver_enable){
		screen_saver_enable = 0;
		msgQSend(ApQ, MSG_APQ_KEY_WAKE_UP, NULL, NULL, MSG_PRI_NORMAL);
	}else{
		if(*tick_cnt_ptr > 56){
			msgQSend(ApQ, MSG_APQ_FILE_LOCK_DURING_RECORDING, NULL, NULL, MSG_PRI_NORMAL);
		} else {
			msgQSend(ApQ, MSG_APQ_FUNCTION_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
		}
	}
	*tick_cnt_ptr = 0;
}

void ap_peripheral_usbd_plug_out_exe(INT16U *tick_cnt_ptr)
{
	msgQSend(ApQ, MSG_APQ_DISCONNECT_TO_PC, NULL, NULL, MSG_PRI_NORMAL);
	*tick_cnt_ptr = 0;
}

void ap_peripheral_pw_key_exe(INT16U *tick_cnt_ptr)
{
	DBG_PRINT("respond PW key %d\r\n", *tick_cnt_ptr);
	if(screen_saver_enable){
		screen_saver_enable = 0;
		msgQSend(ApQ, MSG_APQ_KEY_WAKE_UP, NULL, NULL, MSG_PRI_NORMAL);
	}else{
		if(*tick_cnt_ptr > 24){	//wwj add
			msgQSend(ApQ, MSG_APQ_POWER_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
		} else {
//			msgQSend(ApQ, MSG_APQ_NIGHT_MODE_KEY, NULL, NULL, MSG_PRI_NORMAL);	//wwj add
			msgQSend(ApQ, MSG_APQ_KEY_IDLE, NULL, NULL, MSG_PRI_NORMAL);	//wwj add			
			msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_SCREEN_SAVER_ENABLE, NULL, NULL, MSG_PRI_NORMAL);	//wwj add

		}
	}
	*tick_cnt_ptr = 0;
}

void ap_peripheral_menu_key_exe(INT16U *tick_cnt_ptr)
{
	DBG_PRINT("respond menu key \r\n");
   	msgQSend(ApQ, MSG_APQ_AUDIO_EFFECT_MENU, NULL, NULL, MSG_PRI_NORMAL);
	if(screen_saver_enable){
		screen_saver_enable = 0;
		msgQSend(ApQ, MSG_APQ_KEY_WAKE_UP, NULL, NULL, MSG_PRI_NORMAL);
	}else{
		msgQSend(ApQ, MSG_APQ_MENU_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
	}
	*tick_cnt_ptr = 0;
}

void ap_peripheral_null_key_exe(INT16U *tick_cnt_ptr)
{

}

INT32S ap_peripheral_irkey_message_init(void)
{
	gp_memset((INT8S *) &ir_keycode[0], 0x0, sizeof(ir_keycode));
	gp_memset((INT8S *) &ir_message[0], 0x0, sizeof(ir_message));

	return 0;
}

INT32S ap_peripheral_irkey_message_set(INT8U key_code, INT32U msg_id)
{
	INT32U i;

	for (i=0; i<C_MAX_IR_KEY_NUM; i++) {
		if (ir_keycode[i] == key_code) {
			// If found, replace the message id and then return
			ir_message[i] = msg_id;

			return 0;
		}
	}

	// If not found, find a free entry and replace it
	for (i=0; i<C_MAX_IR_KEY_NUM; i++) {
		if (ir_message[i] == 0x0) {
			// If found, replace the message id and then return
			ir_keycode[i] = key_code;
			ir_message[i] = msg_id;

			return 0;
		}
	}

	return -1;
}

INT32U ap_peripheral_irkey_message_get(INT8U key_code)
{
	INT32U i;

	for (i=0; i<C_MAX_IR_KEY_NUM; i++) {
		if (ir_keycode[i] == key_code) {
			return ir_message[i];
		}
	}

	return 0;
}

INT8U  num_start;
INT32U keynums;

//wwj add
#define IR_POWER_KEY		0x1e
#define IR_FUNCTION_KEY		0x02
#define	IR_UP_KEY			0x04
#define	IR_MENU_KEY			0x05
#define	IR_DOWN_KEY			0x06
#define	IR_MODE_KEY			0x08
//wwj add end


void ap_peripheral_irkey_handler_exe(INT8U key_code, INT8U key_type)
{
	INT32U msg_id;
	INT32U num,keynums_temp;
	INT32U is_num = 1;
	KEY_EVENT_ST key_event;

//wwj add
	INT16U	key_cnt;

  #if C_SCREEN_SAVER == CUSTOM_ON
	if ((key_code == IR_POWER_KEY) ||
		(key_code == IR_FUNCTION_KEY) ||
		(key_code == IR_UP_KEY) ||
		(key_code == IR_MENU_KEY) ||
		(key_code == IR_DOWN_KEY) ||
		(key_code == IR_MODE_KEY)) {
		key_active_cnt = 0;
//		ap_peripheral_lcd_backlight_set(BL_ON);
	}
  #endif

	switch(key_code) {
		case IR_POWER_KEY:
			key_cnt = 25;
			ap_peripheral_pw_key_exe(&key_cnt);
			break;

		case IR_FUNCTION_KEY:
			key_cnt = 5;
			ap_peripheral_ok_key_exe(&key_cnt);
			break;

		case IR_UP_KEY:
			key_cnt = 5;
			ap_peripheral_prev_key_exe(&key_cnt);
			break;

		case IR_MENU_KEY:
			key_cnt = 5;
			ap_peripheral_menu_key_exe(&key_cnt);
			break;

		case IR_DOWN_KEY:
			key_cnt = 5;
			ap_peripheral_next_key_exe(&key_cnt);
			break;

		case IR_MODE_KEY:
			key_cnt = 5;
			ap_peripheral_function_key_exe(&key_cnt);
			break;

		default:
			break;
	}
	return;

//wwj add end

	msg_id = ap_peripheral_irkey_message_get(key_code);

	if (msg_id == 0x0) {
		return;
	}

	switch(msg_id) {
		case EVENT_KEY_NUM_0:
			num = 0;
			break;
		case EVENT_KEY_NUM_1:
			num = 1;
			break;
		case EVENT_KEY_NUM_2:
			num = 2;
			break;
		case EVENT_KEY_NUM_3:
			num = 3;
			break;
		case EVENT_KEY_NUM_4:
			num = 4;
			break;
		case EVENT_KEY_NUM_5:
			num = 5;
			break;
		case EVENT_KEY_NUM_6:
			num = 6;
			break;
		case EVENT_KEY_NUM_7:
			num = 7;
			break;
		case EVENT_KEY_NUM_8:
			num = 8;
			break;
		case EVENT_KEY_NUM_9:
			num = 9;
			break;
		default:
			is_num = 0;
			break;
	}

	if (is_num && !num_start) {
		keynums = num;
		num_start = 1;
		if( (present_state_ID == STATE_VIDEO_PREVIEW) || (present_state_ID == STATE_VIDEO_RECORD) ){
			OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MP3_INPUT_NUM_SHOW | keynums) );
		}
		return;
	}
	else if (is_num) {
		if( (keynums > 0) && (keynums <= 999)){
			keynums = keynums*10+num;
		}else{
			keynums_temp = keynums - ((keynums/1000)*1000);
			keynums = keynums_temp*10+num;
		}
		if( (present_state_ID == STATE_VIDEO_PREVIEW) || (present_state_ID == STATE_VIDEO_RECORD) ){
			OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MP3_INPUT_NUM_SHOW | keynums) );
		}
		return;
	}
	else if (num_start && ((msg_id == EVENT_KEY_MEDIA_NEXT)||(msg_id == EVENT_KEY_MEDIA_PREV))) {
		msg_id = EVENT_KEY_MEDIA_QUICK_SEL;
		num_start = 0;
		msgQSend(ApQ, msg_id, (void *) &keynums, sizeof(INT32U), MSG_PRI_NORMAL);
		if( (present_state_ID == STATE_VIDEO_PREVIEW) || (present_state_ID == STATE_VIDEO_RECORD) ){
			OSQPost(DisplayTaskQ, (void *)MSG_DISPLAY_TASK_MP3_INPUT_NUM_CLEAR );
		}
		keynums = 0;
		return;
	}
	else if (num_start && ((msg_id == EVENT_KEY_FM_CH_UP)||(msg_id == EVENT_KEY_FM_CH_DOWN))) {
		msg_id = EVENT_KEY_FM_CH_QUICK_SEL;
		num_start = 0;
		msgQSend(ApQ, msg_id, (void *) &keynums, sizeof(INT32U), MSG_PRI_NORMAL);
		if( (present_state_ID == STATE_VIDEO_PREVIEW) || (present_state_ID == STATE_VIDEO_RECORD) ){
			OSQPost(DisplayTaskQ, (void *)MSG_DISPLAY_TASK_MP3_INPUT_NUM_CLEAR );
		}
		keynums = 0;
		return;
	}
	else if (num_start && ((msg_id == EVENT_KEY_MEDIA_VOL_UP)||(msg_id == EVENT_KEY_MEDIA_VOL_DOWN))) {
		msg_id = EVENT_KEY_VOLUME_QUICK_SEL;
		num_start = 0;
		msgQSend(ApQ, msg_id, (void *) &keynums, sizeof(INT32U), MSG_PRI_NORMAL);
		if( (present_state_ID == STATE_VIDEO_PREVIEW) || (present_state_ID == STATE_VIDEO_RECORD) ){
			OSQPost(DisplayTaskQ, (void *)MSG_DISPLAY_TASK_MP3_INPUT_NUM_CLEAR );
		}
		keynums = 0;
		return;
	}
	else {
		num_start = 0;
		keynums = 0;
		if( (present_state_ID == STATE_VIDEO_PREVIEW) || (present_state_ID == STATE_VIDEO_RECORD) ){
			OSQPost(DisplayTaskQ, (void *)MSG_DISPLAY_TASK_MP3_INPUT_NUM_CLEAR );
		}
	}

	key_event.key_source = SOURCE_IR_KEY;
	key_event.key_index = key_code;
	key_event.key_type = key_type;

	msgQSend(ApQ, msg_id, (void *) &key_event, sizeof(KEY_EVENT_ST), MSG_PRI_NORMAL);
}

void ap_peripheral_irkey_clean(void)
{
	num_start = 0;
	keynums = 0;
}
#if LIGHT_DETECT_USE > LIGHT_DETECT_USE_RESISTOR
void ap_peripheral_light_detect(void){
#if LIGHT_DETECT_USE == LIGHT_DETECT_USE_SENSOR
	INT32U temp;
#endif
	if(ap_state_config_night_mode_get()){

  #if LIGHT_DETECT_USE == LIGHT_DETECT_USE_SENSOR
  		SPI_LOCK()
		temp = R_SPI0_CTRL;
		R_SPI0_CTRL &= ~(1 << 15);	//spi  disable
		csi_light = sccb_read(SLAVE_ID,0x00);	//Read AGC-Gain
		//DBG_PRINT("agc = %x\r\n",csi_light);
		R_SPI0_CTRL = temp;
		SPI_UNLOCK();	//wwj add
		if(csi_light > 0x1d){
			gpio_init_io(IR_CTRL, GPIO_OUTPUT);
  			gpio_set_port_attribute(IR_CTRL, 1);
			gpio_write_io(IR_CTRL, DATA_HIGH);	//IR ON
		}
		else if(csi_light < 0x05) {
		//	pio_write_io(IR_CTRL, DATA_HIGH);	//IR OFF
			gpio_init_io(IR_CTRL, GPIO_INPUT);
  			gpio_set_port_attribute(IR_CTRL, 1);
		//	gpio_write_io(IR_CTRL, 1);
		}
  #elif LIGHT_DETECT_USE ==  LIGHT_DETECT_USE_GPIO
		if(gpio_read_io(LIGHT_DETECT)){
		//	pio_write_io(IR_CTRL, DATA_HIGH);	//IR OFF
			gpio_init_io(IR_CTRL, GPIO_INPUT);
  			gpio_set_port_attribute(IR_CTRL, 1);
		}
		else{
			gpio_init_io(IR_CTRL, GPIO_OUTPUT);
  			gpio_set_port_attribute(IR_CTRL, 1);
			gpio_write_io(IR_CTRL, DATA_HIGH);	//IR ON
		}
  #endif
	}

}
#endif

//wwj add
static INT8U backlight_tmr = 0;
void ap_TFT_backlight_tmr_check(void)
{
	if(backlight_tmr)
	{
		backlight_tmr--;
		if((backlight_tmr == 0) && (tv == 1))
		{
			gpio_write_io(TFT_BL, DATA_LOW);
		}
	}
}
//wwj add end

void ap_peripheral_tv_detect(void)
{
//	INT8U tv = 1;
	INT32U tmp;
	if(gpio_read_io(AV_IN_DET) != tv){
		tv = gpio_read_io(AV_IN_DET);
		if(tv == 1){	//display use TFT
		//	gpio_write_io(SPEAKER_EN, DATA_HIGH);	//enable speaker

		//	gpio_write_io(TFT_BL, DATA_LOW);	//wwj mark
			backlight_tmr = PERI_TIME_BACKLIGHT_DELAY;

			tv_disable();
		//	tft_tft_en_set(TRUE);	//wwj mark according to qingyu's testing result
			SPI_LOCK();
			tmp = R_SPI0_CTRL;
			R_SPI0_CTRL = 0;
			Write_COMMAND16i(0xf6);
	 		Write_DATA16i(0x01);
			Write_DATA16i(0x00);
			Write_DATA16i(0x03);
			Write_COMMAND16i(0x11);		//Exit Sleep
 			cmd_delay(120);
 			Write_COMMAND16i(0x29);		//Display on
  			cmd_delay(10);
  			R_SPI0_CTRL = tmp;
			SPI_UNLOCK();
			tft_tft_en_set(TRUE);	//wwj mark according to qingyu's testing result	
			DBG_PRINT("tft out put \r\n");
			R_PPU_IRQ_EN &= ~0x00000800;
			R_PPU_IRQ_EN |= 0x00002000;
		}
		else{	//display use TV
			#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
			  #if DV185	//wwj add
				gpio_write_io(SPEAKER_EN, DATA_LOW);
			  #elif DV188
				gpx_rtc_write(8,0x08);
			  #endif
			#endif
			tft_tft_en_set(FALSE);	//wwj mark according to qingyu's testing result
			SPI_LOCK();
			tmp = R_SPI0_CTRL;
			R_SPI0_CTRL = 0;
			Write_COMMAND16i(0xf6);
	 		Write_DATA16i(0x01);
			Write_DATA16i(0x00);
			Write_DATA16i(0x00);
			Write_COMMAND16i(0x28);		//Display off
 			cmd_delay(20);
 			Write_COMMAND16i(0x10);		//ENTER Sleep
  			cmd_delay(10);
  			R_SPI0_CTRL = tmp;
			SPI_UNLOCK();
			gpio_write_io(TFT_BL, DATA_HIGH);
		//	tft_tft_en_set(FALSE);	//wwj mark according to qingyu's testing result
			if (ap_state_config_tv_out_get() == 0) {
				tv_start(TVSTD_NTSC_J, TV_QVGA, TV_NON_INTERLACE);
			} else {
				tv_start(TVSTD_PAL_N, TV_QVGA, TV_NON_INTERLACE);
			}
			DBG_PRINT("display use TV-out\r\n");
			R_PPU_IRQ_EN &= ~0x00002000;
			R_PPU_IRQ_EN |=  0x00000800;
		}
	}
}

void ap_peripheral_config_store(void)
{
	if (config_cnt++ == PERI_COFING_STORE_INTERVAL) {
		config_cnt = 0;
		msgQSend(ApQ, MSG_APQ_USER_CONFIG_STORE, NULL, NULL, MSG_PRI_NORMAL);
	}
}
