#include "task_state_handling.h"

#define SCROLL_BAR_TIME_INTERVAL		128*3	//128 = 1s

extern INT32S ap_state_handling_str_draw(INT16U str_index, INT16U str_color);
extern void ap_state_handling_str_draw_exit(void);
extern void ap_state_handling_icon_show_cmd(INT8U cmd1, INT8U cmd2, INT8U cmd3);
extern void ap_state_handling_icon_clear_cmd(INT8U cmd1, INT8U cmd2, INT8U cmd3);
extern void ap_state_handling_connect_to_pc(void);
extern void ap_state_handling_disconnect_to_pc(void);
extern void ap_state_handling_storage_id_set(INT8U stg_id);
extern INT8U ap_state_handling_storage_id_get(void);
extern void ap_state_handling_led_on(void);
extern void ap_state_handling_led_off(void);
extern void ap_state_handling_led_flash_on(void);	//wwj add
extern void ap_state_handling_led_blink_on(void);	//wwj add
extern void ap_state_handling_power_off(void);
extern void ap_state_handling_calendar_init(void);
#if C_BATTERY_DETECT == CUSTOM_ON 
	extern void ap_state_handling_battery_icon_show(INT8U bat_lvl);
	extern void ap_state_handling_charge_icon_show(INT8U charge_flag);	//wwj add
	extern void ap_state_handling_current_bat_lvl_show(void);	//wwj add
	extern void ap_state_handling_current_charge_icon_show(void);	//wwj add
#endif
#if C_SCREEN_SAVER == CUSTOM_ON
	extern void ap_state_handling_lcd_backlight_switch(INT8U enable);
#endif
extern void ap_state_handling_night_mode_switch(void);	//wwj add
extern INT8U ap_state_handling_night_mode_get(void);	//wwj add
extern void ap_state_handling_scroll_bar_init(void);
extern void ap_state_handling_scroll_bar_exit(INT8U exit_type);
extern void ap_state_handling_zoom_active(INT32U *factor, INT32U type);
extern INT32S ap_state_handling_jpeg_decode(STOR_SERV_PLAYINFO *info_ptr, INT32U jpg_output_addr);
extern INT32S jpeg_buffer_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct);
extern void jpeg_memory_allocate(INT32U fifo);
extern void jpeg_scaler_set_parameters(INT32U fifo);
extern void ap_state_ir_key_init(void);
extern void ap_state_handling_mp3_index_show_cmd(void);
extern void ap_state_handling_mp3_total_index_show_cmd(void);
extern void ap_state_handling_mp3_all_index_clear_cmd(void);
extern void ap_state_handling_mp3_volume_show_cmd(void);
extern void ap_state_handling_mp3_FM_channel_show_cmd(void);
extern void ap_state_handling_mp3_index_show_zero_cmd(void);
extern void ap_state_handling_mp3_total_index_show_zero_cmd(void);
extern INT16U present_state;

extern void Write_COMMAND16i(INT8U cmd);	//wwj add
extern void Write_DATA16i(INT8U Data);	//wwj add
extern void cmd_delay(INT32U i);		//wwj add
extern void SPI_LOCK(void);			//wwj add
extern void SPI_UNLOCK(void);		//wwj add
