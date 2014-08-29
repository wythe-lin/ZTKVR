#include "application.h"

#define TASK_DISPALY_BUFFER_NO		2

//	Display task frame buffer source
#define DISPALY_BUFF_SRC_SENSOR		0
#define DISPALY_BUFF_SRC_JPEG		1
#define DISPALY_BUFF_SRC_MJPEG		2
#define DISPALY_BUFF_SRC_WAV_TIME	3

#define DISPALY_NONE_EFFECT			0
#define DISPALY_PIC_EFFECT			1
#define DISPALY_PIC_PREVIEW_EFFECT	2
#define DISPALY_STRING_EFFECT		3

#define DISPALY_MP3_INDEX_NONE_EFFECT		0
#define DISPALY_MP3_INDEX_SHOW				1
#define DISPALY_MP3_TOTAL_INDEX_SHOW		2
#define DISPALY_MP3_VOLUME_SHOW				4
#define DISPALY_MP3_FM_CHANNEL_SHOW			8

#define DISPALY_MP3_INPUT_NUM_NONE_EFFECT		0
#define DISPALY_MP3_INPUT_NUM_SHOW				1

extern OS_EVENT *DisplayTaskQ;

extern void task_display_entry(void *para);
extern void ap_display_init(void);
extern void ap_display_setting_frame_buff_set(INT32U frame_buff);
extern void ap_display_queue_init(void);
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
extern void ap_display_left_rec_time_draw(INT32U time, INT8U flag);

#define TRANSPARENT_COLOR     0x8c71

typedef struct {
	INT8U h;
	INT8U m;
	INT8U s;
} DISPLAY_REC_TIME;

extern INT16U icon_battery0[];			//1
extern INT16U icon_battery1[];			//2
extern INT16U icon_battery2[];			//3
extern INT16U icon_battery3[];			//4
extern INT16U icon_battery_red[];		//5
extern INT16U icon_capture[];			//6
extern INT16U icon_video[];				//7
extern INT16U icon_review[];			//8
extern INT16U icon_red_light[];			//9
extern INT16U icon_play[];				//10
extern INT16U icon_pause[];				//11
extern INT16U icon_motion_detect[];		//12
extern INT16U icon_mic_off[];			//13
extern INT16U icon_ir_on[];				//14
extern INT16U icon_video1[];			//15
extern INT16U icon_focus[];				//16
extern INT16U icon_no_sd_card[];			//17
extern INT16U icon_motion_detect_start[];	//18
extern INT16U icon_usb_connect[];			//19
extern INT16U icon_scroll_bar[];			//20
extern INT16U icon_scroll_bar_idx[];		//21
extern INT16U icon_md_sts0[];				//22
extern INT16U icon_md_sts1[];				//23
extern INT16U icon_md_sts2[];				//24
extern INT16U icon_md_sts3[];				//25
extern INT16U icon_md_sts4[];				//26
extern INT16U icon_md_sts5[];				//27
extern INT16U icon_battery_charged[];
extern INT16U icon_locked[];
extern INT16U icon_VGA[];
extern INT16U icon_night_mode_enabled[];
extern INT16U icon_night_mode_disabled[];
extern INT16U icon_audio_record[];			//wwj add

extern INT16U ui_up[];
extern INT16U ui_left[];
extern INT16U ui_right[];
extern INT16U ui_down[];

extern INT16U ui_text_0[];
extern INT16U ui_text_1[];
extern INT16U ui_text_2[];
extern INT16U ui_text_3[];
extern INT16U ui_text_4[];
extern INT16U ui_text_5[];
extern INT16U ui_text_6[];
extern INT16U ui_text_7[];
extern INT16U ui_text_8[];
extern INT16U ui_text_9[];
extern INT16U ui_text_klino	[];	//	"/"
extern INT16U ui_text_colon[];	//	":"

// For state setting
extern INT16U sub_menu_2[];
extern INT16U sub_menu_3[];
extern INT16U sub_menu_4[];
extern INT16U select_bar_gs[];
extern INT16U background_c[];
extern INT16U select_bar_bb[];
extern INT16U background_b[];
extern INT16U select_bar_gb[];
extern INT16U background_a[];

//--------Thumbnail--------------//
extern INT16U thumbnail_video_icon[];
extern INT16U thumbnail_cursor_3x3_96x64[];
extern INT16U thumbnail_cursor_3x3_black_96x64[];
extern INT16U thumbnail_lock_icon[];

//-------------MP3---------------//
extern INT16U icon_mp3_play[];
extern INT16U icon_mp3_pause[];
extern INT16U icon_mp3_play_one[];
extern INT16U icon_mp3_play_all[];
extern INT16U icon_mp3_index[];
extern INT16U icon_mp3_volume[];
extern INT16U icon_mp3_mute[];
extern INT16U icon_mp3_stop[];

extern INT16U mp3_index_0[];
extern INT16U mp3_index_1[];
extern INT16U mp3_index_2[];
extern INT16U mp3_index_3[];
extern INT16U mp3_index_4[];
extern INT16U mp3_index_5[];
extern INT16U mp3_index_6[];
extern INT16U mp3_index_7[];
extern INT16U mp3_index_8[];
extern INT16U mp3_index_9[];
extern INT16U mp3_index_dot[];
extern INT16U mp3_index_MHz[];

extern INT16U mp3_input_0[];
extern INT16U mp3_input_1[];
extern INT16U mp3_input_2[];
extern INT16U mp3_input_3[];
extern INT16U mp3_input_4[];
extern INT16U mp3_input_5[];
extern INT16U mp3_input_6[];
extern INT16U mp3_input_7[];
extern INT16U mp3_input_8[];
extern INT16U mp3_input_9[];

extern INT8U display_mp3_volume;
extern INT16U display_mp3_play_index;
extern INT16U display_mp3_total_index;
extern INT16U display_mp3_FM_channel;
extern INT16U display_mp3_input_num;
extern void timer_counter_force_display(INT8U force_en);