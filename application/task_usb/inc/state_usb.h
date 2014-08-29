#ifndef __STATE_USBD_H__
#define __STATE_USBD_H__
/* parents header include */
//#include "turnkey_umi_task.h"
#include "application.h"

#define C_USB_WP_D_PIN       102//IOG[6]

#define C_USBD_STATE_IN		 1
#define C_USBD_STATE_OUT	 0

extern INT32U usbd_check_lun(DRV_PLUG_STATUS_ST * pStorage);
extern void usb_msdc_state_initial(void) ;
extern INT32U usbd_msdc_lun_change(str_USB_Lun_Info* pNEW_LUN);
extern BOOLEAN usbd_pin_detection(void);
extern void usb_msdc_state(void);
extern void usbd_pin_detection_init(void);

#endif  /*__STATE_CALENDAR_H__*/