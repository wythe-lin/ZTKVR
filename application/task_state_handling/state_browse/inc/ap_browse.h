#include "state_browse.h"

#define BROWSE_FILE_NAME_COLOR			255//0
#define BROWSE_FILE_NAME_COLOR_BLACK	0
#define BROWSE_FILE_NAME_START_X		110//155
#define BROWSE_FILE_NAME_START_Y		5
#define BROWSE_DISPLAY_TIMER_INTERVAL	16

extern void ap_browse_init(INT32U prev_state, INT16U play_index);
extern void ap_browse_exit(void);
extern void ap_browse_func_key_active(void);
extern void ap_browse_mjpeg_stop(void);
extern void ap_browse_mjpeg_play_end(void);
extern void ap_browse_next_key_active(INT8U err_flag);
extern void ap_browse_prev_key_active(INT8U err_flag);
extern void ap_browse_sts_set(INT8S sts);
extern void ap_browse_reply_action(STOR_SERV_PLAYINFO *info_ptr);
extern INT8S ap_browse_stop_handle(void);
extern void ap_browse_no_media_show(INT16U str_type);
extern void ap_browse_connect_to_pc(void);
extern void ap_browse_disconnect_to_pc(void);
extern INT8S ap_browse_sts_get(void);
extern void ap_browse_display_update_command(void);
extern INT8U g_browser_reply_action_flag;
extern void ap_browse_display_timer_start(void);
extern void ap_browse_display_timer_stop(void);
extern void ap_browse_display_timer_draw(void);
extern void ap_browse_wav_play(void);
extern void ap_browse_wav_stop(void);
extern void ap_browse_show_file_name(INT8U enable);
