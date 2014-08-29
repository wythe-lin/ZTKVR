
/* pseudo header include */
#include "state_usb.h"

#define USB_TASK_QUEUE_MAX	32

OS_EVENT *USBAPPTaskQ;
void *usb_task_q_stack[USB_TASK_QUEUE_MAX];
INT8U usbd_flag;
extern INT8U s_usbd_pin;

//void state_usb_entry(void* para1);
static void state_usb_init(void);
//static void state_usb_exit(void);
//static void state_usb_suspend(EXIT_FLAG_ENUM exit_flag);
extern void usb_msdc_state_exit(void);
void sys_usbd_state_flag_set(INT8U in_or_out);


void state_usb_init(void)
{
    sys_usbd_state_flag_set(C_USBD_STATE_IN);
	usb_msdc_state_initial();

}

void ap_usb_isr(void)
{
	if (*P_USBD_INT == 0x8000) {
		*P_USBD_INT |= 0x8000;
		vic_irq_disable(VIC_USB);
		usb_uninitial();
		msgQSend(ApQ, MSG_APQ_CONNECT_TO_PC, NULL, NULL, MSG_PRI_NORMAL);
	}
}

void ap_usb_init_isr(void)
{
	if (*P_USBD_INT == 0x8000) {
		*P_USBD_INT |= 0x8000;
		vic_irq_disable(VIC_USB);
		usb_uninitial();
		s_usbd_pin = 1;
	}
}

void state_usb_entry(void* para1)
{
    INT32U msg_id;
    INT8U err,init_detect;
    INT32U i;
    
    usbd_pin_detection_init();
    
    init_detect = 0;
    for (i=0;i<5;i++) {
        if (gpio_read_io(C_USBDEVICE_PIN)) {
            init_detect++;
        }
        if (init_detect >= 3) {
            break;
        }
        OSTimeDly(1);
    }
    
    if (init_detect >= 3) {
    	usb_initial();
    	vic_irq_register(VIC_USB, ap_usb_init_isr);
		vic_irq_enable(VIC_USB);
    	OSTimeDly(35); /* debounce time about 100ms,reset time 10~20ms */
    	//usb_uninitial();
    }
    #if USB_PHY_SUSPEND == 1
    else {
    	*P_USBD_CONFIG |= 0x800;	//Switch to USB20PHY
    	*P_USBD_CONFIG1 |= 0x100; //[8],SW Suspend For PHY
    }
    #endif
    
    USBAPPTaskQ = OSQCreate(usb_task_q_stack, USB_TASK_QUEUE_MAX);
    
	while (1) {
		msg_id = (INT32U) OSQPend(USBAPPTaskQ, 0, &err);
		if((!msg_id) || (err != OS_NO_ERR)) {
	       	continue;
	    }
		
	    switch (msg_id) {
	    	case MSG_USBD_INITIAL:
	    		vic_irq_register(VIC_USB, ap_usb_isr);
   				vic_irq_enable(VIC_USB);
   				usb_initial();
	    		break;	        
		    case MSG_USBD_PLUG_IN:
		    	//usb_uninitial();
		    	OSTimeDly(15);
		    	state_usb_init();
				DBG_PRINT("USBD state Entry\r\n");
				while (s_usbd_pin) {
					usb_isr();
				   	usb_std_service_udisk(0);
				   	usb_msdc_service(0);
				}
				#if USB_PHY_SUSPEND == 0
				usb_initial();
		       	vic_irq_enable(VIC_USB);
		       	#endif
		       	usb_msdc_state_exit();
		       	DBG_PRINT("USBD state Exit\r\n");
		       	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_USBD_EXIT, NULL, NULL, MSG_PRI_NORMAL);
		        break;
		     case MSG_USBCAM_PLUG_IN:
		     	//usb_uninitial();
		     	OSTimeDly(20);
		     	usb_cam_entrance(0);
				usb_webcam_start();
		     	DBG_PRINT("P-CAM Entry\r\n");
		     	while (s_usbd_pin) {
					usb_std_service(0);
				}
				DBG_PRINT("P-CAM Exit\r\n");
				usb_webcam_stop();
				OSTimeDly(30);
   				usb_cam_exit();
   				#if USB_PHY_SUSPEND == 0
   				vic_irq_register(VIC_USB, ap_usb_isr);
   				vic_irq_enable(VIC_USB);
   				usb_initial();
				#endif
   				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_USBD_EXIT, NULL, NULL, MSG_PRI_NORMAL);
		     	break;
	   	}
	}  	
}

#define STG_CFG_CF      0x0001
#define STG_CFG_SD      0x0002
#define STG_CFG_MS      0x0004
#define STG_CFG_USBH    0x0008
#define STG_CFG_USBD    0x0010
#define STG_CFG_NAND1   0x0020
#define STG_CFG_XD      0x0040

#if 0
void state_usb_exit(void)
{
   INT16U stg_init_val=0;    

   //DBG_PRINT("USBD state Exit\r\n");
   sys_usbd_state_flag_set(C_USBD_STATE_OUT);
   /* allow umi to access statck state */

   
   //reinitial cards and NF
#if NAND1_EN == 1
 	if (storage_detection(5)) DrvNand_flush_allblk();
#endif

  if(NAND1_EN == 1) {stg_init_val |= STG_CFG_NAND1;}
  if(SD_EN == 1) {stg_init_val |= STG_CFG_SD;}
  if(MSC_EN == 1) {stg_init_val |= STG_CFG_MS;}
  if(CFC_EN == 1) {stg_init_val |= STG_CFG_CF;}
  if(XD_EN == 1) {stg_init_val |= STG_CFG_XD;}
  // storages_init(0x27);
   storages_init(stg_init_val);


   usb_msdc_state_exit();   
}
#endif
/*
void state_usb_suspend(EXIT_FLAG_ENUM exit_flag)
{
    DBG_PRINT("USBD state Suspend\r\n");
	sys_usbd_state_flag_set(C_USBD_STATE_OUT);
	
}
*/
void sys_usbd_state_flag_set(INT8U in_or_out)
{
	usbd_flag = in_or_out;
}

INT8U sys_usbd_state_flag_get(void)
{
	return usbd_flag;
}
