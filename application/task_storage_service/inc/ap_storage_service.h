#include "task_storage_service.h"
#include "stdio.h"

#define STORAGE_TIME_INTERVAL_MOUNT	128	//128 = 1s
#define STORAGE_TIME_INTERVAL_FREESIZE	128*20	//128 = 1s

extern void ap_storage_service_init(void);
extern INT32S ap_storage_service_storage_mount(void);
#if C_AUTO_DEL_FILE == CUSTOM_ON
	extern void ap_storage_service_freesize_check_switch(INT8U type);
	extern INT32S ap_storage_service_freesize_check_and_del(void);
	extern void ap_storage_service_free_filesize_check(void);
#else
	extern INT32S ap_storage_service_freesize_check_and_del(INT32U size);
#endif
extern void ap_storage_service_file_del(INT32U idx);
extern void ap_storage_service_file_open_handle(INT32U type);
extern void ap_storage_service_timer_start(void);
extern void ap_storage_service_timer_stop(void);
extern void ap_storage_service_play_req(STOR_SERV_PLAYINFO *info_ptr, INT32U req_msg);
#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	extern void ap_storage_service_cyclic_record_file_open_handle(INT8U type);
#endif
extern void ap_storage_service_del_table_item(STOR_SERV_FILEINFO *info_ptr); //wwj add
extern void ap_storage_service_format_req(void);
extern INT32S ap_storage_scan_file(INT32U storage_id);
extern void storage_free_buf(void);extern INT32U bkground_del_thread_size_get(void);
extern void bkground_del_disable(INT32U disable1_enable0);
extern INT8S bkground_del_disable_status_get(void);
extern void ap_storage_service_file_delete_all(void);
extern void ap_storage_service_file_lock_one(void);
extern void ap_storage_service_file_lock_all(void);
extern void ap_storage_service_file_unlock_one(void);
extern void ap_storage_service_file_unlock_all(void);
extern INT32S ap_storage_fast_card_flag_get(void);

