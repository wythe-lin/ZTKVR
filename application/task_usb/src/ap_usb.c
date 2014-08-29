#include "ap_usb.h"
#define DEVICE_READ_ALLOW	0x1
#define DEVICE_WRITE_ALLOW	0x2

//#define MASK_CMD7		            (0x00000007|0x7000|0x0080)	// SELECT/DESELECT_CARD ?

#define LUNTYPE_NF 	        FS_NAND1
#define LUNTYPE_MS   		FS_MS
#define LUNTYPE_SDCMMC	 	FS_SD
#define LUNTYPE_CF	        FS_CF
#define LUNTYPE_XD	        FS_XD
#define LUNTYPE_NF1			FS_NAND2
#define LUNTYPE_CDROM_NF    FS_CDROM_NF
#define LUNTYPE_SPI         FS_SPI
#define LUNTYPE_CDROM_SPI   FS_CDROM_SPI
#define RWSECTOR_512B     1
#define RWSECTOR_1024B    2
#define RWSECTOR_2048B    4
#define RWSECTOR_4096B    8
#define RWSECTOR_8192B    16
#define RWSECTOR_16384B   32
#define READWRITE_SECTOR   RWSECTOR_8192B
////////////////////////str_USB_Lun_Info->unStatus//////////////////////////////////
#define LUNSTS_NORMAL			0x0000
#define LUNSTS_NOMEDIA			0x0001
#define LUNSTS_WRITEPROTECT		0x0002
#define LUNSTS_MEDIACHANGE		0x0004

//#define C_USBDEVICE_PIN   82//IOF[2] = BKCS2, usb device detect pin


struct DrvInfo2 {
	INT32U nSectors;
	INT32U nBytesPerSector;
};
struct Drv_FileSystem2 {
	INT8U 	Name[8];
	INT8U 	Status;
    INT32S	(*Drv_Initial)(void);
    INT32S	(*Drv_Uninitial)(void);
    void 	(*Drv_GetDrvInfo)(struct DrvInfo2* info);
    INT32S	(*Drv_ReadSector)(INT32U, INT32U, INT32U);
    INT32S	(*Drv_WriteSector)(INT32U,INT32U, INT32U);
};
extern INT8U DelOrUnplug;
#if (MCU_VERSION == GPL32_B)
INT16U USBRWError_Flag = 0;             // B IC 
extern INT32U usbd_check_bi_dma();
#endif
int   R_Write_protect;
//LUN
INT16U usbd_current_lun;

INT16U usb_storage_curent[8];
INT8U *p_usbd_user_string_buffer=NULL;
INT16U *BUF=NULL; //A+B

static INT32S ConfirmPass ;
extern str_USB_Lun_Info 	USB_Lun_Define[];
extern struct Drv_FileSystem2   *FileSysDrv[];
extern struct Usb_Storage_Access *UsbReadWrite[];


INT32S Read_10(str_USB_Lun_Info* lUSB_LUN_Read);
INT32S Write_10(str_USB_Lun_Info* lUSB_LUN_Read);
void   usbdsuspend_callback(void);
INT32U usbdvendor_callback(void);
INT32U ep0_datapointer_callback(INT32U nTableType);
BOOLEAN usbd_pin_detection(void);
INT32U usbd_wpb_detection(void);
void ap_usb_pc_connect_show(void);
void usb_ppu_display(void);
///INT32U usbd_storage_detection(void);

INT32U usbd_msdc_lun_change(str_USB_Lun_Info* pNEW_LUN);

void usb_nothing()
{

}
void usb_msdc_state_exit(void)
{
	if (BUF != NULL) {
		gp_free((void *) BUF);
	}
}

void usb_msdc_state_initial(void)
{

  INT16U i,PinID,Luns ;
  INT16S ret;
  struct DrvInfo2 inf;
  
  BUF= (INT16U *) gp_malloc_align(1024* READWRITE_SECTOR, 16);
  if (BUF == NULL) {
  	DBG_PRINT("msdc AB buffer allocate fail\r\n");
  	return;
  }
  
  usbd_current_lun =0;
  for(i =0 ; i<6 ; i++)
    usb_storage_curent[i] = storage_detection(i);
  //polling storages........
  for(i =0 ; i<6 ; i++)
      	USB_Lun_Define[i].unStatus = LUNSTS_NOMEDIA;
 //Set Luns
//  Luns =1;
//  SetLuntpye(0,LUNTYPE_MS);
  // modified by Bruce, 2009/02/24
  Luns =1;

#if MINI_DVR_STORAGE_TYPE == T_FLASH
  SetLuntpye(0,LUNTYPE_SDCMMC);
#else
  SetLuntpye(0,LUNTYPE_NF);
#endif

//  SetLuntpye(1,LUNTYPE_NF);

  usbd_setlun(Luns-1);
  for (i = 0 ; i<Luns; i++)  //x Luns
  {
     switch (USB_Lun_Define[i].unLunType)
     {
      case LUNTYPE_NF:
            PinID =5;
	        break;
      case LUNTYPE_MS:
            PinID =2;
    	    break;
      case  LUNTYPE_SDCMMC:
            PinID =1;
       		break;
      case  LUNTYPE_CF:
            PinID =0;
       		break;
      case  LUNTYPE_XD:
            PinID =6;
       		break;
      case LUNTYPE_NF1:
      		PinID = 5;
      		break; 	
    //  case LUNTYPE_CDROM_NF:
      //      PinID = 5;		//add by erichan for nand cdrom 20091031
        //    break;
//      case LUNTYPE_SPI:
      //      PinID = 8;      
    //        break;
   //   case LUNTYPE_CDROM_SPI:
    //        PinID = 8;
    //        break;      
      default:
       		break;
     }
     if (1)
     {
		ret = FileSysDrv[USB_Lun_Define[i].unLunType]->Drv_Initial();
		FileSysDrv[USB_Lun_Define[i].unLunType]->Drv_GetDrvInfo(&inf);
		//FileSysDrv[USB_Lun_Define[i].unLunType]->Drv_Uninitial(); //wwj mark
		if (!ret)
		{
			USB_Lun_Define[i].ulSecSize = (INT32U)inf.nSectors;
			USB_Lun_Define[i].unStatus = LUNSTS_NORMAL;
		}
		else
		{
			USB_Lun_Define[i].ulSecSize =0;
			USB_Lun_Define[i].unStatus = LUNSTS_NOMEDIA;
		}
     }
  }

  //initial current Luns
  if (USB_Lun_Define[usbd_current_lun].unStatus == LUNSTS_NOMEDIA)
  {
		for (i =0 ;i <6 ; i++)
		{
			if (USB_Lun_Define[i].unStatus == LUNSTS_NORMAL)
			{
				FileSysDrv[USB_Lun_Define[i].unLunType]->Drv_Initial();
				usbd_current_lun =i ;
				break;
			}
		}
  }
  else
  {
       //FileSysDrv[USB_Lun_Define[usbd_current_lun].unLunType]->Drv_Initial(); //wwj mark
  }
  //callback function registers;
  usbd_write10_register(&Write_10);
  usbd_read10_register(&Read_10);
  usbd_suspend_register(&usbdsuspend_callback);
  usbd_ep0descriptor_register(&ep0_datapointer_callback);
  usbd_writeprotected_register(&usbd_wpb_detection);
  usbd_vendorcallback_register(&usbdvendor_callback);
  
/*  #if CDSPI_EN  
  usbd_CDROM_ReadSector_register(&SPIFCDROM_ReadSector);//erichan for spi cdrom 091111   
  #endif
  #if CDNF_EN
  usbd_CDROM_ReadSector_register(&NFCDROM_ReadSector);//erichan for spi cdrom 091111   
  #endif*/
//  #if CDROM_EN  
//  usbd_CDROM_ReadSector_register(&CDROM_ReadSector);//huanghe for cdrom 090605  
//#endif

#if (MCU_VERSION == GPL32_B)
   USBRWError_Flag = 0;            // for B IC 
#endif   
  //IOG[6] WPB initial

  //uninitail uhost
  //USB_Host_Disable();
  //usbd initial
 // usb_reset();

  usb_initial_udisk();

  //user string     
   

}

void usb_msdc_state(void)
{
 INT32U plugcount = 0x2ff000;
 while(1)
 {
   //cmd loop
   usb_isr();
   usb_std_service_udisk(0);
   usb_msdc_service(0);

#if (MCU_VERSION == GPL32_B)   
   if (USBRWError_Flag == 0xfefe)      // B IC 
   {
           break;
   }
#endif   
   //pollingExit:
  //if (!card_detection(4))
  if (!storage_detection(4)) 
  {
       DelOrUnplug = 1;
       break;
  }
   // check if usb is deleted by safe delete
  if(plugcount>0)
     plugcount--;
  else
    {
     if(DelOrUnplug == 1)
       break;
     else
      plugcount = 0x1000;
      
     } 
     
 }
}

void usb_webcam_state(void)
{
INT32U plugcount = 0x4ff000;
 while(1)
 {
   //cmd loop
 usb_std_service(0);

#if (MCU_VERSION == GPL32_B)   
   if (USBRWError_Flag == 0xfefe)      // B IC 
   {
           break;
   }   
#endif 
  
   //pollingExit://usb detect pin check to not usb plug
//  if (!card_detection(4))
 //    break;
    if(!usbd_pin_detection())
   // if(!card_detection(4))
    {
       DelOrUnplug = 1;
       break;
       }
  // check if usb is deleted by safe delete
  if(plugcount>0)
     plugcount--;
  else
    {
     if(DelOrUnplug == 1)
       break;
     else
      plugcount = 0x1000;
      
     } 
     
 }

}

INT32U usbd_check_lun(DRV_PLUG_STATUS_ST * pStorage)
{
 INT8U change_lun_type =0xff;
 INT8U change_lun_id     =0xff;
 INT16S i,ret;
 //INT32U test;
 struct DrvInfo2 inf;
 if (pStorage->storage_id == DRV_FS_DEV_USBD)
 {
   if (pStorage->plug_status == DRV_PLGU_OUT)
   {
      FileSysDrv[USB_Lun_Define[usbd_current_lun].unLunType]->Drv_Uninitial();
      return 1;
   }
 }
 else
 {
  	 if (pStorage->storage_id == DRV_FS_DEV_SDMS)
  	 {
  	   storage_sdms_detection();
  	   if (usb_storage_curent[1] != storage_detection(1))
  	   {
  	     usb_storage_curent[1] = storage_detection(1);
  	     change_lun_type =LUNTYPE_SDCMMC;
  	   }
#if (MSC_EN==1)
  	   else if (usb_storage_curent[2] != storage_detection(2))
  	   {
   	     usb_storage_curent[2] = storage_detection(2);
 	      change_lun_type =LUNTYPE_MS;
  	   }
#endif
#if (XD_EN==1)
  	   else if (usb_storage_curent[6] != storage_detection(6))
  	   {
   	      usb_storage_curent[6] = storage_detection(6);
 	      change_lun_type =LUNTYPE_XD;
  	   }
#endif
       else
  	   {
  	     return 0;
  	   }
  	 }
#if (CFC_EN==1)
  	 else if (pStorage->storage_id == DRV_FS_DEV_CF)
  	 {
  	   storage_cfc_detection();
  	   change_lun_type =LUNTYPE_CF;
  	 }
#endif
#if USB_NUM >= 1
  	 else if (pStorage->storage_id == DRV_FS_DEV_USBH)
  	 {
  	   //When UHOST plug in then STOP usb state aand exit
 	   if (pStorage->plug_status == DRV_PLGU_IN)
 	   {
 	       FileSysDrv[USB_Lun_Define[usbd_current_lun].unLunType]->Drv_Uninitial();
           return 1;
 	   }
  	 }
#endif
     else
  	 {
	   return 0;
  	 }

     for (i=0 ; i<6; i++)
     {
     	if (USB_Lun_Define[i].unLunType == change_lun_type )
       		change_lun_id = i;
   	 }
     if (pStorage->plug_status == DRV_PLGU_OUT)
     {
   //unmount
     	USB_Lun_Define[change_lun_id].unStatus =LUNSTS_NOMEDIA | LUNSTS_MEDIACHANGE;
     	USB_Lun_Define[change_lun_id].ulSecSize =0;
     	FileSysDrv[USB_Lun_Define[change_lun_id].unLunType]->Drv_Uninitial();
  	 }
   	 else
     {
   //1 unmount current
        FileSysDrv[USB_Lun_Define[usbd_current_lun].unLunType]->Drv_Uninitial();
   //2 mount and check plugin storage
        #if (CFC_EN == 1)
        if (change_lun_type ==LUNTYPE_CF)
        {storages_cfc_reset();}
        #endif
     	ret =FileSysDrv[USB_Lun_Define[change_lun_id].unLunType]->Drv_Initial();
     	if (!ret)
     	{
       		FileSysDrv[USB_Lun_Define[change_lun_id].unLunType]->Drv_GetDrvInfo(&inf);
    		USB_Lun_Define[change_lun_id].ulSecSize =(INT32U)inf.nSectors;
	    	//DBG_PRINT("plug in %x \r\n", inf->nSectors);
       		//USB_Lun_Define[change_lun_id].unStatus =LUNSTS_NORMAL | LUNSTS_MEDIACHANGE;
     	}
     	else
     	{
     	   //DBG_PRINT("plug X %x \r\n",change_lun_id);
       		//USB_Lun_Define[change_lun_id].unStatus =LUNSTS_NOMEDIA ;
       		USB_Lun_Define[change_lun_id].ulSecSize =0;
     	}
     //3 unmount check plugin storage
     	FileSysDrv[USB_Lun_Define[change_lun_id].unLunType]->Drv_Uninitial();
     //4 mount current
        FileSysDrv[USB_Lun_Define[usbd_current_lun].unLunType]->Drv_Initial();
       	if (!ret)
     	{
       		USB_Lun_Define[change_lun_id].unStatus =LUNSTS_NORMAL | LUNSTS_MEDIACHANGE;
     	}
     	else
     	{
       		USB_Lun_Define[change_lun_id].unStatus =LUNSTS_NOMEDIA ;
     	}

   	 }
 }
 return 0;
}

#if 1
//IOF[2] as usb pin
//usbd detect 1:plug 0:unplug
BOOLEAN usbd_pin_detection(void)
{
  BOOLEAN stat;
  stat = gpio_read_io(C_USBDEVICE_PIN);
  return stat; 
}
void usbd_pin_detection_init(void)
{
#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_EMU_V2_0
	if (C_USBDEVICE_PIN == IO_F3) {
		gpio_set_memcs(3,0);
	}
#endif
  	gpio_init_io(C_USBDEVICE_PIN,GPIO_INPUT);
  	gpio_set_port_attribute(C_USBDEVICE_PIN, INPUT_WITH_RESISTOR);
  	gpio_write_io(C_USBDEVICE_PIN,DATA_LOW);  	
}
#endif

INT32U usbd_wpb_detection(void)
{
 //if ( gpio_read_io(C_USB_WP_D_PIN) )
 if (storage_wpb_detection())
 {
    USB_Lun_Define[usbd_current_lun].unStatus &= ~LUNSTS_WRITEPROTECT;
	return 0;
 }
 else
 {
	USB_Lun_Define[usbd_current_lun].unStatus |=  LUNSTS_WRITEPROTECT;
    return 1;
  }
}

INT32U usbd_msdc_lun_change(str_USB_Lun_Info* pNEW_LUN)
{
  INT32U ret ;
  if (pNEW_LUN->unStatus ==LUNSTS_NOMEDIA)
   return 1;
  DBG_PRINT("Lun %x ->%x \r\n", usbd_current_lun, pNEW_LUN->unLunNum);
  //umount current
  FileSysDrv[USB_Lun_Define[usbd_current_lun].unLunType]->Drv_Uninitial();
  //mount new storage
  ret =FileSysDrv[pNEW_LUN->unLunType]->Drv_Initial();
  if(ret)
  {
	   USB_Lun_Define[pNEW_LUN->unLunType].unStatus =LUNSTS_NOMEDIA ;
	   FileSysDrv[pNEW_LUN->unLunType]->Drv_Uninitial();
  }
  return ret;
}

//====================================================================================================
//	Description:	This function will be called if PC run to be suspend or device be plug out.
//	Function:		usbdvendor_callback()
//	Syntax:			void usbdvendor_callback(void)
//	Input Paramter:	None
//	Return: 		None
//===================================================================================================
void usb_time_set(void)
{
	TIME_T	g_time;
	
	g_time.tm_year = (GetCMDValueEx(18)<<8) | GetCMDValueEx(19);
	g_time.tm_mon = GetCMDValueEx(20);
	g_time.tm_mday = GetCMDValueEx(21);
	g_time.tm_hour = GetCMDValueEx(22);
	g_time.tm_min = GetCMDValueEx(23);
	g_time.tm_sec = GetCMDValueEx(24);
	cal_time_set(g_time);	
}
INT32U usbdvendor_callback(void)
{
 	int i;
	unsigned int   Command_Value;
	
	if (GetCMDValueEx(29)!='G' || GetCMDValueEx(30)!='P')
	{
		CommandFail(0x27);
		return 0;
	}
	Command_Value = (GetCMDValueEx(15) << 8  | GetCMDValueEx(16) );
	
    Sense_Code =0x1a;
	
	switch (Command_Value )
	{
	 	case 0xF000:
 		    return 0;
		case 0xF001:  //get lun
			GetDriveStatus();
			return 0;

		case 0xF010:				//Read IC info
			GetICVersion();
			return 0;
		case 0xFFF0:
		case 0xF0F0:
			 SetVenderID();
			 ConfirmPass = 1;
			return 0;
		case 0xFFEE:
			if (i == GetCMDValueEx(25))
				ConfirmPass = 1;		//backdoor -_-|||
		    return 0;
    }
    if (ConfirmPass == 0) 	CommandFail(0x1A);
	
    switch (Command_Value )
	{
		case 0xF0FF:
			usb_time_set();
			CSW_Residue=0;
			break;
		default:
			CommandFail(0x1A);
			break;
	}

	return 0;
}
//====================================================================================================
//	Description:	This function will be called if PC run to be suspend or device be plug out.
//	Function:		USBDSuspend_CallBack()
//	Syntax:			void USBSuspend_CallBack(void)
//	Input Paramter:	None
//	Return: 		None
//===================================================================================================
void usbdsuspend_callback(void)
{
	//set R_USB_Suspend=1 will break out usb service loop.
	R_USB_Suspend=1;
	DelOrUnplug = 1;
}

//====================================================================================================
//	Description:	This function will be called if PC run to be suspend or device be plug out.
//	Function:		EP0_DataPointer_CallBack()
//	Syntax:			int EP0_DataPointer_CallBack(int nTableType)
//	Input Paramter:	Descriptor address
//	Return: 		1: if you want to redirect R_Descriptor_High and R_Descriptor_Low address.
//					0: use default reply info.
//====================================================================================================
INT32U ep0_datapointer_callback(INT32U nTableType)
{
	return 0;
}

//unsigned int ModeSense6_CallBack(void)
//{
//	return 0;
//
//////////////////////////////////////////////////////////////
INT32S Read_10(str_USB_Lun_Info* lUSB_LUN_Read)
{
    INT16U  i=0,j=0,stage =0;
    INT32S  ret;
    INT32U  SCSI_LBA,SCSI_Transfer_Length;
    INT32U  AB_Counts,Ren_Sectors;
    INT16U* USB_RW_Buffer_PTR_A;
    INT16U* USB_RW_Buffer_PTR_B;
    INT16U* USB_RW_Buffer_PTR_Temp;
    //Checking curent LUN
    
    if (usbd_current_lun != lUSB_LUN_Read->unLunNum)
    {
      if (usbd_msdc_lun_change(lUSB_LUN_Read))
      {
            //DBG_PRINT("LUN FAIL\r\n");
   			Sense_Code = 0x1B;
			return -1;
      }
      usbd_current_lun = lUSB_LUN_Read->unLunNum;
    }
    stage =0;
	SCSI_LBA=GetLBA();
	SCSI_Transfer_Length=GetTransLeng();
	USB_RW_Buffer_PTR_A=(INT16U*) BUF ;
	USB_RW_Buffer_PTR_B=USB_RW_Buffer_PTR_A+ (256*READWRITE_SECTOR);
	if(SCSI_LBA == 0x3ffd)
	SCSI_LBA= 0x3ffd;

	//DBG_PRINT("READ LBA = %x ,len = %x\r\n",SCSI_LBA,SCSI_Transfer_Length);
	if ((SCSI_LBA + SCSI_Transfer_Length)>lUSB_LUN_Read->ulSecSize)
	{
			Sense_Code = 0x1B;
			return -1;
	}
    
	AB_Counts   = SCSI_Transfer_Length / READWRITE_SECTOR;
	Ren_Sectors = SCSI_Transfer_Length % READWRITE_SECTOR;	
	
	// #if CDNF_EN|CDSPI_EN      //add by erichan 20091031   for nandcdrom      
/*	 #if CDNF_EN      //add by erichan 20091031   for nandcdrom            
	 if(lUSB_LUN_Read->unLunType == LUNTYPE_CDROM_NF)
	// if((lUSB_LUN_Read->unLunType == LUNTYPE_CDROM_NF)||(lUSB_LUN_Read->unLunType == LUNTYPE_CDROM_SPI))	//CDROM, do not modify below CDROM code
	 {
	 
	  SCSI_LBA ++;
	  SCSI_LBA = SCSI_LBA << 2;						//1 sector = 4 *512
	  AB_Counts = AB_Counts <<2;
	  }
	#endif
*/			
    //ret = UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_cmd_phase(SCSI_LBA,SCSI_Transfer_Length); del by erichan
    ret = UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_cmd_phase(SCSI_LBA,SCSI_Transfer_Length); 
	if(ret != 0)
	{
  	    //DBG_PRINT("e1\r\n");
    	CSW_Residue=SCSI_Transfer_Length;
		CSW_Residue=CSW_Residue << 9;
		Sense_Code=0x12;
		return -1;
	}
	for(i=0 ; i < AB_Counts ; i++)
	{
            if (i == 0)
              ret = UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_dma_phase((INT32U*)USB_RW_Buffer_PTR_A,1,READWRITE_SECTOR);
            else
              ret = UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_dma_phase((INT32U*)USB_RW_Buffer_PTR_A,0,READWRITE_SECTOR);
            //if(ret != 0)  {	DBG_PRINT("e5 %x \r\n",i);break; }
			if (ret != 0)	 break;

            if (i != 0)
              ret = UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_dma_phase(NULL,2,READWRITE_SECTOR); //wait and check storage DMA
            if (stage)
			{
				ret = Send_To_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_A,READWRITE_SECTOR,1);
				//Check if timeout
				//if(ret != 0)  {	DBG_PRINT("e1\r\n");break; }
				if(ret != 0)  break;
			}
        	//if(ret != 0)  {	DBG_PRINT("e2\r\n");break; }
			if(ret != 0)  break;
  			//if (i == AB_Counts-1)
            //   UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_cmdend_phase();//Storage end

			ret = Send_To_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_A,READWRITE_SECTOR,0);
		    //if(ret != 0)  {	DBG_PRINT("e3\r\n");break; }
			if(ret != 0)  break;
			stage=1;
			//for last data
			if (i == AB_Counts-1)
			{
				ret = Send_To_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_A,READWRITE_SECTOR,1);
			    //if(ret != 0)  {	DBG_PRINT("e4\r\n");break; }
				if(ret != 0)  break;
			}
			USB_RW_Buffer_PTR_Temp = USB_RW_Buffer_PTR_B;
			USB_RW_Buffer_PTR_B = USB_RW_Buffer_PTR_A;
			USB_RW_Buffer_PTR_A = USB_RW_Buffer_PTR_Temp;

	}//end for
	
	if( (ret == 0x00) && (Ren_Sectors> 0))
	{
    	  for(j=0 ; j < Ren_Sectors ; j++)
     	  {
            if (j == 0)
              ret = UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_dma_phase((INT32U*)USB_RW_Buffer_PTR_A,1,1);
            else
              ret = UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_dma_phase((INT32U*)USB_RW_Buffer_PTR_A,0,1);
            //if(ret != 0)  {	DBG_PRINT("e5 %x \r\n",i);break; }
			if (ret != 0)	 break;
            if (j != 0)
              ret = UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_dma_phase(NULL,2,1); //wait and check storage DMA
            if (stage)
			{
				ret = Send_To_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_A,1,1);
				//Check if timeout
				//if(ret != 0)  {	DBG_PRINT("e1\r\n");break; }
				if(ret != 0)  break;
			}
        	//if(ret != 0)  {	DBG_PRINT("e2\r\n");break; }
			if(ret != 0)  break;
  			ret = Send_To_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_A,1,0);
		    //if(ret != 0)  {	DBG_PRINT("e3\r\n");break; }
			if(ret != 0)  break;
			stage=1;
			//for last data
			if (j == Ren_Sectors-1)
			{
				ret = Send_To_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_A,1,1);
			    //if(ret != 0)  {	DBG_PRINT("e4\r\n");break; }
				if(ret != 0)  break;
			}
			USB_RW_Buffer_PTR_Temp = USB_RW_Buffer_PTR_B;
			USB_RW_Buffer_PTR_B = USB_RW_Buffer_PTR_A;
			USB_RW_Buffer_PTR_A = USB_RW_Buffer_PTR_Temp;
		 }//end for	
	}
    UsbReadWrite[lUSB_LUN_Read->unLunType]->usdc_read_cmdend_phase();//Storage end
	
	if(ret != 0x00)
	{
       	    //DBG_PRINT("READ10 %x %x Fail!\r\n",i,SCSI_Transfer_Length);
			CSW_Residue=SCSI_Transfer_Length-(i*READWRITE_SECTOR)-j;
			CSW_Residue=CSW_Residue << 9;
			Sense_Code = 0x12;
#if (MCU_VERSION == GPL32_B)			
            if usbd_check_bi_dma()
            {
                 USBRWError_Flag = 0xfefe; 
                 return 1;
            }
#endif
	}
	else
			CSW_Residue=0;
			    		
	return 0;
}

INT32S Write_10(str_USB_Lun_Info* lUSB_LUN_Write)
{
   INT32S  i=0,j=0,ret,ret1,stage;
   INT32U  SCSI_LBA,SCSI_Transfer_Length;
   INT32U  AB_Counts,Ren_Sectors;
   INT16U* USB_RW_Buffer_PTR_A;
   INT16U* USB_RW_Buffer_PTR_B;
   INT16U* USB_RW_Buffer_PTR_Temp;
   
   if (usbd_current_lun != lUSB_LUN_Write->unLunNum)
   {
      if (usbd_msdc_lun_change(lUSB_LUN_Write))
      {
   			Sense_Code = 0x1B;
			return -1;
      }
      usbd_current_lun = lUSB_LUN_Write->unLunNum;
   }
   //if (usbd_wpb_detection())   { }
   stage =0;
   SCSI_LBA=GetLBA();
   SCSI_Transfer_Length=GetTransLeng();
   USB_RW_Buffer_PTR_A=(INT16U*) BUF;
   USB_RW_Buffer_PTR_B=USB_RW_Buffer_PTR_A+ (256*READWRITE_SECTOR);
   //DBG_PRINT("W10 %x %x\r\n",SCSI_LBA,SCSI_Transfer_Length);
   if ((SCSI_LBA + SCSI_Transfer_Length)>lUSB_LUN_Write->ulSecSize)
   {
            UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_cmdend_phase();
			Sense_Code = 0x1B; //LOGICAL BLOCK ADDRESS OUT OF RANGE		
			return 1;
   }
   
 /*  #if CDROM_EN

	if(lUSB_LUN_Write->unLunType == LUNTYPE_CDROM)
	{
		
		
		for(i=0 ; i < SCSI_Transfer_Length ; i++)
	{
	//	Temp_buf = CDROM_BUF+(SCSI_LBA*512+i*512)/2;
	//	memcpy(USB_RW_Buffer_PTR_A,(INT16U*)Temp_buf,512);
	//	USB_RW_Buffer_PTR_A = CDROM_BUF + (SCSI_LBA*512+i*512)/2;
	//	memcpy(USB_RW_Buffer_PTR_A,(INT16U*)(CDROM_BUF+SCSI_LBA*512+i*512),512);
	
		ret = Receive_From_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_A,READWRITE_SECTOR,0);
		ret = Receive_From_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_A,READWRITE_SECTOR,1);
	
		ret = CDROM_WriteSector(SCSI_LBA+i,1,(INT32U)USB_RW_Buffer_PTR_A);

	}
	
	}
	else//LUNTYPE_CDROM cdrom
#endif
*/
   {
   AB_Counts   = SCSI_Transfer_Length / READWRITE_SECTOR;
   Ren_Sectors = SCSI_Transfer_Length % READWRITE_SECTOR;				   
   ret = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_cmd_phase(SCSI_LBA,SCSI_Transfer_Length);
   if(ret != 0)
   {
   //DBG_PRINT("E1\r\n");
        UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_cmdend_phase();
    	CSW_Residue=SCSI_Transfer_Length;
		CSW_Residue=CSW_Residue << 9;
		Sense_Code=0x10;
		return -1;
   }
   
   ret=Receive_From_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_B,READWRITE_SECTOR,0);
   for(i=0; j<AB_Counts; i++)
   {
			//=================================================================================================
		    if (i != 0)
		       ret = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_dma_phase(NULL,2,READWRITE_SECTOR);
            //if(ret != 0)    DBG_PRINT("E3\r\n");
            if(ret != 0) break;
		    ret=Receive_From_USB_DMA_USB(NULL,READWRITE_SECTOR,1);
			if(ret != 0) break;
			
            ret = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_dma_phase((INT32U*)USB_RW_Buffer_PTR_B,0,READWRITE_SECTOR);
            //if(ret != 0)    DBG_PRINT("E4\r\n");
            if(ret != 0) break;

			//for last data
			if (i == AB_Counts-1)
			{
		       ret = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_dma_phase(NULL,2,READWRITE_SECTOR);
              //if(ret != 0)    DBG_PRINT("E5\r\n");
		       break;
			}
			USB_RW_Buffer_PTR_Temp = USB_RW_Buffer_PTR_B;
			USB_RW_Buffer_PTR_B = USB_RW_Buffer_PTR_A;
			USB_RW_Buffer_PTR_A = USB_RW_Buffer_PTR_Temp;
		    ret=Receive_From_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_B,READWRITE_SECTOR,0);
		    //ret1 = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_cmdend_phase();
	}//j
   	if( (ret == 0x00)  && (Ren_Sectors> 0))
	{   
			ret=Receive_From_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_B,1,0);
		   	for(j=0; j<Ren_Sectors; j++)
			{
			//=================================================================================================
			    if (j != 0)
			       ret = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_dma_phase(NULL,2,1);
	            //if(ret != 0)    DBG_PRINT("E3\r\n");
        	    if(ret != 0) break;
		    		ret=Receive_From_USB_DMA_USB(NULL,1,1);
				if(ret != 0) break;
			
    	        ret = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_dma_phase((INT32U*)USB_RW_Buffer_PTR_B,0,1);
        	    //if(ret != 0)    DBG_PRINT("E4\r\n");
            	if(ret != 0) break;

			//for last data
				if (j == Ren_Sectors-1)
				{
			       ret = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_dma_phase(NULL,2,1);
            		  //if(ret != 0)    DBG_PRINT("E5\r\n");
			       break;
				}
				USB_RW_Buffer_PTR_Temp = USB_RW_Buffer_PTR_B;
				USB_RW_Buffer_PTR_B = USB_RW_Buffer_PTR_A;
				USB_RW_Buffer_PTR_A = USB_RW_Buffer_PTR_Temp;
			    ret=Receive_From_USB_DMA_USB((INT32U*)USB_RW_Buffer_PTR_B,1,0);
	    	}//j
	        //ret1 = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_cmdend_phase();
	}	
    ret1 = UsbReadWrite[lUSB_LUN_Write->unLunType]->usdc_write_cmdend_phase();
   //if(ret != 0)    DBG_PRINT("E6\r\n");
	if( (ret != 0x00) || (ret1 != 0x00) )
	{
			CSW_Residue=SCSI_Transfer_Length-(i*READWRITE_SECTOR)-j-1;
			CSW_Residue=CSW_Residue << 9;
		    Sense_Code=0x10;
            //DBG_PRINT("Write10 %x %x Fail!\r\n",j,SCSI_Transfer_Length);
	}
	else
 		CSW_Residue=0;
	return 0;
	}
}
