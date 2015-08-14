/*
 ******************************************************************************
 * ztkconfigs.h -
 *
 * Copyright (c) 2014-2016 by ZealTek Electronic Co., Ltd.
 *
 * This software is copyrighted by and is the property of ZealTek
 * Electronic Co., Ltd. All rights are reserved by ZealTek Electronic
 * Co., Ltd. This software may only be used in accordance with the
 * corresponding license agreement. Any unauthorized use, duplication,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice MUST not be removed or modified without prior
 * written consent of ZealTek Electronic Co., Ltd.
 *
 * ZealTek Electronic Co., Ltd. reserves the right to modify this
 * software without notice.
 *
 * History:
 *	2014.10.14	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */

#ifndef __ZTKCONFIGS_H__
#define __ZTKCONFIGS_H__


/* custom */
#define DVR516_CFG			0
#define DVR517_CFG			0
#define K6000_CFG			0
#define K12_CFG				0
#define EVB_CFG				1

/* */
#undef C_DISPLAY_DEVICE
#if   (DVR516_CFG == 1)
   #define C_DISPLAY_DEVICE		ILI8961//HX8268C//ILI9341
   #define FLIP_ILI8961			1
   #define Z_SIDE_BY_SIDE		1
   #define ADKEY_WITH_BAT		0
   #define DUAL_ADP_IN			1
   #define TIME_ADP_OUT_DLY		3		// unit: second
   #define d_anti_flicker(x)		(!x)
#elif (DVR517_CFG == 1)
   #define C_DISPLAY_DEVICE		ILI8961
   #define FLIP_ILI8961			0
   #define Z_SIDE_BY_SIDE		0
   #define ADKEY_WITH_BAT		0
   #define DUAL_ADP_IN			1
   #define TIME_ADP_OUT_DLY		5		// unit: second
   #define d_anti_flicker(x)		(!x)
#elif (K6000_CFG == 1)
   #define C_DISPLAY_DEVICE		ILI9341
   #define Z_SIDE_BY_SIDE		1
   #define ADKEY_WITH_BAT		0
   #define DUAL_ADP_IN			1
   #define TIME_ADP_OUT_DLY		5		// unit: second
   #define d_anti_flicker(x)		(x)
#elif (K12_CFG == 1)
   #define C_DISPLAY_DEVICE		ILI9341
   #define Z_SIDE_BY_SIDE		1
   #define ADKEY_WITH_BAT		0
   #define DUAL_ADP_IN			1
   #define TIME_ADP_OUT_DLY		1		// unit: second
   #define d_anti_flicker(x)		(!x)
#elif (EVB_CFG == 1)
   #define C_DISPLAY_DEVICE		TPO_TD025THD1
   #define Z_SIDE_BY_SIDE		1
   #define ADKEY_WITH_BAT		0
   #define DUAL_ADP_IN			1
   #define TIME_ADP_OUT_DLY		1		// unit: second
   #define d_anti_flicker(x)		(!x)
#else
   #error must be chioce one xxx_CFG
#endif


/* adkey threshold */
#define ADKEY_VAL0			(0)
#define ADKEY_VAL1			(0x28e0)
#define ADKEY_VAL2			(0x51f0)
#define ADKEY_VAL3			(0x7e70)
#define ADKEY_VAL4			(0xb090)
#define ADKEY_VAL5			(0xcd80)
#define ADKEY_VAL6			(0)
#define ADKEY_VAL7			(0)
#define ADKEY_VAL8			(0)

#if (EVB_CFG == 1)
    #define C_AD_VALUE_0		0
    #define C_AD_VALUE_1		70
    #define C_AD_VALUE_2		127+40
    #define C_AD_VALUE_3		280+40 
    #define C_AD_VALUE_4		410+40
    #define C_AD_VALUE_5		520+40 
    #define C_AD_VALUE_6		650+40
    #define C_AD_VALUE_7		760+40
    #define C_AD_VALUE_8		880+40
#else
    #if 0
    // sub some value
	#define ADKEY_THRD_OFFSET	(0x1000)
	#define C_AD_VALUE_0		ADKEY_VAL0
	#define C_AD_VALUE_1		((ADKEY_VAL1 - ADKEY_THRD_OFFSET) >> 6)			// menu: 163
	#define C_AD_VALUE_2		((ADKEY_VAL2 - ADKEY_THRD_OFFSET) >> 6)			// ok:   326
	#define C_AD_VALUE_3		((ADKEY_VAL3 - ADKEY_THRD_OFFSET) >> 6)			// mode: 505
	#define C_AD_VALUE_4		((ADKEY_VAL4 - ADKEY_THRD_OFFSET) >> 6)			// down: 711
	#define C_AD_VALUE_5		((ADKEY_VAL5 - ADKEY_THRD_OFFSET) >> 6)			// up:   822
	#define C_AD_VALUE_6		ADKEY_VAL6
	#define C_AD_VALUE_7		ADKEY_VAL7
	#define C_AD_VALUE_8		ADKEY_VAL8
    #else
	#define C_AD_VALUE_0		ADKEY_VAL0
	#define C_AD_VALUE_1		((ADKEY_VAL1 - ((ADKEY_VAL1 - ADKEY_VAL0) >> 1)) >> 6)	// menu
	#define C_AD_VALUE_2		((ADKEY_VAL2 - ((ADKEY_VAL2 - ADKEY_VAL1) >> 1)) >> 6)	// ok
	#define C_AD_VALUE_3		((ADKEY_VAL3 - ((ADKEY_VAL3 - ADKEY_VAL2) >> 1)) >> 6)	// mode
	#define C_AD_VALUE_4		((ADKEY_VAL4 - ((ADKEY_VAL4 - ADKEY_VAL3) >> 1)) >> 6)	// down
	#define C_AD_VALUE_5		((ADKEY_VAL5 - ((ADKEY_VAL5 - ADKEY_VAL4) >> 1)) >> 6)	// up
	#define C_AD_VALUE_6		ADKEY_VAL6
	#define C_AD_VALUE_7		ADKEY_VAL7
	#define C_AD_VALUE_8		ADKEY_VAL8
    #endif
#endif

/* battery threshold */
#define BATT_LV0			0xD200		
#define BATT_LV1			0xCF00
#define BATT_LV2			0xCC00
#define BATT_LV3			0xC700

#define C_BATT_LEVEL0			BATT_LV0
#define C_BATT_LEVEL1			BATT_LV1
#define C_BATT_LEVEL2			BATT_LV2
#define C_BATT_LEVEL3			BATT_LV3


#endif	// __ZTKCONFIGS_H__