#include "task_state_handling.h"

#define MOTION_DETECT_UNMOUNT	0x1
#define MOTION_DETECT_BUSY		0x2
#define MOTION_DETECT_LOCK		0x4
#define MOTION_DETECT_CONTINUE	0x8

extern void state_motion_detect_entry(void *para);
extern void ap_motion_detect_init(void);
extern void ap_motion_detect_exit(void);
extern void ap_motion_detect_power_off_handle(void);
extern void ap_motion_detect_func_key_active(void);
extern void ap_motion_detect_video_record_active(void);
extern void ap_motion_detect_reply_action(STOR_SERV_FILEINFO *file_info_ptr);
extern void ap_motion_detect_sts_set(INT8S sts);
extern void ap_motion_detect_timer_tick(void);
extern void ap_motion_detect_timer_tick_end(void);
extern INT32S ap_motion_detect_del_reply(INT32S ret);
extern void ap_motion_detect_error_handle(void);