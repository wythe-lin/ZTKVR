#include "ap_storage_service.h"

#define AVI_REC_MAX_BYTE_SIZE   0x70000000  //1879048192 Bytes
#define AP_STG_MAX_FILE_NUMS    625
#define AUDIO_FILE_SCAN_BUF_FIX   1   // Dominant add to fix file scan index buffer
#define BKGROUND_DETECT_INTERVAL  (20*128)  // 20 second
#define BKGROUND_DEL_INTERVAL     (2*128)   // 2 second

#if AUDIO_FILE_SCAN_BUF_FIX==1
    ALIGN32 INT8U audio_fs_scan_buf[4096];
#endif

extern STOR_SERV_PLAYINFO play_info;  //wwj add

static INT16S g_jpeg_index;
static INT16S g_avi_index;
static INT16S g_file_index;
static INT16S g_play_index;
static INT16U g_file_num;
static INT16U g_err_cnt;
static INT16U g_same_index_num;
static INT16S g_latest_avi_file_index;
static INT32U g_avi_file_time;
static INT32U g_jpg_file_time;
static CHAR g_file_path[24];
static CHAR g_next_file_path[24];
static INT8U curr_storage_id;
static INT8U storage_mount_timerid;
static INT8U g_avi_index_9999_exist;
#if C_AUTO_DEL_FILE == CUSTOM_ON
	static INT8U storage_freesize_timerid;
#endif
static INT16U avi_file_table[AP_STG_MAX_FILE_NUMS];
static INT16U jpg_file_table[AP_STG_MAX_FILE_NUMS];
INT8U device_plug_phase=0;
INT8U usbd_storage_exit;
static INT16S g_oldest_avi_file_index = 0;
static INT8U sd_upgrade_file_flag;
static INT32U BkDelThreadMB;

// dominant add for audio record function declared
static INT16S g_wav_index;
static INT32U g_wav_file_time;
static INT16U wav_file_table[AP_STG_MAX_FILE_NUMS];
INT32S get_file_final_wav_index(INT8U count_total_num_enable);

#define MAX_AUDIO_FILE_BUF_SIZE 2000

st_storage_file_node_info FNodeInfo[MAX_SLOT_NUMS];
INT32U audio_present;
static INT32U g_avi_file_oldest_time;
static INT8U ap_step_work_start=0;
static INT32S retry_del_idx=-1;
static INT8U retry_del_counts=0;
static INT8S bkground_del_disble=0;

//	prototypes
void bkground_del_disable(INT32U disable1_enable0);
INT8S bkground_del_disable_status_get(void);
INT32S get_file_final_avi_index(INT8U count_total_num_enable);
INT32S get_file_final_jpeg_index(INT8U count_total_num_enable);
INT16U get_deleted_file_number(void);
INT16U get_same_index_file_number(void);
static INT16S ap_storage_mount(void);

extern CHAR g_curr_file_name[24];
extern CHAR g_next_file_name[24];

void ap_storage_service_init(void)
{	
#if C_AUTO_DEL_FILE == CUSTOM_ON
	storage_freesize_timerid = 0xFF;
#endif
	g_play_index = -1;
	
	if (ap_storage_service_storage_mount() == STATUS_FAIL) {	
		storage_mount_timerid = STORAGE_SERVICE_MOUNT_TIMER_ID;
		sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, storage_mount_timerid, STORAGE_TIME_INTERVAL_MOUNT);
		sd_upgrade_file_flag = 1;
	}
	else {
		storage_mount_timerid = 0xFF;
	}
}

#if C_AUTO_DEL_FILE == CUSTOM_ON
void ap_storage_service_freesize_check_switch(INT8U type)
{
	if (type == TRUE) {
		if (storage_freesize_timerid == 0xFF) {
			storage_freesize_timerid = STORAGE_SERVICE_FREESIZE_TIMER_ID;
			sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, STORAGE_TIME_INTERVAL_FREESIZE);
		}
	} else {
		if (storage_freesize_timerid != 0xFF) {
			sys_kill_timer(storage_freesize_timerid);
			storage_freesize_timerid = 0xFF;
		}
	}
}

void ap_storage_service_del_thread_mb_set(void)
{
	INT8U temp[5] = {0, 1, 3, 5, 10};	//wwj add
    INT32U SDC_MB_Size;    
    
	BkDelThreadMB = temp[ap_state_config_record_time_get()]*100;//ap_state_config_record_time_get()*100;	//wwj modify
	SDC_MB_Size=drvl2_sd_sector_number_get()/2048;

	if (BkDelThreadMB>(SDC_MB_Size/2) || (BkDelThreadMB==0)) {
		BkDelThreadMB=100;
	}	
}


void ap_storage_service_free_filesize_check()
{
	struct f_info file_info;
    struct stat_t buf_tmp;
	INT32S nRet, ret;
	INT64U total_size, temp_size;

	total_size = 0;//vfsFreeSpace(MINI_DVR_STORAGE_TYPE);
	ret = STATUS_FAIL;
	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet >= 0) {
		while (1) {
			stat((CHAR *) file_info.f_name, &buf_tmp);
			if(!(buf_tmp.st_mode & D_RDONLY)) {
				total_size += file_info.f_size;
				temp_size = total_size >> 20;
				if(temp_size > BkDelThreadMB) {
					ret = STATUS_OK;
					break;
				}
			}

			nRet = _findnext(&file_info);
			if (nRet < 0) {
				break;
			}
		}
	}
	DBG_PRINT("Delect thread %d MB,AVI used size %d MB==========\r\n",BkDelThreadMB,temp_size);
	msgQSend(ApQ, MSG_APQ_FREE_FILESIZE_CHECK_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}


INT32S ap_storage_service_freesize_check_and_del(void)
{
	CHAR f_name[24];
	INT32U i, j;
	INT32S del_index, ret;
	INT64U  disk_free_size;
    INT32S  step_ret;
    INT16S del_ret;
    struct stat_t buf_tmp;

    ret = STATUS_OK;
    
    if(ap_step_work_start==0)
    {
        disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
		ap_storage_service_del_thread_mb_set();
RE_DEL:
        //DBG_PRINT("\r\n[Bkgnd Del Detect (DskFree: %d MB)]\r\n",disk_free_size);
        //DBG_PRINT("[Thread:%dMB]\r\n",BkDelThreadMB);

        if (bkground_del_disble==1)  /*關畢背景砍檔要做的事*/
        {
            if (disk_free_size<50ULL)
            {
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 128*2);
                // if sdc redundant size less than 6 MB, STOP recording now.
                if(disk_free_size<20ULL) {
                    sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 32);
                    if (disk_free_size <= 6ULL/*CARD_FULL_MB_SIZE*/) {
                        AviPacker_Break_Set(1);
                        msgQSend(ApQ, MSG_APQ_VDO_REC_STOP, NULL, NULL, MSG_PRI_NORMAL);
                        //sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DETECT_INTERVAL);
  						ap_storage_service_freesize_check_switch(FALSE);

                    }
                }
            }           
            else if (current_movie_Byte_size_get()>AVI_REC_MAX_BYTE_SIZE)
            {
                DBG_PRINT ("AVI Max Size:%d MB Attend\r\n",current_movie_Byte_size_get()>>20);
                msgQSend(ApQ, MSG_APQ_VDO_REC_RESTART, NULL, NULL, MSG_PRI_NORMAL);
            }
            return 0;
        }

    	if (disk_free_size <= BkDelThreadMB) 
        {
            if(g_avi_index_9999_exist)
            {
			    del_index = g_oldest_avi_file_index;
			    sprintf((char *)f_name, (const char *)"MOVI%04d.avi", g_oldest_avi_file_index);
		    }
            else
            {
    		    del_index = -1;
    		    for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) 
                {
        			if (avi_file_table[i]) {
        				for (j=0 ; j<16 ; j++) {
        					if (avi_file_table[i] & (1<<j)) {
        						del_index = (i << 4) + j;
        						sprintf((char *)f_name, (const char *)"MOVI%04d.avi", del_index);
        						stat(f_name, &buf_tmp);
        						if(buf_tmp.st_mode == D_RDONLY) {
        							del_index = -1;
								} else if(gp_strcmp((INT8S*)f_name,(INT8S*)g_curr_file_name) == 0) {
									del_index = -1;
								} else if(gp_strcmp((INT8S*)f_name,(INT8S*)g_next_file_name) == 0) {
									del_index = -1;
								} else if(gp_strcmp((INT8S*)f_name,(INT8S*)g_next_file_path) == 0) {
									del_index = -1;
        						} else {
									break;
	        					}
        					}
        				}
        				if (del_index != -1) {
        					break;
        				}
        			}
    		    }
                
        		if (del_index == -1) {
        			// There is no more .avi file, must enter no free size handle.
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, NULL, NULL, MSG_PRI_NORMAL);
					bkground_del_disble = 1;
					if (disk_free_size <= 50ULL) {
						sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 128*2);
					} else {
						sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 128*5);
					}
	                return STATUS_FAIL;
        		}
            }
            
    		DBG_PRINT("\r\nDel <%s>\r\n", f_name);
            //chdir("C:\\DCIM");
            unlink_step_start();
            del_ret = unlink(f_name) ;

    		if (del_ret< 0) 
            {
                if (retry_del_idx<0) {
                    retry_del_idx = del_index;
                } else {                
                    retry_del_counts++;
                }
                
                if (retry_del_counts>2)
                {
                    retry_del_idx = -1;  // reset retry index
                    retry_del_counts = 0;  // reset retry counts
    				avi_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));
    				g_file_num--;
                    DBG_PRINT("Del Fail, avoid\r\n");
                } else {
                    DBG_PRINT("Del Fail, retry\r\n");
                }

    			ret = STATUS_FAIL;
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 128);
    		}
            else 
            {
    		    retry_del_idx = -1;  // reset retry index
    		    retry_del_counts = 0;  // reset retry counts
    		    ap_step_work_start=1;
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);
    			avi_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));
    			g_file_num--;
    			DBG_PRINT("Step Del Init OK\r\n");

                if(g_avi_index_9999_exist)
                {
			        get_file_final_avi_index(0);
			        get_file_final_jpeg_index(0);
			        get_file_final_wav_index(0); //wwj add
                }
		    }

            if (disk_free_size<=20ULL)
            {
				//sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DETECT_INTERVAL);
				unlink_step_flush(); 
				ap_step_work_start=0;
				// 緊急砍檔後看看砍完有沒有安全庫存 20MB 以上, 否則繼續砍
				if ((vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20) <= 20ULL) {
					DBG_PRINT ("Re-Delete\r\n");
					goto RE_DEL;
				}
				ret = STATUS_OK;
            }
            ret = STATUS_OK;           
        }
    }
    else 
    {
        step_ret = unlink_step_work();        
        if(step_ret!=0)
        {
            sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);
            DBG_PRINT ("StepDel Continue\r\n");
        } else {
            ap_step_work_start=0;
            DBG_PRINT ("StepDel Done\r\n");

            disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
            DBG_PRINT("\r\n[Dominant (DskFree: %d MB)]\r\n", disk_free_size);
            if (disk_free_size > BkDelThreadMB)
            {
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DETECT_INTERVAL);
            } else {
                // 才剛砍完馬上又說不夠, 遇到小檔, 加速偵測 1 second scan
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 128);
            }
        }

		if (current_movie_Byte_size_get()>AVI_REC_MAX_BYTE_SIZE)
        {
            DBG_PRINT ("AVI Max Size:%d MB Attend\r\n",current_movie_Byte_size_get()>>20);
            msgQSend(ApQ, MSG_APQ_VDO_REC_RESTART, NULL, NULL, MSG_PRI_NORMAL);
        }        
    }
	return ret;
}

#else

INT32S ap_storage_service_freesize_check_and_del(INT32U size)
{
	CHAR f_name[24];
	INT32U i,j;
	INT32U uuu;
	INT32S del_index, ret;
	INT64U  disk_free_size;
	
	uuu = OSTimeGet();	
	disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
	DBG_PRINT("[Judge to delete or not, remain %d MB.] \r\n", disk_free_size);
	ret = STATUS_OK;
	
	if (disk_free_size > size) {
		return ret;
	}
	OSTaskChangePrio(AUD_DEC_PRIORITY,AUD_DEC_HIGH_PRIORITY);
	while (disk_free_size <= size) {
		if(g_avi_index_9999_exist){
			del_index = g_oldest_avi_file_index;
			sprintf((char *)f_name, (const char *)"MOVI%04d.avi", g_oldest_avi_file_index);
		}else{
			del_index = -1;
			for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) {
				if (avi_file_table[i]) {
					for (j=0 ; j<16 ; j++) {
						if (avi_file_table[i] & (1<<j)) {
							del_index = (i << 4) + j;
							sprintf((char *)f_name, (const char *)"MOVI%04d.avi", del_index);
							break;
						}
					}
					if (del_index != -1) {
						break;
					}
				}
			}
			if (del_index == -1) {
				// There is no more .avi file, must enter no free size handle.
				ret = STATUS_FAIL;
				break;
			}
		}
		DBG_PRINT("=%s=\r\n", f_name);
//		uuu = 0;
//		uuu = OSTimeGet();
		chdir("C:\\DCIM");
		if (unlink(f_name) < 0) {
			DBG_PRINT("Delete avi file fail.\r\n");
			ret = STATUS_FAIL;
		} else {
			avi_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));
			g_file_num--;
			DBG_PRINT("Delete avi file OK.\r\n");
			if(g_avi_index_9999_exist){
				get_file_final_avi_index(0);
				get_file_final_jpeg_index(0);							
				get_file_final_wav_index(0);
			}
			ret = STATUS_OK;
		}
//		DBG_PRINT("Time = %d.\r\n", (OSTimeGet() - uuu));
		disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
	}
	OSTaskChangePrio(AUD_DEC_HIGH_PRIORITY,AUD_DEC_PRIORITY);
	DBG_PRINT("[free size =  %d MB.] \r\n", disk_free_size);
	DBG_PRINT("Time = %d.\r\n", (OSTimeGet() - uuu));
	return ret;
}
#endif

void ap_storage_service_file_del(INT32U idx)
{
	INT32S ret, i;
	INT32U max_temp;
	struct stat_t buf_tmp;
	INT64U  disk_free_size;

	if (idx == 0xFFFFFFFF) {
#if C_AUTO_DEL_FILE == CUSTOM_ON	
		ret = ap_storage_service_freesize_check_and_del();
		//msgQSend(ApQ, MSG_APQ_VIDEO_FILE_DEL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
#endif		
	} else {
		close(play_info.file_handle); //wwj add

		stat(g_file_path, &buf_tmp);
		if(buf_tmp.st_mode & D_RDONLY){
			ret = STATUS_OK;
			msgQSend(ApQ, MSG_APQ_FILE_LOCK_ONE_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);	//Do nothing
			return;
		}
		if (unlink(g_file_path) < 0) {
			DBG_PRINT("Delete file fail.\r\n");
			ret = STATUS_FAIL;
		} else {
			DBG_PRINT("Delete file OK.\r\n");
			g_jpeg_index = 0;
            g_avi_index = 0;
            g_file_index = 0;
            g_file_num = 0;
            g_same_index_num = 0;
			g_wav_index = 0;
			//g_play_index = -1;	//Daniel marked for returning to previous one after deleting the current one
			for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) {
				avi_file_table[i] = 0;
				jpg_file_table[i] = 0;
				wav_file_table[i] = 0;
			}
			get_file_final_avi_index(1);
			get_file_final_jpeg_index(1);
			get_file_final_wav_index(1);

			if(g_avi_index_9999_exist)
			{
                if (g_avi_index == 10000) {
                    g_file_index = 0;
                } else if (g_jpeg_index == 10000) {
                    g_file_index = 0;
                } else if (g_wav_index == 10000) {
                    g_file_index = 0;
                } else {
					if(g_avi_file_time > g_jpg_file_time)
					{
                        max_temp = g_avi_index;
						if (g_avi_file_time > g_wav_file_time) {
                        	g_file_index = max_temp;
						}else{
                        	g_file_index = g_wav_index;
                    	}
					} else {
                        max_temp = g_jpeg_index;
						if (g_jpg_file_time > g_wav_file_time) {
                        	g_file_index = max_temp;
						}else{
                        	g_file_index = g_wav_index;
                    	}                        
					}
				}
			} else {
                if (g_avi_index > g_jpeg_index) {
                    max_temp = g_avi_index;
				} else {
                    max_temp = g_jpeg_index;
                }

                if (max_temp > g_wav_index) {
                    g_file_index = max_temp;
				} else {
                    g_file_index = g_wav_index;
				}
			}
			ret = STATUS_OK;
		}
		g_same_index_num = get_same_index_file_number();
		msgQSend(ApQ, MSG_APQ_SELECT_FILE_DEL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
	}

	disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
}

void ap_storage_service_file_delete_all(void)
{
	struct f_info file_info;
	struct stat_t buf_tmp;
	INT8U	locked_files_exist = 0;
	INT32S nRet, i, ret;
	INT32U max_temp;
	INT64U  disk_free_size;

//check if there is any locked video file
	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		locked_files_exist = 0;
	}
	else
	{
		while (1) 
		{
			stat((CHAR *) file_info.f_name, &buf_tmp);
			if(buf_tmp.st_mode & D_RDONLY){
				locked_files_exist = 1;
				break;
			} 
			nRet = _findnext(&file_info);
			if (nRet < 0) {
				locked_files_exist = 0;
				break;
			}
			continue;
		}
	}

    g_file_index = 0;
    g_file_num = 0;
    g_wav_index = 0;
	for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) {
		avi_file_table[i] = 0;
		jpg_file_table[i] = 0;
		wav_file_table[i] = 0;
	}
	g_play_index = -1;
	g_avi_index_9999_exist = g_avi_file_time = g_avi_file_oldest_time = 0;
//AVI	
    g_avi_index = -1;
	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		g_avi_index++;
	}
	else
	{
		g_avi_index = 0;
		while (1) 
		{
			stat((CHAR *) file_info.f_name, &buf_tmp);
			if(buf_tmp.st_mode & D_RDONLY){		//skip if it's locked
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				continue;
			}
			if (unlink((CHAR *) file_info.f_name) == 0) 
			{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				continue;
			}
		}
	}
//JPEG	
	g_jpeg_index = -1;
	g_jpg_file_time = 0;
	nRet = _findfirst("*.jpg", &file_info, D_ALL);
	if (nRet < 0) {
		g_jpeg_index++;
	}
	else
	{
		g_jpeg_index = 0;
		while (1) 
		{
			if (unlink((CHAR *) file_info.f_name)==0) 
			{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				continue;
			}
		}
	}

//WAV
 	g_wav_index = -1;
	g_wav_file_time = 0;

	nRet = _findfirst("*.wav", &file_info, D_ALL);
	if (nRet < 0) {
		g_wav_index++;
	}
	else
	{
		g_wav_index = 0;
		while (1) 
		{
			if (unlink((CHAR *) file_info.f_name)==0) 
			{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				continue;
			}
		}
	}
//Scan again if locked_files_exist
	if(locked_files_exist)
	{
		get_file_final_avi_index(1);
		get_file_final_jpeg_index(1);
		get_file_final_wav_index(1);
        if(g_avi_index_9999_exist)
        {
			if(g_avi_index == 10000) {
				g_file_index = 0;
            } else if (g_jpeg_index == 10000) {
                g_file_index = 0;
            } else if (g_wav_index == 10000) {
                g_file_index = 0;
            }
            else 
            {
				if(g_avi_file_time > g_jpg_file_time)
				{
					max_temp = g_avi_index;
					if (g_avi_file_time > g_wav_file_time) {
						g_file_index = max_temp;
					} else {
						g_file_index = g_wav_index;
					}
				}
				else
				{
                    max_temp = g_jpeg_index;
					if (g_jpg_file_time > g_wav_file_time) {
						g_file_index = max_temp;
					} else {
						g_file_index = g_wav_index;
					}                        
				}
			}
		}
        else
        {
			if (g_avi_index > g_jpeg_index) {
				max_temp = g_avi_index;
			} else {
				max_temp = g_jpeg_index;
			}

			if (max_temp>g_wav_index) {
				g_file_index = max_temp;
			} else {
				g_file_index = g_wav_index;
			}
		}	
	}	
	
	ret = STATUS_OK;
	g_same_index_num = 0;
	msgQSend(ApQ, MSG_APQ_FILE_DEL_ALL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);

	disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
}

void ap_storage_service_file_lock_one(void)
{
	INT32S ret;
	if ( (avi_file_table[g_play_index >> 4] & (1 << (g_play_index & 0xF)))) {	//only lock video files
		_setfattr(g_file_path, D_RDONLY);
	}
	ret = STATUS_OK;
	msgQSend(ApQ, MSG_APQ_FILE_LOCK_ONE_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);	
}

void ap_storage_service_file_lock_all(void)
{
	struct f_info file_info;
	INT32S nRet, ret;

	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		ret = STATUS_FAIL;
	}
	else
	{
		while (1) 
		{
			if (_setfattr((CHAR *) file_info.f_name, D_RDONLY) == 0) 
			{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				continue;
			}
		}
	}
	ret = STATUS_OK;
	msgQSend(ApQ, MSG_APQ_FILE_LOCK_ALL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_file_unlock_one(void)
{
	INT32S ret;
	if ( (avi_file_table[g_play_index >> 4] & (1 << (g_play_index & 0xF)))){	//only unlock video files
		_setfattr(g_file_path, D_NORMAL);
	}
	ret = STATUS_OK;
	msgQSend(ApQ, MSG_APQ_FILE_UNLOCK_ONE_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_file_unlock_all(void)
{
	struct f_info file_info;
	INT32S nRet, ret;
	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		ret = STATUS_FAIL;
	}
	else
	{
		while (1) 
		{
			if (_setfattr((CHAR *) file_info.f_name, D_NORMAL) == 0) 
			{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				continue;
			}
		}
	}
	ret = STATUS_OK;
	msgQSend(ApQ, MSG_APQ_FILE_UNLOCK_ALL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_timer_start(void)
{
	if (storage_mount_timerid == 0xFF) {
		storage_mount_timerid = STORAGE_SERVICE_MOUNT_TIMER_ID;
		sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, storage_mount_timerid, STORAGE_TIME_INTERVAL_MOUNT);
	}
}

void ap_storage_service_timer_stop(void)
{
	if (storage_mount_timerid != 0xFF) {
		sys_kill_timer(storage_mount_timerid);
		storage_mount_timerid = 0xFF;
	}
	//msgQSend(ApQ, MSG_STORAGE_SERVICE_TIMER_STOP_DONE, NULL, 0, MSG_PRI_NORMAL);
} 


INT16S ap_storage_mount(void)
{ 
    INT16S ret;
    ret = _devicemount(MINI_DVR_STORAGE_TYPE);
    chdir("C:\\DCIM");
#if DYNAMIC_QVALUE==1
    if (ret==0) 
    { 
        INT8U  cluster_sectors;
        INT8U  Qvalue;
        INT32U fps;
    
        cluster_sectors = GetSectorsPerCluster(MINI_DVR_STORAGE_TYPE);

        switch (cluster_sectors)
        {
            case 128: //cluster64K
            case 64: //cluster32K
                Qvalue=70;
                fps=AVI_FRAME_RATE;
                break;
            case 32: // cluster16K
                Qvalue=50;
                fps=AVI_FRAME_RATE;
                break;
            case 16: // cluster8K       
            case 8: //cluster4K
            case 4: //cluster2K
            case 2: //cluster1K
            case 1: //cluster512Bytes
                if (drvl2_sd_sector_number_get()>4194304)  // TotalSize>2GB
                {
                    Qvalue=35;
                    fps = 25;
                } else {
                    Qvalue=QUALITY_FACTOR;
                    fps=AVI_FRAME_RATE;
                }     
                break;
            default:
                Qvalue=QUALITY_FACTOR;
                fps=AVI_FRAME_RATE;
                break;
        }
        DBG_PRINT ("Jpeg Q-Value:%d\r\n",Qvalue);
        DBG_PRINT ("Video frame rate:%d fps\r\n",fps);
        video_encode_set_jpeg_quality(Qvalue);
        //video_encode_frame_rate_set(fps);	//wwj delete
    }
#else
        video_encode_set_jpeg_quality(QUALITY_FACTOR);
        video_encode_frame_rate_set(AVI_FRAME_RATE); 
#endif
    return ret;
}


INT32S ap_storage_service_storage_mount(void)
{
    INT16S nRet;
	INT32S i;
	INT16S fd;
    INT32U max_temp;
	
	if (storage_mount_timerid == 0xFF) {
		return STATUS_OK;
	}

#if MINI_DVR_STORAGE_TYPE==T_FLASH 
/* #BEGIN# modify by ShengHua - 2014.10.17 */
//	if (device_plug_phase == 0) {
//		nRet = ap_storage_mount(); //_devicemount(MINI_DVR_STORAGE_TYPE);        
//	} else  {
//		nRet = drvl2_sdc_live_response();
//	}
//
//	if (nRet != 0) {
//		device_plug_phase = 0;  // plug out phase
//		nRet = ap_storage_mount();//_devicemount(MINI_DVR_STORAGE_TYPE);  
//		if (nRet==0) {
//			DBG_PRINT ("Retry OK\r\n");
//			curr_storage_id = NO_STORAGE;
//			device_plug_phase = 1;  // plug in phase
//		} else {
//			usbd_storage_exit = 0;
//		}
//	} else {
//		device_plug_phase = 1;  // plug in phase
//	}        

	if (device_plug_phase==0){
		if(!drvl2_sdc_live_response()){
			nRet = ap_storage_mount();
		}else{
			nRet = 1;
		}
	}else{
		nRet = drvl2_sdc_live_response();
	}
	
	if (nRet!=0) {
		device_plug_phase = 0;  // plug out phase
		usbd_storage_exit = 0;
	} else {
		device_plug_phase = 1;  // plug in phase
	}
/* #END# modify by ShengHua - 2014.10.17 */

#else
    nRet = ap_storage_mount(); //_devicemount(MINI_DVR_STORAGE_TYPE);  
#endif
        
#if 0  // Dominant add for format test
    if (nRet==0 && fff!=0x888) 
    {
        INT32U ft;

        ft = sw_timer_get_counter_L();
        DBG_PRINT ("Format Start....\r\n");
        _format(MINI_DVR_STORAGE_TYPE , FAT32_Type);
        DBG_PRINT ("Format Time: %d ms....\r\n", sw_timer_get_counter_L()-ft);
        DBG_PRINT ("Remount....\r\n");
        _devicemount(MINI_DVR_STORAGE_TYPE);
        fff = 0x888;
        //while(1);
    }
#endif

    if (nRet != 0) {
        if (curr_storage_id != NO_STORAGE) {
            curr_storage_id = NO_STORAGE;
            OSTaskChangePrio(STORAGE_SERVICE_PRIORITY, STORAGE_SERVICE_PRIORITY2);
            msgQSend(ApQ, MSG_STORAGE_SERVICE_NO_STORAGE, &curr_storage_id, sizeof(INT8U), MSG_PRI_NORMAL);
            msgQSend(ApQ, MSG_SYS_STORAGE_SCAN_DONE, NULL, 0, MSG_PRI_NORMAL);
            audio_present = 0;
            audio_send_stop();
            storage_free_buf();
        }
        return STATUS_FAIL;
    } else {
        if ((curr_storage_id != MINI_DVR_STORAGE_TYPE) || (usbd_storage_exit == 1)) {
            curr_storage_id = MINI_DVR_STORAGE_TYPE;
            OSTaskChangePrio(STORAGE_SERVICE_PRIORITY2, STORAGE_SERVICE_PRIORITY);

            mkdir("C:\\DCIM");
            chdir("C:\\DCIM");

            g_jpeg_index = 0;
            g_avi_index = 0;
            g_wav_index = 0;
            g_file_index = 0;
            g_file_num = 0;
            g_same_index_num = 0;
            g_play_index = -1;
            for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) {
                avi_file_table[i] = 0;
                jpg_file_table[i] = 0;
                wav_file_table[i] = 0;
            }
			get_file_final_avi_index(1);
			get_file_final_jpeg_index(1);
			get_file_final_wav_index(1);

            if(g_avi_index_9999_exist) {
                if (g_avi_index == 10000) {
                    g_file_index = 0;
                } else if (g_jpeg_index == 10000) {
                    g_file_index = 0;
                } else if (g_wav_index == 10000) {
                    g_file_index = 0;
                } else {
					if(g_avi_file_time > g_jpg_file_time) {
                        max_temp = g_avi_index;
						if (g_avi_file_time > g_wav_file_time) {
                        	g_file_index = max_temp;
						} else {
                        	g_file_index = g_wav_index;
                    	}
					} else {
                        max_temp = g_jpeg_index;
						if (g_jpg_file_time > g_wav_file_time) {
                        	g_file_index = max_temp;
						} else {
                        	g_file_index = g_wav_index;
                    	}                        
					}
				}
			} else {
                if (g_avi_index > g_jpeg_index) {
                    max_temp = g_avi_index;
				} else {
                    max_temp = g_jpeg_index;
                }

                if(max_temp > g_wav_index) {
                    g_file_index = max_temp;
				} else {
                    g_file_index = g_wav_index;
				}
			}
			g_same_index_num = get_same_index_file_number();

//			if (usbd_storage_exit == 0) {
				msgQSend(ApQ, MSG_STORAGE_SERVICE_MOUNT, &curr_storage_id, sizeof(INT8U), MSG_PRI_NORMAL);
//			}
//			else {
			if(usbd_storage_exit) {	//wwj modify
				usbd_storage_exit = 0;
				if (fm_tx_status_get()) {
					storage_free_buf();
				}
			}
			if (fm_tx_status_get()) {
				ap_storage_scan_file(curr_storage_id);
			}
			
			if (sd_upgrade_file_flag == 0) {
				chdir("C:\\");
				fd = open((CHAR*)"C:\\ztkvr_upgrade.bin", O_RDONLY);
				if (fd < 0) {
					sd_upgrade_file_flag = 1; //no need upgrade
					chdir("C:\\DCIM");
				} else {
					close(fd);
					sd_upgrade_file_flag = 2; //want to upgrade
				}
			}
			ap_storage_service_del_thread_mb_set();
			msgQSend(ApQ, MSG_SYS_STORAGE_SCAN_DONE, NULL, 0, MSG_PRI_NORMAL);
		}		
		return STATUS_OK;
	}
}

void ap_storage_service_file_open_handle(INT32U req_type)
{
	INT32U reply_type;
	STOR_SERV_FILEINFO file_info;
	struct stat_t buf_tmp;
	
	if (storage_mount_timerid != 0xFF) {
		sys_kill_timer(storage_mount_timerid);
		storage_mount_timerid = 0xFF;
	}
	file_info.storage_free_size = (vfsFreeSpace(curr_storage_id) >> 20);

	while ( (avi_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF))))
	{
		stat(g_file_path, &buf_tmp);
		if(buf_tmp.st_mode & D_RDONLY){
			g_file_index++;
			if(g_file_index > 9999) {
				g_file_index = 0;
			}
			//sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_file_index);
		} else {
			break;
		}
	}

    switch (req_type)
    {
    	case MSG_STORAGE_SERVICE_VID_REQ:
			reply_type = MSG_STORAGE_SERVICE_VID_REPLY;
			sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_file_index);
			file_info.file_handle = open (g_file_path, O_WRONLY|O_CREAT|O_TRUNC);
    		if (file_info.file_handle >= 0) 
            {
				file_info.file_path_addr = (INT32U) g_file_path;
				if( (avi_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF))) == 0){
					avi_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
					g_file_num++;
				}
				g_file_index++;
				if(g_file_index == 10000){
					g_file_index = 0;
				}				
				g_avi_index = g_file_index;
				DBG_PRINT("FileName = %s\r\n", file_info.file_path_addr);		
				write(file_info.file_handle, 0, 1);
				lseek(file_info.file_handle, 0, SEEK_SET);		
			}
            break;

        case MSG_STORAGE_SERVICE_PIC_REQ:
			reply_type = MSG_STORAGE_SERVICE_PIC_REPLY;
			sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_file_index);
			file_info.file_handle = open (g_file_path, O_WRONLY|O_CREAT|O_TRUNC);
			if (file_info.file_handle >= 0) {
				file_info.file_path_addr = (INT32U) g_file_path;
				if( (jpg_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF))) == 0){
					jpg_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
					g_file_num++;
				}			
				g_file_index++;
				if(g_file_index == 10000){
					g_file_index = 0;
				}				
				g_jpeg_index = g_file_index;
				DBG_PRINT("FileName = %s\r\n", file_info.file_path_addr);
			}
            break;

		case MSG_STORAGE_SERVICE_AUD_REQ:
    		reply_type = MSG_STORAGE_SERVICE_AUD_REPLY;
    		sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_file_index);
    		file_info.file_handle = open (g_file_path, O_WRONLY|O_CREAT|O_TRUNC);
    		if (file_info.file_handle >= 0) {
    			file_info.file_path_addr = (INT32U) g_file_path;
    			if( (wav_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF))) == 0) {
    				wav_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
    				g_file_num++;
    			}
    			g_file_index++;
				if(g_file_index == 10000){
					g_file_index = 0;
				}    			
    			g_wav_index = g_file_index;
    			DBG_PRINT("FileName = %s\r\n", file_info.file_path_addr);
			}
            break;

        default:
            DBG_PRINT ("UNKNOW STORAGE SERVICE\r\n");
            break;        
    }

	msgQSend(ApQ, reply_type, &file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
}

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
void ap_storage_service_cyclic_record_file_open_handle(INT8U type)
{
	INT32U reply_type;
	STOR_SERV_FILEINFO file_info;
	struct stat_t buf_tmp;
	
	if (storage_mount_timerid != 0xFF) {
		sys_kill_timer(storage_mount_timerid);
		storage_mount_timerid = 0xFF;
	}

	while ( (avi_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF))))
	{
		stat(g_file_path, &buf_tmp);
		if(buf_tmp.st_mode & D_RDONLY){
			g_file_index++;
			if(g_file_index > 9999) {
				g_file_index = 0;
			}
			//sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_file_index);
		} else {
			break;
		}
	}

	if (type == TRUE) {
		reply_type = MSG_STORAGE_SERVICE_VID_CYCLIC_REPLY;
		sprintf((char *)g_next_file_path, (const char *)"MOVI%04d.avi", g_file_index);
		file_info.file_handle = open (g_next_file_path, O_WRONLY|O_CREAT|O_TRUNC);
		if (file_info.file_handle >= 0) {
			file_info.file_path_addr = (INT32U) g_next_file_path;
			if( (avi_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF))) == 0){
				avi_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
				g_file_num++;
			}
			g_file_index++;
			if(g_file_index == 10000){
				g_file_index = 0;
			}
			g_avi_index = g_file_index;
		}	
		msgQSend(ApQ, reply_type, &file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
#if C_AUTO_DEL_FILE == CUSTOM_ON
		if (storage_freesize_timerid == 0xFF) {
			storage_freesize_timerid = STORAGE_SERVICE_FREESIZE_TIMER_ID;
			sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, STORAGE_TIME_INTERVAL_FREESIZE);
		}
#endif		
	} else {
		g_file_index--;
		g_file_num--;
		avi_file_table[g_file_index >> 4] &= ~(1 << (g_file_index & 0xF));
		g_avi_index = g_file_index;
	}
}
#endif

void ap_storage_service_del_table_item(STOR_SERV_FILEINFO *info_ptr)
{
	INT32S	temp;
	CHAR  *pdata;

	if(info_ptr) {
		g_file_index--;
		g_file_num--;

		pdata = (CHAR *)info_ptr->file_path_addr;
		temp = (*(pdata + 4) - 0x30)*1000;
		temp += (*(pdata + 5) - 0x30)*100;
		temp += (*(pdata + 6) - 0x30)*10;
		temp += (*(pdata + 7) - 0x30);
		avi_file_table[temp >> 4] &= ~(1 << (temp & 0xF));
	}
}


void ap_storage_service_play_req(STOR_SERV_PLAYINFO *info_ptr, INT32U req_msg)
{
	INT16S index_tmp, i = 0, k, l;
	INT8U type = (req_msg & 0xFF), err_flag = ((req_msg & 0xFF00) >> 8);
	INT16U given_play_index = ((req_msg>>16) & 0xFFFF);
	struct stat_t buf_tmp;

	if (storage_mount_timerid != 0xFF) {
		sys_kill_timer(storage_mount_timerid);
		storage_mount_timerid = 0xFF;
	}

	info_ptr->search_type = type;
	if (type == STOR_SERV_SEARCH_INIT) {
		g_play_index = -1;
	}
	if (g_play_index < 0) {
		get_file_final_avi_index(0);
		get_file_final_jpeg_index(0);
		get_file_final_wav_index(0);

		if(g_avi_index_9999_exist){
			if(g_avi_file_time > g_jpg_file_time)
			{
				if(g_avi_file_time > g_wav_file_time)
				{
					info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
					g_play_index = g_avi_index - 1;
					sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
				} else {
					info_ptr->file_type = TK_IMAGE_TYPE_WAV;
					g_play_index = g_wav_index - 1;
					sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);				
				}
			}
			else
			{
				if(g_jpg_file_time > g_wav_file_time)
				{
					info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
					g_play_index = g_jpeg_index - 1;
					sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
				}else{
					info_ptr->file_type = TK_IMAGE_TYPE_WAV;
					g_play_index = g_wav_index - 1;
					sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);
				
				}
			}
		} else {
			if (g_jpeg_index > g_avi_index) 
			{
				if(g_jpeg_index > g_wav_index)
				{
					info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
					g_play_index = g_jpeg_index - 1;
					sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
				}else{
					info_ptr->file_type = TK_IMAGE_TYPE_WAV;
					g_play_index = g_wav_index - 1;
					sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);				
				}
			} else if (g_jpeg_index < g_avi_index) {
				if(g_avi_index > g_wav_index)
				{
					info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
					g_play_index = g_avi_index - 1;
					sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
				}else{
					info_ptr->file_type = TK_IMAGE_TYPE_WAV;
					g_play_index = g_wav_index - 1;
					sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);				
				}
			} else if(g_wav_index) {
				info_ptr->file_type = TK_IMAGE_TYPE_WAV;
				g_play_index = g_wav_index - 1;
				sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);			
			} else {
				info_ptr->err_flag = STOR_SERV_NO_MEDIA;
				msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
				return;
			}
		}
	} else {
		if (type == STOR_SERV_SEARCH_PREV) {
			INT8U flag = 0; //wwj add

			index_tmp = g_play_index - 1;
			if (index_tmp < 0) {
				index_tmp = 9999;
			}
			k = index_tmp >> 4;
			l = index_tmp & 0xF;
			while (i <= 626) {
				i++;
				if (avi_file_table[k] || jpg_file_table[k] || wav_file_table[k]) {
					for ( ; l>=0 ; l--) {
						if (avi_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
							sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
							flag = 1;  //wwj add
							break;
						}
						if (jpg_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
							sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
							flag = 1;  //wwj add
							break;
						}
						if (wav_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_WAV;
							sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);
							flag = 1;  //wwj add
							break;
						}						
					}
					//l = 0xF; //wwj mark
					//if (index_tmp != g_play_index - 1) {
					if(flag) {
						break;
					}
				}
				l = 0xF; //wwj add
				k--;
				if (k < 0) {
					k = 624;
				}
			}
			if (i > 626) {
				info_ptr->err_flag = STOR_SERV_NO_MEDIA;
				g_err_cnt = 0;
				msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
				return;
			}
		} else if (type == STOR_SERV_SEARCH_NEXT) {
			INT8U flag = 0; //wwj add

			index_tmp = g_play_index + 1;
			if (index_tmp > 9999) {
				index_tmp = 0;
			}
			k = index_tmp >> 4;
			l = index_tmp & 0xF;
			while (i <= 626) {
				i++;
				if (avi_file_table[k] || jpg_file_table[k] || wav_file_table[k]) {
					for ( ; l<0x10 ; l++) {
						if (avi_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
							sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
							flag = 1; //wwj add
							break;
						}
						if (jpg_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
							sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
							flag = 1; //wwj add
							break;
						}
						if (wav_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_WAV;
							sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);
							flag = 1; //wwj add
							break;
						}						
					}
					//l = 0x0; //wwj mark
					//if (index_tmp != g_play_index + 1) {
					if(flag) {
						break;
					}
				}
				l = 0x0; //wwj add
				k++;
				if (k > 624) {
					k = 0;
				}
			}
			if (i > 626) {
				info_ptr->err_flag = STOR_SERV_NO_MEDIA;
				g_err_cnt = 0;
				msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
				return;
			}
		} else {
			if (type == STOR_SERV_SEARCH_GIVEN) {
				g_play_index = given_play_index;
			}
			k = g_play_index >> 4;
			l = g_play_index & 0xF;
			if (avi_file_table[k] & (1<<l)) {
				info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
				sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
			} else if (jpg_file_table[k] & (1<<l)) {
				info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
				sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
			} else if (wav_file_table[k] & (1<<l)) {
				info_ptr->file_type = TK_IMAGE_TYPE_WAV;
				sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);
			}
		}
	}
	info_ptr->file_path_addr = (INT32U) g_file_path;
	info_ptr->file_handle = open(g_file_path, O_RDONLY);
	if (info_ptr->file_handle >= 0) {
		stat(g_file_path, &buf_tmp);
		info_ptr->file_size = buf_tmp.st_size;
		if (!err_flag) {
			info_ptr->err_flag = STOR_SERV_OPEN_OK;
			g_err_cnt = 0;
			DBG_PRINT("open file %s OK\n\r",g_file_path);
		} else {
			g_err_cnt++;
			if (g_err_cnt <= g_file_num) {
				info_ptr->err_flag = STOR_SERV_OPEN_OK;
				DBG_PRINT("open file %s OK 1\n\r",g_file_path);
			} else {
				info_ptr->err_flag = STOR_SERV_DECODE_ALL_FAIL;
				DBG_PRINT("open file %s fail 2\n\r",g_file_path);
			}
		}
		
	} else {
		info_ptr->err_flag = STOR_SERV_OPEN_FAIL;
		g_err_cnt = 0;
		DBG_PRINT("open file %s fail\n\r",g_file_path);
	}
	info_ptr->deleted_file_number = get_deleted_file_number();
	info_ptr->play_index = g_play_index;
	info_ptr->total_file_number = g_file_num - g_same_index_num;
	msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
}

INT32S get_file_final_avi_index(INT8U count_total_num_enable)
{
	CHAR  *pdata;
	INT32S nRet, temp;
	INT32U temp_time;
	struct f_info file_info;	
	struct stat_t buf_tmp;
	
	g_avi_index = -1;
	g_avi_index_9999_exist = g_avi_file_time = g_avi_file_oldest_time = 0;
	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		g_avi_index++;
		return g_avi_index;
	}
	while (1) {	
		pdata = (CHAR *) file_info.f_name;

		// Remove 0KB AVI files
		if (file_info.f_size<512 && unlink((CHAR *) file_info.f_name)==0) {
			nRet = _findnext(&file_info);
			if (nRet < 0) {
				break;
			}
			continue;
		}

        stat((CHAR *) file_info.f_name, &buf_tmp);
		if (gp_strncmp((INT8S *) pdata, (INT8S *) "MOVI", 4) == 0) {
			temp = (*(pdata + 4) - 0x30)*1000;
			temp += (*(pdata + 5) - 0x30)*100;
			temp += (*(pdata + 6) - 0x30)*10;
			temp += (*(pdata + 7) - 0x30);
			if (temp < 10000) {
				avi_file_table[temp >> 4] |= 1 << (temp & 0xF);
				if(count_total_num_enable){
					g_file_num++;
				}
				if (temp > g_avi_index) {
					g_avi_index = temp;
				}
				temp_time = (file_info.f_date<<16)|file_info.f_time;
				if( ((!g_avi_file_time) || (temp_time > g_avi_file_time)) && ((buf_tmp.st_mode & D_RDONLY) == 0) ){
					g_avi_file_time = temp_time;
					g_latest_avi_file_index = temp;
				}
				if( ((!g_avi_file_oldest_time) || (temp_time < g_avi_file_oldest_time)) && ((buf_tmp.st_mode & D_RDONLY) == 0) ){
					g_avi_file_oldest_time = temp_time;
					g_oldest_avi_file_index = temp;
				}		
			}
		}
		nRet = _findnext(&file_info);
		if (nRet < 0) {
			break;	
		}
	}
	g_avi_index++;
	if(g_avi_index == 10000){
		g_avi_index_9999_exist = 1;
		g_avi_index = g_latest_avi_file_index+1;
	}
	
	return g_avi_index;
}

INT32S get_file_final_jpeg_index(INT8U count_total_num_enable)
{
	CHAR  *pdata;
	struct f_info   file_info;
	INT16S latest_file_index;
	INT32S nRet, temp;
	INT32U temp_time;
	
	g_jpeg_index = -1;
	g_jpg_file_time = 0;
		
	nRet = _findfirst("*.jpg", &file_info, D_ALL);
	if (nRet < 0) {
		g_jpeg_index++;
		return g_jpeg_index;
		
	}	
	while (1) {	
		pdata = (CHAR*)file_info.f_name;

		// Remove 0KB JPG files
		if (file_info.f_size<256 && unlink((CHAR *) file_info.f_name)==0) {
			nRet = _findnext(&file_info);
			if (nRet < 0) {
				break;
			}
			continue;
		}

		if (gp_strncmp((INT8S *) pdata, (INT8S *) "PICT", 4) == 0) {
			temp = (*(pdata + 4) - 0x30)*1000;
			temp += (*(pdata + 5) - 0x30)*100;
			temp += (*(pdata + 6) - 0x30)*10;
			temp += (*(pdata + 7) - 0x30);
			if (temp < 10000) {
				jpg_file_table[temp >> 4] |= 1 << (temp & 0xF);
				if(count_total_num_enable){
					g_file_num++;
				}
				if (temp > g_jpeg_index) {
					g_jpeg_index = temp;
				}
				
				temp_time = (file_info.f_date<<16)|file_info.f_time;
				if( (!g_jpg_file_time) || (temp_time > g_jpg_file_time) ){
					g_jpg_file_time = temp_time;
					latest_file_index = temp;
				}			
			}
		}		
		nRet = _findnext(&file_info);
		if (nRet < 0) {
			break;
		}
	}
	g_jpeg_index++;
	if( (g_jpeg_index == 10000) || (g_avi_index_9999_exist == 1) ){
		g_avi_index_9999_exist = 1;
		g_jpeg_index = latest_file_index+1;
		g_avi_index = g_latest_avi_file_index+1;
	}
	
	return g_jpeg_index;
}

INT32S get_file_final_wav_index(INT8U count_total_num_enable)
{
	CHAR  *pdata;
	struct f_info   file_info;
    INT16S latest_file_index;
	INT32S nRet, temp;
    INT32U temp_time;

	g_wav_index = -1;
	g_wav_file_time = 0;

	nRet = _findfirst("*.wav", &file_info, D_ALL);
	if (nRet < 0) {
		g_wav_index++;
		return g_wav_index;

	}
	while (1) {
		pdata = (CHAR*)file_info.f_name;

		// Remove 0KB WAV files
		if (file_info.f_size==0 && unlink((CHAR *) file_info.f_name)==0) {
			nRet = _findnext(&file_info);
			if (nRet < 0) {
				break;
			}
			continue;
		}

		if (gp_strncmp((INT8S *) pdata, (INT8S *) "RECR", 4) == 0) {
			temp = (*(pdata + 4) - 0x30)*1000;
			temp += (*(pdata + 5) - 0x30)*100;
			temp += (*(pdata + 6) - 0x30)*10;
			temp += (*(pdata + 7) - 0x30);

			if (temp < 10000)
            {
				wav_file_table[temp >> 4] |= 1 << (temp & 0xF);
				if(count_total_num_enable){
					g_file_num++;
				}

    			if (temp > g_wav_index) {
    				g_wav_index = temp;
    			}

                temp_time = (file_info.f_date<<16)|file_info.f_time;

                if( (!g_wav_file_time) || (temp_time > g_wav_file_time) ){
					g_wav_file_time = temp_time;
					latest_file_index = temp;
				}
			}
		}

		nRet = _findnext(&file_info);
		if (nRet < 0) {
			break;
		}
	}
	g_wav_index++;
	if( (g_wav_index == 10000) || (g_avi_index_9999_exist == 1) ){
		g_avi_index_9999_exist = 1;
		g_wav_index = latest_file_index+1;
		g_avi_index = g_latest_avi_file_index+1;
	}

	return g_wav_index;	
}

INT16U get_deleted_file_number(void)
{
	INT16U i;
	INT16U deleted_file_number = 0;
	
	for(i=0; i<g_play_index+1; i++)
	{
        if( ((avi_file_table[(g_play_index-i)>>4] & (1<<((g_play_index-i)&0xF))) == 0) && 
          ((jpg_file_table[(g_play_index-i)>>4] & (1<<((g_play_index-i)&0xF))) == 0) &&
          ((wav_file_table[(g_play_index-i)>>4] & (1<<((g_play_index-i)&0xF))) == 0))
		{
			deleted_file_number++;
		}
	}
	return deleted_file_number;
}


INT16U get_same_index_file_number(void)
{
	INT16U i, j;
	INT16U same_index_file_number = 0;
	
	for(i=0; i<AP_STG_MAX_FILE_NUMS; i++)
	{
		for(j=0; j<16; j++)
		{
			if( (avi_file_table[i] & (1<<j)) && (jpg_file_table[i] & (1<<j)) && (wav_file_table[i] & (1<<j)) ){
				same_index_file_number++;
			}
		}
	}
	return same_index_file_number;
}

void ap_storage_service_format_req(void)
{
	INT32U i;
    INT32U max_temp;
	
	  if(drvl2_sd_sector_number_get()<=2097152) {  // Dominant  1GB card should use FAT16
      	_format(MINI_DVR_STORAGE_TYPE, FAT16_Type);
    } else {
        _format(MINI_DVR_STORAGE_TYPE, FAT32_Type);
    }

	mkdir("C:\\DCIM");
	chdir("C:\\DCIM");
	g_jpeg_index = 0;
    g_avi_index = 0;
    g_file_index = 0;
    g_file_num = 0;
    g_wav_index = 0;
	g_play_index = -1;
	for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) {
		avi_file_table[i] = 0;
		jpg_file_table[i] = 0;
        wav_file_table[i] = 0;
	}
	get_file_final_avi_index(1);
	get_file_final_jpeg_index(1);
	get_file_final_wav_index(1);

	if(g_avi_index_9999_exist) {
		if(g_avi_index == 10000) {
			g_file_index = 0;
        } else if (g_jpeg_index == 10000) {
            g_file_index = 0;
        } else if (g_wav_index == 10000) {
            g_file_index = 0;
        } else {
			if(g_avi_file_time > g_jpg_file_time) {
                max_temp = g_avi_index;
				if (g_avi_file_time > g_wav_file_time) {
                	g_file_index = max_temp;
				} else {
                	g_file_index = g_wav_index;
            	}
			} else {
                max_temp = g_jpeg_index;
				if (g_jpg_file_time > g_wav_file_time) {
                	g_file_index = max_temp;
				} else {
                	g_file_index = g_wav_index;
            	}                        
			}
		}
	} else {
	    if (g_avi_index > g_jpeg_index) {
	        max_temp = g_avi_index;
		} else {
	        max_temp = g_jpeg_index;
	    }

	    if (max_temp > g_wav_index) {
	        g_file_index = max_temp;
		}else{
			g_file_index = g_wav_index;
		}
	}
	FNodeInfo[SD_SLOT_ID].audio.MaxFileNum = 0;
	msgQSend(ApQ, MSG_STORAGE_SERVICE_FORMAT_REPLY, NULL, NULL, MSG_PRI_NORMAL);
}

void FileSrvRead(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S read_cnt;

	read_cnt = read(para->fd, para->buf_addr, para->buf_size);
	if(para->result_queue)
	{
		OSQPost(para->result_queue, (void *) read_cnt);
	}
}

INT32S ap_storage_scan_file(INT32U storage_id)
{
	STScanFilePara stScanFilePara;
	INT32S  status;
	INT8U   err;
	
	gp_memset((INT8S*)&FNodeInfo[SD_SLOT_ID],0,sizeof(st_storage_file_node_info));
			
	gp_strcpy((INT8S*)FNodeInfo[SD_SLOT_ID].ext_name,(INT8S*)"mp3");
	
	if (gp_strlen((INT8S*) FNodeInfo[SD_SLOT_ID].ext_name) != 0) {
		if (FNodeInfo[SD_SLOT_ID].ext_name[0] == '|') {
			gp_strcpy((INT8S*)FNodeInfo[SD_SLOT_ID].ext_name,(INT8S*)&FNodeInfo[SD_SLOT_ID].ext_name[1]);
		}
		FNodeInfo[SD_SLOT_ID].audio.disk = storage_id;
		FNodeInfo[SD_SLOT_ID].audio.attr = D_FILE_1;
		FNodeInfo[SD_SLOT_ID].audio.pExtName = FNodeInfo[SD_SLOT_ID].ext_name;

    #if AUDIO_FILE_SCAN_BUF_FIX == 0
        if (FNodeInfo[SD_SLOT_ID].audio.FileBuffer == NULL) { // Dominant add     
		FNodeInfo[SD_SLOT_ID].audio.FileBuffer = (INT16U*) gp_malloc(MAX_AUDIO_FILE_BUF_SIZE*2);
        }
        
		if (FNodeInfo[SD_SLOT_ID].audio.FileBuffer == NULL) {
			DBG_PRINT("allocate audio scan buffer fail\r\n");
			return STATUS_FAIL;
		}
    #else
        FNodeInfo[SD_SLOT_ID].audio.FileBuffer = (INT16U*) &audio_fs_scan_buf[0];
    #endif
		FNodeInfo[SD_SLOT_ID].audio.FileBufferSize = MAX_AUDIO_FILE_BUF_SIZE;
		FNodeInfo[SD_SLOT_ID].audio.FolderBuffer = NULL;
		FNodeInfo[SD_SLOT_ID].audio.FolderBufferSize = 0;

		stScanFilePara.pstFNodeInfo = &(FNodeInfo[SD_SLOT_ID].audio);
		stScanFilePara.result_queue = sys_fsq;

		status = msgQSend(fs_msg_q_id, MSG_FILESRV_SCAN, (void *)&stScanFilePara, sizeof(STScanFilePara), MSG_PRI_NORMAL);
		if (status != 0) {
			DBG_PRINT("Search music file fail!\r\n");
			audio_present = 0;
		}
		else {
			status = (INT32S) OSQPend(sys_fsq, 0, &err);
			if (status != 0) {
				DBG_PRINT("No audio file found!\r\n");
				audio_present = 0;
			}
			else {
				audio_present = 1;
			}
		}
	}
	else {
		audio_present = 0;
	}
	
	return STATUS_OK;
}

void storage_free_buf(void)
{
    INT32U cnt = 0;
    
    if (FNodeInfo[SD_SLOT_ID].audio.flag == 2) {
        while(FNodeInfo[SD_SLOT_ID].audio.flag != 1) {
            OSTimeDly(5);
            if (cnt++ > 20) {
            	break;
            }
        }
    }

#if AUDIO_FILE_SCAN_BUF_FIX == 0   // dominant add
    if (FNodeInfo[SD_SLOT_ID].audio.FileBuffer != NULL) {
        gp_free(FNodeInfo[SD_SLOT_ID].audio.FileBuffer);
        FNodeInfo[SD_SLOT_ID].audio.FileBuffer = NULL;
    }
#endif

    FNodeInfo[SD_SLOT_ID].audio.flag = 0;
    FNodeInfo[SD_SLOT_ID].audio.MaxFileNum = 0;
}

INT8U storage_sd_upgrade_file_flag_get(void)
{
	return sd_upgrade_file_flag;
}

INT32U bkground_del_thread_size_get(void)
{
	if (bkground_del_disble!=1) {
		return BkDelThreadMB;
	} else {
		return 0;
	}
}

void bkground_del_disable(INT32U disable1_enable0)
{
    bkground_del_disble=disable1_enable0;
}

INT8S bkground_del_disable_status_get(void)
{
    return bkground_del_disble;
}


