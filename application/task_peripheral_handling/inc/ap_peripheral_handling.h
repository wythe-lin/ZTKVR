#include "ztkconfigs.h"
#include "task_peripheral_handling.h"

#define PERI_TIME_INTERVAL_KEY_DETECT	4		//128 = 1s
#define PERI_TIME_INTERVAL_AD_DETECT	4		//128 = 1s

//wwj add
#define PERI_TIME_LED_FLASH_ON			64/PERI_TIME_INTERVAL_KEY_DETECT
#define PERI_TIME_LED_BLINK_ON			64/PERI_TIME_INTERVAL_KEY_DETECT
#define PERI_TIME_LED_BLINK_OFF			64/PERI_TIME_INTERVAL_KEY_DETECT

#define PERI_TIME_BACKLIGHT_DELAY		16	//128 = 1s

//wwj add end
#define PERI_ADP_OUT_PWR_OFF_TIME	TIME_ADP_OUT_DLY*128/PERI_TIME_INTERVAL_KEY_DETECT
//#define PERI_ADP_OUT_PWR_OFF_TIME	5*128/PERI_TIME_INTERVAL_KEY_DETECT		// 5s
#define PERI_LOW_VOLTAGE_PWR_OFF_TIME	5*128/(32*PERI_TIME_INTERVAL_AD_DETECT)
#define PERI_USB_PHY_SUSPEND_TIME	4*128/PERI_TIME_INTERVAL_KEY_DETECT

#define PERI_COFING_STORE_INTERVAL      5*128/PERI_TIME_INTERVAL_KEY_DETECT //5s

#if USE_ADKEY_NO
	#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_EMU_V2_0
		#define AD_KEY_1_MIN				0xC000
		#define AD_KEY_2_MAX				0xA520
		#define AD_KEY_2_MIN				0x7320
		#define AD_KEY_3_MAX				0x6F10
		#define AD_KEY_3_MIN				0x4380
		#define AD_KEY_4_MAX				0x3D60
		#define AD_KEY_4_MIN				0x22A0
	#else
		#define AD_KEY_1_MIN				0x0000	// 0xE678
		#define AD_KEY_2_MAX				0x0000	// 0xC350
		#define AD_KEY_2_MIN				0x0000	// 0x9088
		#define AD_KEY_3_MAX				0x0000	// 0x7D00
		#define AD_KEY_3_MIN				0x0000	// 0x5DC0
		#define AD_KEY_4_MAX				0x0000	// 0x4100
		#define AD_KEY_4_MIN				0x0000	// 0x30D4
	#endif
#endif

typedef void (*KEYFUNC)(INT16U *tick_cnt_ptr);

typedef struct
{
	KEYFUNC key_function;
	INT16U key_io;
	INT16U key_cnt;
}KEYSTATUS;

extern void ap_peripheral_init(void);
extern void ap_peripheral_key_judge(void);
extern void ap_peripheral_adaptor_out_judge(void);
extern void ap_peripheral_key_register(INT8U type);
extern void ap_peripheral_motion_detect_judge(void);
extern void ap_peripheral_motion_detect_start(void);
extern void ap_peripheral_motion_detect_stop(void);
extern void ap_peripheral_led_set(INT8U type);
extern void ap_peripheral_led_flash_set(void);	//wwj add
extern void ap_peripheral_led_blink_set(void);	//wwj add
extern void ap_peripheral_zoom_key_flag_set(INT8U flag);
extern void ap_peripheral_lcd_backlight_set(INT8U type);
extern void ap_peripheral_night_mode_set(INT8U type);	//wwj add
extern void ap_peripheral_ad_key_judge(void);
extern void ap_peripheral_battery_check_calculate(void);
extern void ap_peripheral_battery_sts_send(void);
extern void ap_TFT_backlight_tmr_check(void);	//wwj add
extern void ap_peripheral_tv_detect(void);
extern void ap_peripheral_config_store(void);
extern INT16U present_state_ID;
extern void ap_peripheral_irkey_clean(void);
extern void ap_peripheral_light_detect(void);	//wwj add declaration
extern INT8U ap_state_config_night_mode_get(void);	//wwj add declaration
extern INT8U ap_state_config_md_get(void);		//wwj add

extern void Write_COMMAND16i(INT8U cmd);	//wwj add
extern void Write_DATA16i(INT8U Data);	//wwj add
extern void cmd_delay(INT32U i);		//wwj add
extern void SPI_LOCK(void);			//wwj add
extern void SPI_UNLOCK(void);		//wwj add


/* #BEGIN# add by xyz - 2014.11.14 */
extern void	camswitch_init(void);
extern void	camswitch_judge(void);
extern void	camswitch_send_mode(INT8U mode);
extern void	camswitch_set_mode(INT8U mode);
/* #END#   add by xyz - 2014.11.14 */


