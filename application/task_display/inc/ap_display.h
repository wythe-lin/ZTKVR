#include "task_display.h"


#define PHOTO_CAPTURE_TIME_INTERVAL		128	//128 = 1s

extern void ap_display_init(void);
extern void ap_display_queue_init(void);
extern void ap_display_setting_frame_buff_set(INT32U frame_buff);
extern void ap_display_scaler_buff_ack(void);
extern void ap_display_icon_move(DISPLAY_ICONMOVE *icon);
extern void ap_display_icon_sts_set(INT32U msg);
extern void ap_display_icon_sts_clear(INT32U msg);
extern void ap_display_buff_copy_and_draw(INT32U buff_addr, INT16U src_type);
extern void ap_display_timer(void);
extern void ap_display_effect_sts_set(INT8U type);
extern void ap_display_string_draw(STR_ICON *str_info);
extern void ap_display_video_preview_end(void);
extern void ap_display_mp3_index_sts_set(INT8U type);
extern void ap_display_mp3_input_num_sts_set(INT8U type);
extern void timer_counter_force_display(INT8U force_en);
extern void ap_display_left_rec_time_draw(INT32U time, INT8U flag);
extern void ap_display_timer_browser(INT8U type);	//wwj add
extern INT32S vid_dec_get_total_time(void);		//wwj add
extern INT32S vid_dec_get_current_time(void);	//wwj add
extern INT32S audio_dac_get_curr_play_time(void); //wwj add
extern INT32S audio_dac_get_total_time(void); //wwj add

extern INT8U display_mp3_volume;
extern INT16U display_mp3_play_index;
extern INT16U display_mp3_total_index;
extern INT16U display_mp3_FM_channel;
extern INT16U display_mp3_input_num;