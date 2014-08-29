#include "application.h"

#define TRANSPARENT_COLOR     		0x8C71

#define VIDEO_STREAM				0x63643030
#define AUDIO_STREAM				0x62773130

#define SCROLL_DISAPPEAR				0
#define STATA_EXIT						1

#define CARD_FULL_MB_SIZE				6ULL // <= 6MB is mean card full

#define EFFECT_CLICK     0
#define EFFECT_CAMERA    1
#define EFFECT_POWER_ON  2
#define EFFECT_POWER_OFF 3
#define EFFECT_FILE_LOCK 4
#define EFFECT_BEEP		 5

typedef struct {
	INT16U	string_width;
	INT16U	string_height;
} t_STRING_TABLE_STRUCT;

typedef struct{
    INT16U font_color;
    INT16U font_type;
    INT16S pos_x;
    INT16S pos_y;
    INT16U buff_w;
    INT16U buff_h;
    char *str_ptr;
}STRING_ASCII_INFO;

typedef struct{
	INT8U b_alarm_flag;
	INT8U B_alarm_hour;
	INT8U B_alarm_minute;
	INT8U Alarm_Music_idx[3];
} ALARM_STRUCT,*pALARM_STRUCT;

typedef struct{
	INT8U mode;
	INT8U on_hour;
	INT8U on_minute;
	INT8U off_hour;
	INT8U off_minute;
} POWERONOFF_STRUCT, *pPOWERONOFF_STRUCT;

typedef struct
{
	INT32U Frequency;
} FM_STATION_FORMAT;

#define FM_STATION_NUM	20

typedef struct
{
	FM_STATION_FORMAT 	FM_Station[FM_STATION_NUM];
	INT8U				FM_Station_index;
} FM_INFO;

typedef struct {
     INT8U  language;
     INT8U  date_format_display;
     INT8U  time_format_display;
     INT8U  week_format_display;
     INT8U  lcd_backlight;
     INT8U  sound_volume;
     INT8U  current_storage;
     INT8U  current_photo_index[3];
     INT8U  slideshow_duration;
     INT8U  slideshow_bg_music[3];
     INT8U  slideshow_transition;
     INT8U  slideshow_photo_date_on;
     INT8U  calendar_duration;
     INT8U  calendar_displaymode;
     INT8U  thumbnail_mode;
     INT8U  music_play_mode;
     INT8U  music_on_off;
     INT8U  midi_exist_mode;
     INT8U  factory_date[3];                //yy,mm,dd
     INT8U  factory_time[3];                //hh:mm:ss
     ALARM_STRUCT	nv_alarm_struct1;
     ALARM_STRUCT	nv_alarm_struct2;
     INT8U	alarm_modual;
     INT8U	full_screen;
     INT8U	sleep_time;
     POWERONOFF_STRUCT powertime_onoff_struct1;
     POWERONOFF_STRUCT powertime_onoff_struct2;
     INT8U powertime_modual;
	 INT8S powertime_active_id;
	 INT8U powertime_happened;
     INT32U copy_file_counter;
     INT32U Pre_Pow_off_state;
     FM_INFO FM_struct;
	 INT8U	save_as_logo_flag;
	 INT8S	alarm_volume1;
	 INT8S	alarm_volume2;
	 INT8U	alarm_mute1;
	 INT8U	alarm_mute2;
     INT8U	ui_style;
     INT32U base_day;
     INT8U  ifdirty;
} USER_ITEMS;

typedef struct {
    USER_ITEMS item ;
	INT8U crc[4];
	INT8U  DUMMY_BYTE[512-sizeof(USER_ITEMS)-4];
} SYSTEM_USER_OPTION, *pSYSTEM_USER_OPTION;

extern OS_EVENT *StateHandlingQ;
extern void state_handling_entry(void *para);
extern void state_startup_entry(void *para);
extern void state_video_preview_entry(void *para);
extern void state_video_record_entry(void *para);
#if C_MOTION_DETECTION == CUSTOM_ON	
	extern void state_motion_detect_entry(void *para);
#endif	
extern void state_browse_entry(void *para, INT16U play_index);
extern void state_setting_entry(void *para);
extern void state_thumbnail_entry(void *para);

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
extern void ap_state_handling_power_off(void);
extern void ap_state_handling_led_flash_on(void);	//wwj add
extern void ap_state_handling_led_blink_on(void);	//wwj add
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
extern INT32S ap_state_resource_init(void);
extern void ap_state_resource_exit(void);
extern INT32S ap_state_resource_string_resolution_get(STRING_INFO *str_info, t_STRING_TABLE_STRUCT *str_res);
extern INT32S ap_state_resource_string_draw(INT16U *frame_buff, STRING_INFO *str_info);
extern INT32S ap_state_resource_string_ascii_draw(INT16U *frame_buff, STRING_ASCII_INFO *str_ascii_info);
extern INT16U ap_state_resource_language_num_get(void);
extern INT32S ap_state_resource_user_option_load(SYSTEM_USER_OPTION *user_option);
extern INT16U ap_state_resource_ir_key_num_get(void);
extern void ap_state_resource_ir_key_map_get(INT8U **keycode, INT32U **key_msg);
extern void ap_state_config_initial(INT32S status);
extern void ap_state_config_default_set(void);
extern void ap_state_config_store(void);
extern INT32S ap_state_config_load(void);
extern void ap_state_config_ev_set(INT8U ev);
extern INT8U ap_state_config_ev_get(void);
extern void ap_state_config_white_balance_set(INT8U white_balance);
extern INT8U ap_state_config_white_balance_get(void);
extern void ap_state_config_md_set(INT8U md);
extern INT8U ap_state_config_md_get(void);
//extern void ap_state_config_night_mode_set(INT8U night_mode);
extern void ap_state_config_voice_record_switch_set(INT8U voice_record_mode);	//wwj modify
//extern INT8U ap_state_config_night_mode_get(void);
extern INT8U ap_state_config_voice_record_switch_get(void);	//wwj modify
extern void ap_state_config_pic_size_set(INT8U pic_size);
extern INT8U ap_state_config_pic_size_get(void);
extern void ap_state_config_quality_set(INT8U quality);
extern INT8U ap_state_config_quality_get(void);
extern void ap_state_config_scene_mode_set(INT8U scene_mode);
extern INT8U ap_state_config_scene_mode_get(void);
extern void ap_state_config_iso_set(INT8U iso);
extern INT8U ap_state_config_iso_get(void);
extern void ap_state_config_color_set(INT8U color);
extern INT8U ap_state_config_color_get(void);
extern void ap_state_config_saturation_set(INT8U saturation);
extern INT8U ap_state_config_saturation_get(void);
extern void ap_state_config_sharpness_set(INT8U sharpness);
extern INT8U ap_state_config_sharpness_get(void);
extern void ap_state_config_preview_set(INT8U preview);
extern INT8U ap_state_config_preview_get(void);
extern void ap_state_config_burst_set(INT8U burst);
extern INT8U ap_state_config_burst_get(void);
extern void ap_state_config_auto_off_set(INT8U auto_off);
extern INT8U ap_state_config_auto_off_get(void);
extern void ap_state_config_light_freq_set(INT8U light_freq);
extern INT8U ap_state_config_light_freq_get(void);
extern void ap_state_config_tv_out_set(INT8U tv_out);
extern INT8U ap_state_config_tv_out_get(void);
extern void ap_state_config_usb_mode_set(INT8U usb_mode);
extern INT8U ap_state_config_usb_mode_get(void);
extern void ap_state_config_language_set(INT8U language);
extern INT8U ap_state_config_language_get(void);
extern void ap_state_config_volume_set(INT8U sound_volume);
extern INT8U ap_state_config_volume_get(void);
extern void ap_state_config_motion_detect_set(INT8U motion_detect);
extern INT8U ap_state_config_motion_detect_get(void);
extern void ap_state_config_record_time_set(INT8U record_time);
extern INT8U ap_state_config_record_time_get(void);
extern void ap_state_config_date_stamp_set(INT8U date_stamp);
extern INT8U ap_state_config_date_stamp_get(void);
extern void ap_state_config_factory_date_get(INT8U *date);
extern void ap_state_config_factory_time_get(INT8U *time);
extern void ap_state_config_base_day_set(INT32U day);
extern INT32U ap_state_config_base_day_get(void);
extern void ap_state_musiic_play_onoff_set(INT8U on_off);
extern INT8U ap_state_music_play_onoff_get(void);
extern void ap_state_music_set(INT32U music_index);
extern INT32U ap_state_music_get(void);
extern void ap_state_music_play_mode_set(INT8U music_play_mode);
extern INT8U ap_state_music_play_mode_get(void);
extern void ap_state_music_fm_ch_set(INT32U freq);
extern INT32U ap_state_music_fm_ch_get(void);

extern void ap_setting_sensor_command_switch(INT16U cmd_addr, INT16U reg_bit, INT8U enable);
extern void ap_setting_value_set_from_user_config(void);

/* ap music */
typedef struct {
	INT8U   aud_state;
	BOOLEAN mute;
	INT8U   volume;
	INT8U   play_style;
}audio_status_st;

typedef enum
{
	STATE_IDLE,
	STATE_PLAY,
	STATE_PAUSED,
	STATE_AUD_FILE_ERR
}AUDIO_STATE_ENUM;

typedef enum
{
//	PLAY_ONCE,
	PLAY_SEQUENCE,
	PLAY_REPEAT,
	PLAY_MIDI,
	PLAY_SPECIAL_REPEAT,
	PLAY_SPECIAL_BYNAME_REPEAT,
	PLAY_SHUFFLE_MODE
}AUDIO_PLAY_STYLE;


extern void ap_music_init(void);
extern void ap_music_effect_resource_init(void); //wwj add
extern void audio_play_process(void);
extern void audio_play_pause_process(void);
extern void audio_send_pause(void);
extern void audio_send_resume(void);
extern void audio_next_process(void);
extern void audio_prev_process(void);
extern void audio_send_stop(void);
extern void audio_res_play_process(INT32S result);
extern void audio_res_resume_process(INT32S result);
extern void audio_res_pause_process(INT32S result);
extern void audio_res_stop_process(INT32S result);
extern void audio_mute_ctrl_set(BOOLEAN status);
extern void audio_vol_inc_set(void);
extern void audio_vol_dec_set(void);
extern INT8U audio_fg_vol_get(void);
extern void audio_play_style_set(INT8U play_style);
extern void audio_playing_mode_set_process(void);
extern void audio_quick_select(INT32U index);
extern void audio_fm_freq_inc_set(void);
extern void audio_fm_freq_dec_set(void);
extern void audio_fm_freq_quick_set(INT32U freq);
extern void audio_vol_set(INT8U vol);
extern INT8U audio_playing_state_get(void);
extern INT32S audio_effect_play(INT32U effect_type); //wwj add
extern void audio_confirm_handler(STAudioConfirm *aud_con);
extern INT32S storage_file_nums_get(void);
extern INT32S storage_scan_status_get(INT8U *status);
extern INT32S storage_fopen(INT32U file_idx, STORAGE_FINFO *storage_finfo);
extern INT32S ap_music_index_get(void);
extern INT32S storage_file_nums_get(void);
extern INT8U audio_fg_vol_get(void);
extern INT32U audio_fm_freq_ch_get(void);
extern void ap_music_update_icon_status(void);
extern void ap_music_reset(void);
extern INT32S ap_state_common_handling(INT32U msg_id); //wwj add
extern void audio_wav_play(INT16S fd);
extern void audio_wav_pause(void);
extern void audio_wav_resume(void);
extern void audio_wav_stop(void);

extern void ap_state_ir_key_init(void);

extern void ap_state_handling_mp3_index_show_cmd(void);
extern void ap_state_handling_mp3_total_index_show_cmd(void);
extern void ap_state_handling_mp3_all_index_clear_cmd(void);
extern void ap_state_handling_mp3_volume_show_cmd(void);
extern void ap_state_handling_mp3_FM_channel_show_cmd(void);
extern void ap_state_handling_mp3_index_show_zero_cmd(void);
extern void ap_state_handling_mp3_total_index_show_zero_cmd(void);
extern INT16U present_state;
extern INT8U sensor_clock_disable;
extern INT32S music_file_idx;
extern INT8S ap_video_record_sts_get(void);

extern void ap_state_firmware_upgrade(void);
extern INT16S ap_state_resource_time_stamp_position_x_get(void);
extern INT16S ap_state_resource_time_stamp_position_y_get(void);
// pure audio record function
extern void state_audio_record_entry(void *para);



