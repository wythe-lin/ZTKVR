#include "task_state_handling.h"

#define VIDEO_RECORD_UNMOUNT	0x1
#define VIDEO_RECORD_BUSY		0x2
#define VIDEO_RECORD_MD			0x4

extern void state_video_record_entry(void *para);
extern void ap_video_record_init(void);
extern void ap_video_record_exit(void);
extern void ap_video_record_power_off_handle(void);
#if C_MOTION_DETECTION == CUSTOM_ON	
	extern void ap_video_record_md_tick(void);
	extern INT8S ap_video_record_md_active(void);
	extern void ap_video_record_md_disable(void);
	extern void ap_video_record_md_icon_update(INT8U sts);
#endif
extern void ap_video_record_func_key_active(INT32U event);
extern void ap_video_record_start(void); //wwj add
extern void ap_video_record_reply_action(STOR_SERV_FILEINFO *file_info_ptr);
extern void ap_video_record_sts_set(INT8S sts);
extern INT8S ap_video_record_sts_get(void);
#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	extern void ap_video_record_cycle_reply_open(STOR_SERV_FILEINFO *file_info_ptr);
	extern void ap_video_record_cycle_reply_action(void);
#endif
extern INT32S ap_video_record_del_reply(INT32S ret);
extern void ap_video_record_error_handle(void);
#if C_AUTO_DEL_FILE == CUSTOM_OFF
	extern void ap_video_cyclic_record_del_reply(INT32S ret);
#endif

extern void ap_video_record_md_icon_clear_all(void);
extern void video_calculate_left_recording_time_enable(void);
extern void video_calculate_left_recording_time_disable(void);
extern void ap_video_record_lock_current_file(void);
extern void ap_video_clear_lock_flag(void);
