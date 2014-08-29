#include "task_state_handling.h"

#define AUDIO_RECORD_UNMOUNT				0x1
#define AUDIO_RECORD_BUSY					0x2

extern void state_audio_record_entry(void *para);
extern INT8S ap_audio_record_init(INT32U prev_state);
extern void ap_audio_record_exit(INT32U next_state_msg);
extern INT8U ap_audio_record_sts_get(void);
extern void ap_audio_record_func_key_active(void);
extern INT8S ap_audio_record_reply_action(STOR_SERV_FILEINFO *file_info_ptr);
extern void ap_audio_record_sts_set(INT8S sts);
extern void ap_audio_record_timer_start(void);
extern void ap_audio_record_timer_stop(void);
extern void ap_audio_record_timer_draw(void);

extern void audio_calculate_left_recording_time_enable(void); //wwj add
extern void audio_calculate_left_recording_time_disable(void); //wwj add
