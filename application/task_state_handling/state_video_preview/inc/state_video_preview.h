#include "task_state_handling.h"

#define VIDEO_PREVIEW_UNMOUNT	0x1
#define VIDEO_PREVIEW_BUSY		0x2

extern void state_video_preview_entry(void *para);
extern void ap_video_preview_init(void);
extern void ap_video_preview_exit(void);
extern void ap_video_preview_power_off_handle(void);
extern void ap_video_preview_func_key_active(void);
extern void ap_video_preview_reply_action(STOR_SERV_FILEINFO *file_info_ptr);
extern void ap_video_preview_sts_set(INT8S sts);
extern INT8S ap_video_preview_sts_get(void); //wwj add
extern INT8U sensor_clock_disable;