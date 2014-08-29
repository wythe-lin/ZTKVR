#include "task_state_handling.h"

#define CRC32_POLY     0xEDB88320

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