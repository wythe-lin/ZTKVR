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

#define DVR516_CFG			1
#define DVR517_CFG			0
#define K6000_CFG			0
#define K12_CFG				0


#undef C_DISPLAY_DEVICE
#if   (DVR516_CFG == 1)
   #define C_DISPLAY_DEVICE		ILI8961//ILI9341
   #define Z_SIDE_BY_SIDE		1
   #define ADKEY_WITH_BAT		0
   #define DUAL_ADP_IN			1
#elif (DVR517_CFG == 1)
   #define C_DISPLAY_DEVICE		ILI8961
   #define Z_SIDE_BY_SIDE		0
   #define ADKEY_WITH_BAT		0
   #define DUAL_ADP_IN			1
#elif (K6000_CFG == 1)
   #define C_DISPLAY_DEVICE		ILI9341
   #define Z_SIDE_BY_SIDE		1
   #define ADKEY_WITH_BAT		0
   #define DUAL_ADP_IN			1
#elif (K12_CFG == 1)
   #define C_DISPLAY_DEVICE		ILI9341
   #define Z_SIDE_BY_SIDE		1
   #define ADKEY_WITH_BAT		0
   #define DUAL_ADP_IN			1
#else
   #error must be chioce one xxx_CFG
#endif


#endif	// __ZTKCONFIGS_H__