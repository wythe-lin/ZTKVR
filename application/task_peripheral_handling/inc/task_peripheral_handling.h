#include "application.h"

#define PERIPHERAL_TASK_QUEUE_MAX			256
#define PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN	20

typedef struct
{
    INT8U  key_code;
    INT8U  key_type;
} st_key_para;

extern MSG_Q_ID PeripheralTaskQ;
extern void ap_peripheral_init(void);
extern void task_peripheral_handling_entry(void *para);
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
extern void ap_peripheral_irkey_handler_exe(INT8U key_code, INT8U key_type);
extern void ap_TFT_backlight_tmr_check(void);	//wwj add
extern void ap_peripheral_tv_detect(void);
extern void ap_peripheral_config_store(void);
extern INT16U present_state_ID;
extern INT8U  usbd_exit;
extern INT8U screen_saver_enable;
extern void ap_peripheral_irkey_clean(void);
extern void ap_peripheral_light_detect(void);	//wwj add declaration
extern INT8U ap_state_config_night_mode_get(void);	//wwj add declaration
