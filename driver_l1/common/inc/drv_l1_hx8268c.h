/*
 ******************************************************************************
 * drv_l1_hx8268c.h - 3-wire Serial Interface for ILI8961
 *
 * Copyright (c) 2014-2015 by ZealTek Electronic Co., Ltd.
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
 *	2014.08.21	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */
#ifndef __DRV_L1_HX8268C_H__
#define __DRV_L1_HX8268C_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"


extern void		hx8268c_init(void);
extern void		hx8268c_write(unsigned char reg, unsigned char val);
extern unsigned char	hx8268c_read(unsigned char reg);


#endif // __DRV_L1_ILI8961_H__
