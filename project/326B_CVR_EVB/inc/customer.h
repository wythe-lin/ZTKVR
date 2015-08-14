#ifndef __CUSTOMER_H__
#define __CUSTOMER_H__

#define CUSTOM_ON       1
#define CUSTOM_OFF      0

/*=== IC Version definition choices ===*/
//---------------------------------------------------------------------------
#define	GPL32_A                     0x1                                    //
#define	GPL32_B                     0x2                                    //
#define	GPL32_C                     0x3                                    //
#define	GPL326XX                    0x10                                   //
#define GPL326XX_B                  0x11                                   //
#define GPL326XX_C                  0x12                                   //
#define GPL326XXB					0x17                                   //
#define GP326XXXA                   0x18
#define GPL327XX                  	0x20
#define GP327XXXA					0x21                                   //
#define MCU_VERSION             	GPL326XXB                              //
//---------------------------------------------------------------------------

/*=== Board ID Config ===*/
//---------------------------------------------------------------------------
#define BOARD_TURNKEY_BASE      0x10000000                                 //
#define BOARD_EMU_BASE          0x00000000                                 //
                                                                           //
#define BOARD_EMU_V1_0          (BOARD_EMU_BASE + 0x10)                    //
#define BOARD_EMU_V2_0          (BOARD_EMU_BASE + 0x20)                    //
#define BOARD_DEMO_GPL32XXX 	(BOARD_EMU_BASE + 0x30)
#define BOARD_DEMO_GPL326XX 	(BOARD_EMU_BASE + 0x40)
#define BOARD_DEMO_GPL327XX 	(BOARD_EMU_BASE + 0x50)
#define BOARD_DEMO_GPL326XXB 	(BOARD_EMU_BASE + 0x60)
#define BOARD_DEMO_GP326XXXA 	(BOARD_EMU_BASE + 0x70)
#define BOARD_DEMO_GP327XXXA 	(BOARD_EMU_BASE + 0x80)
#define BOARD_TK35_V1_0         (BOARD_TURNKEY_BASE + 0x10)                //
#define BOARD_TK_V1_0           (BOARD_TURNKEY_BASE + 0x20)                //
#define BOARD_TK_V2_0           (BOARD_TURNKEY_BASE + 0x30)                //
#define BOARD_TK_V3_0           (BOARD_TURNKEY_BASE + 0x40)                //
#define BOARD_TK_V4_0           (BOARD_TURNKEY_BASE + 0x50)                //
#define BOARD_TK_V4_1           (BOARD_TURNKEY_BASE + 0x60)                //
#define BOARD_TK_V4_3           (BOARD_TURNKEY_BASE + 0x70)                //
#define BOARD_TK_A_V1_0         (BOARD_TURNKEY_BASE + 0x80)                //
#define BOARD_GPY0200_EMU_V2_0  (BOARD_TURNKEY_BASE + 0x90)                //
#define BOARD_TK_V4_4           (BOARD_TURNKEY_BASE + 0xA0)                //
#define BOARD_TK_A_V2_0         (BOARD_TURNKEY_BASE + 0xB0)                //
#define BOARD_TK_A_V2_1_5KEY    (BOARD_TURNKEY_BASE + 0xC0)                //
#define BOARD_TK_7D_V1_0_7KEY   (BOARD_TURNKEY_BASE + 0xD0)                //
#define BOARD_TK_8D_V1_0        (BOARD_TURNKEY_BASE + 0xE0)                //
#define BOARD_TK_8D_V2_0        (BOARD_TURNKEY_BASE + 0xF0)                //
#define BOARD_TK_32600_V1_0     (BOARD_TURNKEY_BASE + 0x100)               //
#define BOARD_TK_32600_COMBO    (BOARD_TURNKEY_BASE + 0x110)               //
#define BOARD_TK_32600_7A_V1_0  (BOARD_TURNKEY_BASE + 0x120)               //
#define BOARD_TYPE              BOARD_DEMO_GPL326XXB


/*=== Key support ===*/
//---------------------------------------------------------------------------
#define KEY_AD_NONE             0
#define KEY_AD_4_MODE           4                                          //
#define KEY_AD_5_MODE           5                                          //
#define KEY_AD_6_MODE           6                                          //
#define KEY_AD_8_MODE           8                                          //
#define KEY_IO_MODE             2                                          //
#define KEY_USER_MODE           3                                          //
#define SUPPORT_AD_KEY          KEY_AD_8_MODE                              //
//---------------------------------------------------------------------------


/* GPY02xx support */
//---------------------------------------------------------------------------
#define GPY02XX_NONE      0                                                //
#define BODY_GPY0200      1                                                //
#define BODY_GPY0201      2                                                //
#define BODY_GPY0202      3                                                //
//choice one                                                               //
#define SUPPORT_GPY02XX   GPY02XX_NONE                                     //
//---------------------------------------------------------------------------


/*=== alarm function configuration ===*/
//---------------------------------------------------------------------------
#define _ALARM_SETTING	CUSTOM_OFF


//configure
#define GPL32680_MINI_DVR_EMU_V2_0			1
#define GPL32680_MINI_DVR_DEMO_BOARD		2
#define GPL32680_MINI_DVR_CAR_RECORD_V2		3
#define MINI_DVR_BOARD_VERSION				GPL32680_MINI_DVR_CAR_RECORD_V2

#define DV185	0
#define DV188	1

#define AVI_FRAME_RATE		30//27//15
#define QUALITY_FACTOR		50
#define REMAIN_SPACE		100		//MB
#define AVI_WIDTH			640
#define AVI_HEIGHT			480
#define TFT_WIDTH			320
#define TFT_HEIGHT			240//240

//mini DVR storage type
#define NAND_FLASH						0
#define T_FLASH							2
#define MINI_DVR_STORAGE_TYPE			T_FLASH

//mic phone input source
#define C_ADC_LINE_IN					0
#define C_GPY0050_IN					1
#define C_BUILDIN_MIC_IN				2  //only GPL32600 support
#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2
	#define MIC_INPUT_SRC					C_BUILDIN_MIC_IN
#else
	#define MIC_INPUT_SRC					C_BUILDIN_MIC_IN
#endif

//audio record sampling rate
#define AUD_RECORD_EN                   1
#define AUD_SAMPLING_RATE_8K			8000
#define AUD_SAMPLING_RATE_16K			16000
#define AUD_SAMPLING_RATE_22K			22050
#define AUD_SAMPLING_RATE_44K			44100
#define AUD_REC_SAMPLING_RATE			AUD_SAMPLING_RATE_22K

#define LIGHT_DETECT_USE_RESISTOR		0
#define LIGHT_DETECT_USE_SENSOR			1
#define LIGHT_DETECT_USE_GPIO			2
#define LIGHT_DETECT_USE 				LIGHT_DETECT_USE_RESISTOR//LIGHT_DETECT_USE_GPIO	//wwj modify


//define multi-media function use
#define DBG_MESSAGE						CUSTOM_ON		//for debug use	//wwj modify

#define KEY_ACTIVE						DATA_LOW
#define ADKEY_LVL_1						0
#define ADKEY_LVL_2						1
#define ADKEY_LVL_3						2
#define ADKEY_LVL_4						3
#define ADKEY_LVL_5						4
#if MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_EMU_V2_0
	#define USE_IOKEY_NO					5//2//4
	#define USE_ADKEY_NO					0//4//0
	#define	PW_KEY							IO_F15
	#define	MENU_KEY						IO_B14//ADKEY_LVL_1//IO_F9
	#define OK_KEY							IO_F5
	#define NEXT_KEY						IO_B12//ADKEY_LVL_3//IO_F10
	#define PREVIOUS_KEY					IO_B13//ADKEY_LVL_4//IO_F11
	#define FUNCTION_KEY					IO_B11//ADKEY_LVL_2
	#define C_USBDEVICE_PIN   				IO_F3
	#define SENSOR_PW						IO_F15
	#define TFT_BL							IO_F15
	#define LED								IO_F15
	#define AD_DETECT_PIN					ADC_LINE_0
	#define ADP_OUT_PIN                     IO_F15
	#define IR_CTRL                     	IO_F15
	#define AV_IN_DET                     	IO_F15
#elif MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_CAR_RECORD_V2

	#define USE_IOKEY_NO					1
	#define USE_ADKEY_NO					5
	#define WATCH_CAM0					IO_A13	// active low
	#define WATCH_CAM1					IO_A14	// active low
	#define	PW_KEY						IO_I5
	#define	MENU_KEY					ADKEY_LVL_1
	#define OK_KEY						ADKEY_LVL_5
	#define NEXT_KEY					ADKEY_LVL_3//IO_F10
	#define PREVIOUS_KEY					ADKEY_LVL_4//IO_F11
	#define FUNCTION_KEY					ADKEY_LVL_2//IO_F5
	#define C_USBDEVICE_PIN   				IO_F5
	#define SENSOR_PW					IO_C10
	#define TFT_BL						IO_B6
	#define LED						IO_I7
	#define AD_DETECT_PIN					ADC_LINE_3
	#define IR_CTRL						IO_A9
	#define ADP_OUT_PIN					C_USBDEVICE_PIN
	#define POWER_EN					IO_I4
	#define AV_IN_DET					IO_C3
	#define SD_CD_PIN					IO_I6
	#define AMP_EN						IO_I9
	#define SPEAKER_EN					AMP_EN
#else
	#define USE_IOKEY_NO					2
	#define USE_ADKEY_NO					4
	#define	PW_KEY							IO_F9
	#define	MENU_KEY						ADKEY_LVL_1
	#define OK_KEY							IO_B15
	#define NEXT_KEY						ADKEY_LVL_3//IO_F10
	#define PREVIOUS_KEY					ADKEY_LVL_4//IO_F11
	#define FUNCTION_KEY					ADKEY_LVL_2//IO_F5
	#define C_USBDEVICE_PIN   				IO_F5
	#define SENSOR_PW						IO_F9
	#define TFT_BL							IO_F9
	#define LED								IO_F9
	#define AD_DETECT_PIN					ADC_LINE_0
#endif

//function
#define C_UVC										CUSTOM_ON
#define C_MOTION_DETECTION							CUSTOM_OFF
#define C_AUTO_DEL_FILE								CUSTOM_ON   //GPL326xxB have this function?  // dominant enable for zero second drop solution
#define C_CYCLIC_VIDEO_RECORD						CUSTOM_ON
#define C_VIDEO_PREVIEW								CUSTOM_ON
#define C_SCREEN_SAVER								CUSTOM_OFF
#define C_BOOT_REC									CUSTOM_ON
#define C_LOGO										CUSTOM_ON
#define C_POWER_OFF_LOGO							CUSTOM_ON
#define C_BATTERY_DETECT							CUSTOM_OFF
#define C_ZOOM										CUSTOM_ON
#define C_FAST_SWITCH_FILE							CUSTOM_ON

//items on setting
#define SETUP_SUPPROT_LANGUAGE						CUSTOM_ON
#define SETUP_SUPPROT_DATE_TIME						CUSTOM_ON
#define SETUP_SUPPROT_VOLUME_CONTROL				CUSTOM_OFF
#define SETUP_SUPPROT_FORMAT						CUSTOM_ON
#define SETUP_SUPPROT_RESTORE_DEFAULT				CUSTOM_ON
#define SETUP_SUPPROT_VERSION						CUSTOM_ON
#define SETUP_SUPPROT_DATE_STAMP					CUSTOM_ON
#define SETUP_SUPPROT_CYCLIC_RECORD_TIME			C_CYCLIC_VIDEO_RECORD	//must refer to "C_CYCLIC_VIDEO_RECORD"
#define SETUP_SUPPROT_MOTION_DETECTION				C_MOTION_DETECTION		//must refer to "C_MOTION_DETECTION"
#define SETUP_SUPPROT_SCREEN_SAVER					C_SCREEN_SAVER			//must refer to "C_SCREEN_SAVER"
#define SETUP_SUPPROT_BOOT_REC						C_BOOT_REC				//must refer to "C_BOOT_REC"

#define BATTERY_FULL_LEVEL					0x1C84		// 4.0 Voltage
#define BATTERY_LOW_LEVEL					0x1A5E		// 3.5 Voltage

//define video encode mode
#define VIDEO_ENCODE_WITH_PPU_IRQ		0
#define VIDEO_ENCODE_WITH_TG_IRQ		1	//only gpl32600 support
#define VIDEO_ENCODE_WITH_FIFO_IRQ		2	//only gpl32600 support
#define VIDEO_ENCODE_USE_MODE			VIDEO_ENCODE_WITH_PPU_IRQ//VIDEO_ENCODE_WITH_TG_IRQ


#endif //__CUSTOMER_H__
