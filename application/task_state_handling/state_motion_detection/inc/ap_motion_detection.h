#include "state_motion_detection.h"

#define MOTION_DETECT_FILE_TIME_INTERVAL		60	//unit : second
#define MOTION_DETECT_FILE_TIME_COUNTDOWN		10	//unit : second. It can't bigger than MOTION_DETECT_FILE_TIME_INTERVAL.
													//It will judge contiune to REC or not, if motion detect active before this value to MOTION_DETECT_FILE_TIME_INTERVAL.
#define MOTION_DETECT_TIME_TICK_INTERVAL		128	//128 = 1s

extern void ap_motion_detect_init(void);
extern void ap_motion_detect_exit(void);
extern void ap_motion_detect_power_off_handle(void);
extern void ap_motion_detect_func_key_active(void);
extern void ap_motion_detect_video_record_active(void);
extern void ap_motion_detect_sts_set(INT8S sts);
extern void ap_motion_detect_timer_tick(void);
extern void ap_motion_detect_timer_tick_end(void);
extern INT32S ap_motion_detect_del_reply(INT32S ret);
extern void ap_motion_detect_error_handle(void);