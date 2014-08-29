//================Function Declaration=================
//author: erichan
//date  :2010-08-12 
//================Include Area=========================
#include "drv_l1_usbd.h"
#include "drv_l1_msdc.h"
//#include "drv_l1_usbd_tbl.h"
#include "usbdesc.h"
#include "usb.h"
#include "usbcore.h"
#include "uvc.h"
#include "usbuser.h"
#include "ap_usb.h"
#include "usbdesc.h"
//================Define Area==========================
#define USB_CAM_QUEUE_MAX_LEN    	 20
#define C_USB_CAM_STATE_STACK_SIZE   2048

#if C_UVC == CUSTOM_ON

//================Struct Area==========================
typedef struct   
{
    INT32U FrameSize;
    INT32U  AddrFrame;
    INT8U IsoSendState;
}ISOTaskMsg;
//================Function Declaration=================
INT32U  USBCamStateStack[C_USB_CAM_STATE_STACK_SIZE];
INT32S  usb_cam_task_create(INT8U pori);
INT32S usb_cam_task_del(void);
void *USBCamApQ_Stack[USB_CAM_QUEUE_MAX_LEN];
void *iso_task_Q_Stack[USB_CAM_QUEUE_MAX_LEN];
void SendOneFrameTaskEntry(void *param);
//=====================================================
OS_EVENT  *USBCamApQ;
//=====================================================
extern OS_EVENT *iso_task_q;
extern OS_EVENT  *usbwebcam_frame_q;
extern BOOL SendISOData;
//=====================================================
void usb_cam_entrance(INT16U usetask)
{
	INT32S nRet;	
	INT32U tmp;
    
	UVC_Delay = 0;
	TestCnt = 0;
	JPG_Cnt = 0;
	PTS_Value = 0;
	PicsToggle = 0;
    tmp = (INT32U)gp_malloc_align(512,16);
	SOF_Event_Buf =(char *) tmp;
	DBG_PRINT("usb_initial start \r\n");
    usb_initial();
	usbd_desc_jpg_size_set(1280, 480);
    DBG_PRINT("usb_initial end \r\n");
    vic_irq_register(VIC_USB, usb_isr);//usb_isr);
	vic_irq_enable(VIC_USB);
	nRet = usb_cam_task_create(USB_CAM_PRIORITY);
    if(nRet < 0)
    	DBG_PRINT("usb_cam_task_create fail !!!");
}
void usb_cam_exit(void)
{
	gp_free((void *) SOF_Event_Buf);
	usb_cam_task_del();
}

//=====================================================
INT32S usb_cam_task_create(INT8U pori)
{
	INT8U err;

	//creat usb cam message
	USBCamApQ = OSQCreate(USBCamApQ_Stack, USB_CAM_QUEUE_MAX_LEN);
    if(!USBCamApQ) 
     return STATUS_FAIL;
    //creat usb cam task
    err = OSTaskCreate(SendOneFrameTaskEntry,  (void*)NULL,&USBCamStateStack[C_USB_CAM_STATE_STACK_SIZE - 1], pori);
	if(err != OS_NO_ERR) {
	    return STATUS_FAIL;
    }
	return STATUS_OK;
}
INT32S usb_cam_task_del(void)
{
	INT8U   err;
  
	OSTaskDel(USB_CAM_PRIORITY);
	OSQFlush(USBCamApQ);
	OSQDel(USBCamApQ, OS_DEL_ALWAYS, &err);
	
	return STATUS_OK;
}

extern BOOL Drv_USBD_ISOI_DMA_START(char *buf,INT16U ln);
extern BOOL Drv_USBD_ISOI_DMA_Finish(void);
extern INT8U s_usbd_pin;
//=====================================================
void SendOneFrameTaskEntry(void *param)
{
    BOOL sendok = 0;
	INT8U err;
    ISOTaskMsg  *isosend0;
    BOOL ret;
    INT32U  addr;
	INT32U  len;
	
    SendISOData = 0;
     
    while(1) {
	    isosend0 = (ISOTaskMsg *)OSQPend(USBCamApQ, 0, &err);//if encode one frame end
		sendok = 0;
		addr = isosend0->AddrFrame;
		len = isosend0->FrameSize;
		
		if(!s_usbd_pin) {
		    SendISOData = 0;
		}
		
		if(SendISOData) { 
		    while(!sendok) {
			    sendok = USB_SOF_Event(len,addr);
                Drv_USBD_ISOI_DMA_START((char*)SOF_Event_Buf,512);
			    ret = Drv_USBD_ISOI_DMA_Finish();
			    if(ret==FALSE||(!SendISOData)) {
			        break;
			    }         
	        }
	    }
	    else {
	        *P_USBD_FUNCTION &= ~BIT9 ;    // Disable DMA Bulk in
	    }
	    OSQPost(usbwebcam_frame_q, (void *)isosend0->AddrFrame);
    }
}
/////////////////////////////////////////////////////////////////////////////////
#endif