#include "ztkconfigs.h"
#include "task_storage_service.h"

/* for debug */
#define DEBUG_TASK_STORAGE_SERVICE	1
#if DEBUG_TASK_STORAGE_SERVICE
    #include "gplib.h"
    #define _dmsg(x)			print_string x
#else
    #define _dmsg(x)
#endif

/*  */
MSG_Q_ID StorageServiceQ;

#define SYS_FS_Q_SIZE           1
OS_EVENT *sys_fsq;
void *sys_fs_q[SYS_FS_Q_SIZE];

void *storage_service_q_stack[STORAGE_SERVICE_QUEUE_MAX];
static __align(4) INT8U storage_service_para[STORAGE_SERVICE_QUEUE_MAX_MSG_LEN];

STOR_SERV_PLAYINFO play_info = {0};  //wwj move here

extern INT8U s_usbd_pin;

//	prototypes
void task_storage_service_init(void);

void task_storage_service_init(void)
{
	StorageServiceQ = msgQCreate(STORAGE_SERVICE_QUEUE_MAX, STORAGE_SERVICE_QUEUE_MAX, STORAGE_SERVICE_QUEUE_MAX_MSG_LEN);

	sys_fsq = OSQCreate(sys_fs_q, SYS_FS_Q_SIZE);
}

void task_storage_service_entry(void *para)
{
	INT32U msg_id;
//	STOR_SERV_PLAYINFO play_info;
#if C_AUTO_DEL_FILE == CUSTOM_OFF
	FREE_SIZE_CHECK free_size_check;
#endif

	task_storage_service_init();
	while (1) {	
		if (msgQReceive(StorageServiceQ, &msg_id, storage_service_para, STORAGE_SERVICE_QUEUE_MAX_MSG_LEN) == STATUS_FAIL) {
			continue;
		}		
		switch (msg_id) {
        	case MSG_FILESRV_TASK_READY:
			ap_storage_service_init();        		
        		break;
        	case MSG_STORAGE_SERVICE_STORAGE_CHECK:
        		if (FNodeInfo[SD_SLOT_ID].audio.flag != 2 && s_usbd_pin == 0) { // don't mount when scaning files
        			//DBG_PRINT("storage check\r\n"); //marty mark . 
        			ap_storage_service_storage_mount();
        		}
        		break;
#if C_AUTO_DEL_FILE == CUSTOM_ON
        	case MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH:
        		ap_storage_service_freesize_check_switch(storage_service_para[0]);
        		break;
        	case MSG_STORAGE_SERVICE_FREESIZE_CHECK:
        		ap_storage_service_freesize_check_and_del();
        		break;
        	case MSG_STORAGE_SERVICE_FREE_FILESIZE_CHECK:
				ap_storage_service_free_filesize_check();
        		break;

        	case MSG_STORAGE_SERVICE_AUTO_DEL_LOCK:
        		ap_storage_service_freesize_check_switch(FALSE);
        		break;
#else
			case MSG_STORAGE_SERVICE_FREESIZE_CHECK:
        		{
        			INT32S ret;
        			
        			free_size_check = *((FREE_SIZE_CHECK*)storage_service_para);
        			ret = ap_storage_service_freesize_check_and_del(free_size_check.size);
        			if(free_size_check.tag == 0x5A5A5A5A){
        				free_size_check.tag = 0;
        				msgQSend(ApQ, MSG_APQ_FREE_SIZE_CHECK_REPLY, NULL, NULL, MSG_PRI_NORMAL);
        			} else {
        				msgQSend(ApQ, MSG_APQ_VID_CYCLIC_FILE_DEL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
        			}
        		}
        		break;
#endif
        	case MSG_STORAGE_SERVICE_VIDEO_FILE_DEL:
        		ap_storage_service_file_del(*((INT32U *) storage_service_para));
        		break;
        	case MSG_STORAGE_SERVICE_FILE_DEL_ALL:
        		ap_storage_service_file_delete_all();
        		break; 
        	case MSG_STORAGE_SERVICE_LOCK_ONE:
        		ap_storage_service_file_lock_one();
        		break; 
        	case MSG_STORAGE_SERVICE_LOCK_ALL:
        		ap_storage_service_file_lock_all();
        		break; 
        	case MSG_STORAGE_SERVICE_UNLOCK_ONE:
        		ap_storage_service_file_unlock_one();
        		break; 
        	case MSG_STORAGE_SERVICE_UNLOCK_ALL:
        		ap_storage_service_file_unlock_all();
        		break;         		        		        		        		       		
        	case MSG_STORAGE_SERVICE_TIMER_START:
        		ap_storage_service_timer_start();
        		break;
        	case MSG_STORAGE_SERVICE_TIMER_STOP:
        		ap_storage_service_timer_stop();
        		break;
        	case MSG_STORAGE_SERVICE_AUD_REQ:
        	case MSG_STORAGE_SERVICE_PIC_REQ:
        	case MSG_STORAGE_SERVICE_VID_REQ:
        		ap_storage_service_file_open_handle(msg_id);
        		break;
#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
			case MSG_STORAGE_SERVICE_VID_CYCLIC_REQ:
				ap_storage_service_cyclic_record_file_open_handle(storage_service_para[0]);
				break;
#endif

			case MSG_STORAGE_SERVICE_DEL_TABLE_ITEM:
				ap_storage_service_del_table_item((STOR_SERV_FILEINFO *)storage_service_para);
				break;

        	case MSG_STORAGE_SERVICE_THUMBNAIL_REQ:
        	case MSG_STORAGE_SERVICE_BROWSE_REQ:
        		ap_storage_service_play_req(&play_info, *((INT32U*) storage_service_para));
        		break;
        	case MSG_STORAGE_SERVICE_FORMAT_REQ:
     			ap_storage_service_format_req();
        		break;        		
        	case MSG_FILESRV_FS_READ:
				FileSrvRead((P_TK_FILE_SERVICE_STRUCT)storage_service_para);
				break;
			case MSG_STORAGE_USBD_EXIT:
			case MSG_STORAGE_USBD_PCAM_EXIT:
				device_plug_phase = 0;
				usbd_storage_exit = 1;
				disk_safe_exit(MINI_DVR_STORAGE_TYPE);
				break; 		
        	default:
        		break;
        }
        
    }
}