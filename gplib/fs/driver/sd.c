#define CREAT_DRIVERLAYER_STRUCT

#include "fsystem.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (defined SD_EN) && (SD_EN == 1)                               //
//================================================================//
#define FS_FLUSH_CAHCE_EN       1 
#define FS_TBL_MIRROR_SYNC      0
#define FAT_WRITE_TBL_ID        0  //0: FATTbl0->FATTbl1, 1: FATTbl1->FATTbl0
#define FS_CACHE_DBG_EN         0 
#define FS_INFO_CACHE           0

#define SDC_CACHE_SIZE          32768
#define SDC_CACHE_SECTOR_NUMS   (SDC_CACHE_SIZE/512)
#define FS_DRV_PRINT    DBG_PRINT


ALIGN32 INT8U sd_cache_ram[SDC_CACHE_SIZE];

#if (FS_INFO_CACHE==1 || FAT_WRITE_TBL_ID==1)
ALIGN32 INT8U sd_fs_info_ram[512];

#endif
//ALIGN32 INT8U sd_cache_work_buf[512];
INT32U  cache_start_id;
INT32U  cache_end_sec_id;
INT32U  cache_ref_id;
INT32U  cache_in_flag;
INT32U  cache_flush_size;
INT32U  fat_start_id[2];
INT32U  mirror_start_id; // unit: sector
INT32U  mirror_end_id; // unit: sector
INT32U  fat_tbl_size;  // unit: sectors
INT8U   cache_init_flag=0;
INT8U   sd_cache_invalid;
INT32U  BPB_Start_id;
INT32U  fat_cluster_size;  // unit: sectors
INT32S  miss_cnt;
INT32S  hit_cnt;
INT8U   cache_switch_cnt;
INT8U   cache_sync_flag;  //看看是否有連進 cache 之情況
INT8U   miss_thread;
INT8U   fat_cache_L1_en;
INT32U  cache_start_id;
static INT8U fat_type;
static INT32U fs_info_start;
#if FAT_WRITE_TBL_ID==1
static INT32U FSI_Free_Count=0;
static INT32U FSI_Next_Free=0;

#endif

#if FAT_WRITE_TBL_ID == 0
  #define FAT_MAIN_TBL_ID         0
  #define FAT_SECOND_TBL_ID       1
#elif FAT_WRITE_TBL_ID == 1
  #define FAT_MAIN_TBL_ID         1
  #define FAT_SECOND_TBL_ID       0
  #define FS_TBL_MIRROR_SYNC      1  // when FAT2->1 Mirror table must be enable
#endif

#define C_DATA3_PIN       38//IOC[6]
#define C_DATA1_PIN       40//IOC[8]
#define C_DATA2_PIN       41//IOC[9]

#define SD_RW_RETRY_COUNT       3
#define MISS_THREAD_MIN         10
#define MISS_THREAD_MAX         70
#define MISS_HIT_SAMPLE_CNTS    20  // totale count to sample for calculating miss rate    

static void FsCpy4(void *_dst, const void *_src, int len);
static INT32S SD_Initial(void);
static INT32S SD_Uninitial(void);
static void SD_GetDrvInfo(struct DrvInfo* info);
static INT32S SD_ReadSector(INT32U blkno, INT32U blkcnt, INT32U buf);
static INT32S SD_WriteSector(INT32U blkno, INT32U blkcnt, INT32U buf);
static INT32S SD_Flush(void);
static INT32S fat_l1_cache_init(void);
INT32S SD_tbl_mirror_sync(void);
INT32S cache_sync(void);
void fat_l1_cache_reinit(void);
INT32S cache_read_from_nv(void);
INT32S fs_info_flush(void);

#if FAT_WRITE_TBL_ID == 1  //FAT2->FAT1
INT32S SD_tbl_mirror_init(void);
#endif

INT32U DMAmmCopy2(INT32U wSourceAddr, INT32U wTargeAddr, INT16U Count); //count is byte num

struct Drv_FileSystem FS_SD_driver = {
	"SD",
	DEVICE_READ_ALLOW|DEVICE_WRITE_ALLOW,
	SD_Initial,
	SD_Uninitial,
	SD_GetDrvInfo,
	SD_ReadSector,
	SD_WriteSector,
	SD_Flush,
	SD_tbl_mirror_sync,
};

void fat_l1_cahhe_uninit(void)
{
    fat_cache_L1_en=0;
}

void fat_l1_cache_reinit(void)
{
    cache_init_flag=0;
    fat_l1_cache_init();
}

INT32S fat_l1_cache_init(void)  
{
    INT32S ret;
    //INT8U *cache_init_buf;
    INT8U *byte_buf;
    INT16U *short_buf;
    INT32U *word_buf;
    INT32U Next_Free=0xFFFFFFFF; 
    INT32U FS_info_tag=0x00000000;
    INT32U Free_Count=0xFFFFFFFF;
  #if FS_INFO_CACHE == 1
    INT32U *fi_word_buf=(INT32U *) &sd_fs_info_ram[0];
  #endif

  #if FAT_WRITE_TBL_ID==1    
    INT8U  mirror_en=1;
  #endif
    
    if (cache_init_flag!=0xA8)
    {
        INT32U *sd_cache_work_buf=(INT32U *) &sd_cache_ram[0];
      #if FS_CACHE_DBG_EN==1
        DBG_PRINT ("SD FatL1 cache Init...");
      #endif
        hit_cnt = 0;
        miss_cnt = 0;
        cache_in_flag = 0;
        cache_switch_cnt = 0;
        cache_sync_flag = 0;
        cache_ref_id = 0xFFFFFFFF;
        cache_start_id = 0xFFFFFFFF;
        fs_info_start = 0xFFFFFFFF;
        BPB_Start_id = 0;  // NO MBR, BPB always is 0
        miss_thread = MISS_THREAD_MIN;
        fat_cache_L1_en=1;
        short_buf = (INT16U *) &sd_cache_work_buf[0];
        word_buf = (INT32U *) &sd_cache_work_buf[0];
        byte_buf = (INT8U *) &sd_cache_work_buf[0];
        ret = drvl2_sd_read(BPB_Start_id, (INT32U *) &sd_cache_work_buf[0], 1);

        if (ret==0 && short_buf[0x1fE/2]==0xAA55 && byte_buf[0]!=0xEB) 
        {
            
            BPB_Start_id = ((short_buf[0x1C8/2]<<16) | short_buf[0x1C6/2]);
            DBG_PRINT ("MBR Find, first part offset :0x%x\r\n",BPB_Start_id);
            ret = drvl2_sd_read(BPB_Start_id, (INT32U *) &sd_cache_work_buf[0], 1);

        }

        if(ret==0)
        {
            fat_cluster_size = byte_buf[13];  // 8 sectors
            fat_start_id[0]=short_buf[7]+BPB_Start_id;   //0x24*0x200=0x4800 
            fat_tbl_size = short_buf[22/2];
            if (fat_tbl_size==0) {
                // FIND FAT32
                fat_type=32;
                DBG_PRINT ("FAT32\r\n");
                fat_tbl_size = word_buf[9];  //0x1d72*0x200=0x3AE400
                fs_info_start = short_buf[48/2]+BPB_Start_id;

              #if FS_INFO_CACHE == 1
                drvl2_sd_read(fs_info_start, (INT32U *) &sd_fs_info_ram[0], 1);
                FS_info_tag = fi_word_buf[0];
                Free_Count = fi_word_buf[488/4]+BPB_Start_id;
                Next_Free = fi_word_buf[492/4]+BPB_Start_id;
              #else
                drvl2_sd_read(fs_info_start, (INT32U *) &sd_cache_work_buf[0], 1);
                FS_info_tag = word_buf[0];
                Free_Count = word_buf[488/4]+BPB_Start_id;
                Next_Free = word_buf[492/4]+BPB_Start_id;
              #endif
              
                DBG_PRINT ("Fs info Tag: 0x%x (SectorId:%d)\r\n",FS_info_tag,fs_info_start);                
            #if FAT_WRITE_TBL_ID == 1  //FAT2->FAT1   

                if (FS_info_tag == 0x41615252) {
                    if ((FSI_Free_Count == Free_Count) && (FSI_Next_Free==Next_Free)) 
                    {
                        mirror_en=0;
                    }
                }
            #endif

            } else {
                fat_type=16;           
                DBG_PRINT ("FAT16\r\n");
            }
            fat_start_id[1]=fat_start_id[0]+fat_tbl_size; // 0x3AE400+0x4800=0x3B2C00
        } else {
            fat_cache_L1_en=0;
        #if FS_CACHE_DBG_EN==1
            DBG_PRINT ("Fail\r\n");
        #endif
            return -1;
        }

   #if FAT_WRITE_TBL_ID==1
        if (mirror_en) {
            SD_tbl_mirror_init(); // Mirror initial FAT1->FAT2
        } else {
            DBG_PRINT ("Tbl Mirror initial avoid\r\n"); 
        }
   #endif    
        mirror_start_id = 0x3FFFFFFF;
        mirror_end_id = 0;  // default mirror start

      #if FS_CACHE_DBG_EN==1
        DBG_PRINT ("\r\nfat_cluster_size:%d\r\n",fat_cluster_size);
        DBG_PRINT ("fat_start_id[0]:%d\r\n",fat_start_id[0]);
        DBG_PRINT ("fat_start_id[1]:%d\r\n",fat_start_id[1]);
        DBG_PRINT ("fat_tbl_size:%d\r\n",fat_tbl_size);
      #endif
        cache_init_flag = 0xA8;
    }

    sd_cache_invalid = 1;

    return 0;
}


  #if FAT_WRITE_TBL_ID==1
    INT32S SD_fsinfo_sync(void)  //  // dominant fat2->fat1
    {
        INT32U *word_buf=(INT32U *)&sd_fs_info_ram[0];
        INT32S ret;

        if (fat_type==32)
        {
            #if FS_INFO_CACHE==1
                DBG_PRINT ("FI_SYNC\r\n");
                ret=0;
            #else
                ret = drvl2_sd_read(fs_info_start, (INT32U *) &sd_fs_info_ram[0], 1);
            #endif
            
            if (ret==0) {
                FSI_Free_Count = word_buf[488/4]+BPB_Start_id;
                FSI_Next_Free = word_buf[492/4]+BPB_Start_id;
            } else {
                FSI_Free_Count = 0xFFFFFFEE;
                FSI_Next_Free = 0;
            }
            return ret;
            }
        return -1;
    }
  #endif


#if FAT_WRITE_TBL_ID==1
INT32S SD_tbl_mirror_init(void)  // dominant fat2->fat1
{
    INT32U i;
    INT32U sync_size;
    INT32U move_cnt;
    INT32U redundant_cnt;
    INT32U mirror_start_offset;
    INT32U fat0_sector_id;
    INT32U fat1_sector_id;
    INT32U fat_mirror_init_time=sw_timer_get_counter_L();

    sync_size = fat_tbl_size;
    fat0_sector_id = fat_start_id[0];
    fat1_sector_id = fat_start_id[1];
    
    move_cnt = sync_size/SDC_CACHE_SECTOR_NUMS;
    redundant_cnt = sync_size%SDC_CACHE_SECTOR_NUMS;
    mirror_start_offset = 0; 
    for (i=0;i<move_cnt;i++)
    {
        drvl2_sd_read(fat0_sector_id+SDC_CACHE_SECTOR_NUMS*i, (INT32U *) &sd_cache_ram[0] , SDC_CACHE_SECTOR_NUMS);
        drvl2_sd_write(fat1_sector_id+SDC_CACHE_SECTOR_NUMS*i, (INT32U *) &sd_cache_ram[0], SDC_CACHE_SECTOR_NUMS);
    }

    if (redundant_cnt)
    {
        drvl2_sd_read(fat0_sector_id+SDC_CACHE_SECTOR_NUMS*i, (INT32U *) &sd_cache_ram[0] , redundant_cnt);
        drvl2_sd_write(fat1_sector_id+SDC_CACHE_SECTOR_NUMS*i, (INT32U *) &sd_cache_ram[0], redundant_cnt);
    }        
    
    mirror_start_id = 0x3FFFFFFF;
    mirror_end_id = 0;  // default mirror start

    #if 1 //FS_CACHE_DBG_EN==1
        DBG_PRINT ("FAT1->FAT2 full size %d KB mirror time: %d\r\n",fat_tbl_size/2,sw_timer_get_counter_L()-fat_mirror_init_time);
    #endif

    return 0;
}
#endif



INT32S SD_tbl_mirror_sync(void)
{
  #if FS_TBL_MIRROR_SYNC==1
    INT32U i;
    INT32U sync_size;
    INT32U move_cnt;
    INT32U redundant_cnt;
    INT32U mirror_start_offset;
    INT32S ret=-1;
  #else
    INT32S ret=0;    
  #endif  

    #if FS_FLUSH_CAHCE_EN==1
        cache_sync();
    #endif
  
    #if FS_INFO_CACHE==1
        fs_info_flush();

    #endif

    #if FAT_WRITE_TBL_ID==1  // 一定要放在 fs_info_flush 之後
        SD_fsinfo_sync();
    #endif

#if FS_TBL_MIRROR_SYNC==1
    sync_size = mirror_end_id-mirror_start_id+1;
    if ((mirror_start_id!=0x3FFFFFFF) && (sync_size<=fat_tbl_size))
    {
        move_cnt = sync_size/SDC_CACHE_SECTOR_NUMS;
        redundant_cnt = sync_size%SDC_CACHE_SECTOR_NUMS;
        mirror_start_offset = mirror_start_id-fat_start_id[FAT_MAIN_TBL_ID];
        for (i=0;i<move_cnt;i++)
        {
            drvl2_sd_read(fat_start_id[FAT_MAIN_TBL_ID]+mirror_start_offset+SDC_CACHE_SECTOR_NUMS*i, (INT32U *) &sd_cache_ram[0] , SDC_CACHE_SECTOR_NUMS);
            drvl2_sd_write(fat_start_id[FAT_SECOND_TBL_ID]+mirror_start_offset+SDC_CACHE_SECTOR_NUMS*i, (INT32U *) &sd_cache_ram[0], SDC_CACHE_SECTOR_NUMS);
        }

        if (redundant_cnt)
        {
            drvl2_sd_read(fat_start_id[FAT_MAIN_TBL_ID]+mirror_start_offset+SDC_CACHE_SECTOR_NUMS*i, (INT32U *) &sd_cache_ram[0] , redundant_cnt);
            drvl2_sd_write(fat_start_id[FAT_SECOND_TBL_ID]+mirror_start_offset+SDC_CACHE_SECTOR_NUMS*i, (INT32U *) &sd_cache_ram[0], redundant_cnt);
        }        
        
        mirror_start_id = 0x3FFFFFFF;
        mirror_end_id = 0;  // default mirror start
    #if FS_CACHE_DBG_EN==1
        DBG_PRINT ("FS TBL SYNC %d Sectors [%d:%d]->[%d:%d]\r\n",sync_size,fat_start_id[FAT_MAIN_TBL_ID]+mirror_start_offset,fat_start_id[FAT_MAIN_TBL_ID]+mirror_start_offset+sync_size-1,fat_start_id[FAT_SECOND_TBL_ID]+mirror_start_offset,fat_start_id[FAT_SECOND_TBL_ID]+mirror_start_offset+sync_size-1);
    #endif
        ret = 0;      
    }
  #if FS_CACHE_DBG_EN==1
    if (ret==-1) 
    {
        DBG_PRINT ("FS TBL SYNC NONE..\r\n");
    }
  #endif

#endif

    return ret;
}

INT32S SD_Initial(void)
{
    INT32S ret;
    
//    gpio_write_io(C_DATA3_PIN, 1);
//    gpio_write_io(C_DATA1_PIN, 1);
//    gpio_write_io(C_DATA2_PIN, 1);

    ret = drvl2_sd_init();
    if (ret == 0) {
    	fat_l1_cache_init();
    }
    fs_sd_ms_plug_out_flag_reset();
	return ret; 
}

INT32S SD_Uninitial(void)
{
     drvl2_sd_card_remove();
	 return 0;
}

void SD_GetDrvInfo(struct DrvInfo* info)
{
    fat_l1_cache_init();
	info->nSectors = drvl2_sd_sector_number_get();
	info->nBytesPerSector = 512;
}

//read/write speed test function
#if (FAT_WRITE_TBL_ID==1)
static INT32U fat_tbl_tansfer(INT32U blkno)
{
    if ((blkno>=fat_start_id[0]) && (blkno<fat_start_id[0]+fat_tbl_size))
    {
        blkno += fat_tbl_size;
    }

    return blkno;
}
#endif

INT32S SD_ReadSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;

    fat_l1_cache_init();
    
    if (fs_sd_ms_plug_out_flag_get()==1) {return 0xFFFFFFFF;}

#if (FAT_WRITE_TBL_ID==1)
    blkno=fat_tbl_tansfer(blkno);
#endif


  #if FS_INFO_CACHE == 1
    if (blkcnt==1)
    {
        if(blkno==fs_info_start)
        {
            DBG_PRINT ("FI_R\r\n");
            FsCpy4((void *)buf, (void *) &sd_fs_info_ram[0], 512);
            return 0;
        }
    }
  #endif

#if FS_FLUSH_CAHCE_EN==1  //讀之前要確定 cache 已經 read well, 這部份容易有問題先不做
    if (blkcnt==1)
    {
        if (fat_cache_L1_en) 
        {
            if ((blkno>=fat_start_id[FAT_MAIN_TBL_ID]) && (blkno<fat_start_id[FAT_MAIN_TBL_ID]+fat_tbl_size))
            {
                if (cache_ref_id==0xFFFFFFFF) {
                  #if 0
                    cache_ref_id = blkno;
                    cache_read_from_nv();
                  #else
                   #if FS_CACHE_DBG_EN==1
                    DBG_PRINT("Direct-R:%d\r\n",blkno);
                   #endif               
                    goto DIRECTLY_READ;
                  #endif
                }
                
                if ((blkno>=cache_ref_id) && blkno<(cache_ref_id+SDC_CACHE_SECTOR_NUMS))
                {
                  #if FS_CACHE_DBG_EN==1
                    DBG_PRINT("Hit-R:%d from [%d,%d]\r\n",blkno,cache_ref_id,(cache_ref_id+SDC_CACHE_SECTOR_NUMS-1));
                  #endif
                    FsCpy4((void *)buf, (void *) &sd_cache_ram[512*(blkno-cache_ref_id)], 512);
                    //DMAmmCopy2( (INT32U) &sd_cache_ram[512*(blkno-cache_ref_id)],buf,  512);

                    return 0;
                }        
            }
        }
    }
DIRECTLY_READ:    
    
#endif

    
	for(i = 0; i < SD_RW_RETRY_COUNT; i++)
	{
		ret = drvl2_sd_read(blkno, (INT32U *) buf, blkcnt);
		if(ret == 0)
		{
			break;
		}
	}
  #if SUPPORT_STG_SUPER_PLUGOUT == 1
    if (ret!=0) 
    {
        //if (drvl2_sd_read(0, (INT32U *) buf, 1)!=0)
        {
            fs_sd_ms_plug_out_flag_en();
           // DBG_PRINT ("============>SUPER PLUG OUT DETECTED<===========\r\n");
        }
    }
  #endif
	return ret;
}

INT32S SD_WriteSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;
    INT32S  total_cnt;
    INT8U  miss_rate;

    fat_l1_cache_init();

    if (fs_sd_ms_plug_out_flag_get()==1) {return 0xFFFFFFFF;}

#if (FAT_WRITE_TBL_ID==1)
    blkno=fat_tbl_tansfer(blkno);
#endif

    if (blkcnt==1)
    {

        if ((blkno>=fat_start_id[FAT_MAIN_TBL_ID]) && (blkno<fat_start_id[FAT_MAIN_TBL_ID]+fat_tbl_size))
        {
            if (blkno<mirror_start_id)
            {
                mirror_start_id = blkno;
            }

            if (blkno>mirror_end_id)
            {
                mirror_end_id = blkno;
            }

      #if FS_FLUSH_CAHCE_EN==1
            if (fat_cache_L1_en) 
            {
                //fuzzy_id = blkno;
                if (cache_ref_id==0xFFFFFFFF) {
                    cache_ref_id = blkno;
                    cache_read_from_nv();
                }
                
                if ((blkno>=cache_ref_id) && blkno<(cache_ref_id+SDC_CACHE_SECTOR_NUMS))
                {
                 #if FS_CACHE_DBG_EN==1
                    DBG_PRINT("Hit-W:%d from [%d,%d]\r\n",blkno,cache_ref_id,(cache_ref_id+SDC_CACHE_SECTOR_NUMS-1));
                  #endif
                    cache_sync_flag = 0;
                    FsCpy4((void *)&sd_cache_ram[512*(blkno-cache_ref_id)],(const void *) buf, 512);
                    //DMAmmCopy2( buf, (INT32U) &sd_cache_ram[512*(blkno-cache_ref_id)], 512);
                    hit_cnt++;
                    //miss_cnt = 0;
                    return 0;
                }   
                else  
                {
                  #if FS_CACHE_DBG_EN==1
                    DBG_PRINT("Miss-W:%d out-of [%d,%d]\r\n",blkno,cache_ref_id,(cache_ref_id+SDC_CACHE_SECTOR_NUMS-1));
                  #endif
                    miss_cnt++;
                    total_cnt = hit_cnt+miss_cnt;
                    if (total_cnt >= MISS_HIT_SAMPLE_CNTS) // miss rate 取樣數, 若取樣太少就判定不及格這樣不好
                    {

                        if (cache_sync_flag==1) {
                            cache_switch_cnt++;
                        }
                        if (cache_switch_cnt>2)  // 連續3次以上做 cache sync
                        {
                            miss_thread += 5;
                            cache_switch_cnt=0;
                            DBG_PRINT ("Miss thread +5\r\n");
                            if (miss_thread>MISS_THREAD_MAX)
                            {
                                DBG_PRINT ("FAT L1 cache stop\r\n");
                                fat_cache_L1_en=0;
                            }
                        }                            

                        miss_rate = (miss_cnt*100/total_cnt);
                        if (miss_rate>miss_thread)
                        {
                            DBG_PRINT ("Miss rate:%d%\r\n",miss_rate);
                            hit_cnt = 0;
                            miss_cnt=0;
                            cache_sync();
                            cache_sync_flag=1;
                        }
                    }
                }
            }
      #endif 
        }  
      #if FS_INFO_CACHE == 1
        else if(blkno==fs_info_start)
        {
            DBG_PRINT ("FI_W\r\n");
            FsCpy4((void *)&sd_fs_info_ram[0],(const void *) buf, 512);
            return 0;
        }
      #endif
        
    }


	for(i = 0; i < SD_RW_RETRY_COUNT; i++)
	{
		ret = drvl2_sd_write(blkno, (INT32U *) buf, blkcnt);
		if(ret == 0)
		{
			break;
		}
	}
  #if SUPPORT_STG_SUPER_PLUGOUT == 1
    if (ret!=0) 
    {
        if (drvl2_sd_read(0, (INT32U *) buf, 1)!=0)
        {
            fs_sd_ms_plug_out_flag_en();
            DBG_PRINT ("============>SUPER PLUG OUT DETECTED<===========\r\n");
        }
    }
  #endif
	return ret;
}

INT32S SD_Flush(void)
{
#if FS_FLUSH_CAHCE_EN==1
    if (fat_cache_L1_en) {
        cache_sync(); 
    }
#endif
	return 0;
}

INT32S cache_read_from_nv(void)
{
#if FS_FLUSH_CAHCE_EN==1
    INT32S ret;

    if (cache_ref_id!=0xFFFFFFFF)
    {
    #if FS_CACHE_DBG_EN == 1
        DBG_PRINT ("FAT L1 read from SDC...[%d:%d]\r\n",cache_ref_id,cache_ref_id+SDC_CACHE_SECTOR_NUMS-1);
    #endif
        ret = drvl2_sd_read(cache_ref_id, (INT32U *) &sd_cache_ram[0], SDC_CACHE_SECTOR_NUMS);
        return ret;
    }

    return -1;
#endif
}

INT32S cache_sync(void)
{
#if FS_FLUSH_CAHCE_EN==1
    if (fat_cache_L1_en) 
    {
        if (cache_ref_id != 0xFFFFFFFF) 
        {
        #if FS_CACHE_DBG_EN == 1
            DBG_PRINT ("Fat-L1 write back [%d:%d]\r\n",cache_ref_id,cache_ref_id+SDC_CACHE_SECTOR_NUMS-1);
        #endif
            drvl2_sd_write(cache_ref_id, (INT32U *) &sd_cache_ram[0], SDC_CACHE_SECTOR_NUMS);
            cache_ref_id = 0xFFFFFFFF;
	return 0;
}
    }
    return -1;
#endif    
}


//=== SD 1 setting ===//
#if((SD_DUAL_SUPPORT==1)&&(MCU_VERSION>=GPL326XX_C))

INT32S SD1_Initial(void)
{
	return drvl2_sd1_init();
}

INT32S SD1_Uninitial(void)
{
     drvl2_sd1_card_remove();
	 return 0;
}

void SD1_GetDrvInfo(struct DrvInfo* info)
{
	info->nSectors = drvl2_sd1_sector_number_get();
	info->nBytesPerSector = 512;
}

//read/write speed test function
INT32S SD1_ReadSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;

    if (fs_sd_ms_plug_out_flag_get()==1) {return 0xFFFFFFFF;}
	for(i = 0; i < SD_RW_RETRY_COUNT; i++)
	{
		ret = drvl2_sd1_read(blkno, (INT32U *) buf, blkcnt);
		if(ret == 0)
		{
			break;
		}
	}
  #if SUPPORT_STG_SUPER_PLUGOUT == 1
    if (ret!=0) 
    {
        //if (drvl2_sd_read(0, (INT32U *) buf, 1)!=0)
        {
            fs_sd_ms_plug_out_flag_en();
            DBG_PRINT ("============>SUPER PLUG OUT DETECTED<===========\r\n");
        }
    }
  #endif
	return ret;
}

INT32S SD1_WriteSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;

    if (fs_sd_ms_plug_out_flag_get()==1) {return 0xFFFFFFFF;}
	for(i = 0; i < SD_RW_RETRY_COUNT; i++)
	{
		ret = drvl2_sd1_write(blkno, (INT32U *) buf, blkcnt);
		if(ret == 0)
		{
			break;
		}
	}
  #if SUPPORT_STG_SUPER_PLUGOUT == 1
    if (ret!=0) 
    {
        if (drvl2_sd1_read(0, (INT32U *) buf, 1)!=0)
        {
            fs_sd_ms_plug_out_flag_en();
            DBG_PRINT ("============>SUPER PLUG OUT DETECTED<===========\r\n");
        }
    }
  #endif
	return ret;
}

INT32S SD1_Flush(void)
{

    //DBG_PRINT ("+++++++SD1_Flush\r\n");
	return 0;
}

struct Drv_FileSystem FS_SD1_driver = {
	"SD1",
	DEVICE_READ_ALLOW|DEVICE_WRITE_ALLOW,
	SD1_Initial,
	SD1_Uninitial,
	SD1_GetDrvInfo,
	SD1_ReadSector,
	SD1_WriteSector,
	SD1_Flush,
};
#endif

#if FS_INFO_CACHE==1
INT32S fs_info_flush(void)
{
    INT32S ret=0;

    if (fat_type==32)
    {
        DBG_PRINT ("FI_FLUSH\r\n");
        ret = drvl2_sd_write(fs_info_start, (INT32U *)&sd_fs_info_ram[0], 1);
    }
    
    return ret;
}
#endif


static void FsCpy4(void *_dst, const void *_src, int len)
{
	register long *dst = (long*)_dst;
	const register long *src = (long*)_src;
	register long *end = (long*)((char*)_dst + len);
	len >>= 2;
	switch(len & 15)
	{
	case 15:
		__asm{
			LDMIA src!,{r4-r10}
			STMIA dst!,{r4-r10}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 14:
		__asm{
			LDMIA src!,{r4-r9}
			STMIA dst!,{r4-r9}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 13:
		__asm{
			LDMIA src!,{r4-r8}
			STMIA dst!,{r4-r8}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 12:
		__asm{
			LDMIA src!,{r4-r7}
			STMIA dst!,{r4-r7}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 11:
		__asm{
			LDMIA src!,{r4-r6}
			STMIA dst!,{r4-r6}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 10:
		__asm{
			LDMIA src!,{r4-r5}
			STMIA dst!,{r4-r5}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
		break;
	case 9:
		__asm{
			LDMIA src!,{r4-r12}
			STMIA dst!,{r4-r12}
		}
		break;
	case 8:
		__asm{
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}
		}
		break;
	case 7:
		__asm{
			LDMIA src!,{r4-r10}
			STMIA dst!,{r4-r10}
		}
		break;
	case 6:
		__asm{
			LDMIA src!,{r4-r9}
			STMIA dst!,{r4-r9}
		}
		break;
	case 5:
		__asm{
			LDMIA src!,{r4-r8}
			STMIA dst!,{r4-r8}
		}
		break;
	case 4:
		__asm{
			LDMIA src!,{r4-r7}
			STMIA dst!,{r4-r7}
		}
		break;
	case 3:
		__asm{
			LDMIA src!,{r4-r6}
			STMIA dst!,{r4-r6}
		}
		break;
	case 2:
		__asm{
			LDMIA src!,{r4-r5}
			STMIA dst!,{r4-r5}
		}
		break;
	case 1:
		*dst++ = *src++;
	}

	while(dst < end)
	{
		__asm{
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}
			LDMIA src!,{r4-r11}
			STMIA dst!,{r4-r11}			
		}
	}
}



//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif //(defined SD_EN) && (SD_EN == 1)                          //
//================================================================//

