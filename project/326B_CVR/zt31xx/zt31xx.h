/*
 ******************************************************************************
 * zt31xx.h - 
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
 *	2015.12.21	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */
#ifndef __ZT31XX_H__
#define __ZT31XX_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*
 ******************************************************************************
 * Includes
 ******************************************************************************
 */


/*
 ******************************************************************************
 * Constants
 ******************************************************************************
 */



/*
 ******************************************************************************
 * Variables
 ******************************************************************************
 */


/*
 ******************************************************************************
 * API Functions
 ******************************************************************************
 */
extern void		zt31xx_reset(void);
extern unsigned char	zt31xx_ready(void);		// return zero: ready, non-zero: no good

extern void		zt31xx_set_opmode(unsigned char param1, unsigned char param2, unsigned char param3);

extern unsigned char	zt31xx_rd_sensor_r16d8(unsigned char sen, unsigned short reg, unsigned char *val);	// return zero: pass, non-zero: fail
extern unsigned char	zt31xx_wr_sensor_r16d8(unsigned char sen, unsigned short reg, unsigned char val);	// return zero: pass, non-zero: fail

extern void		zt31xx_spi_enable(void);
extern void		zt31xx_spi_erase(void);
extern void		zt31xx_spi_write(unsigned long buf, unsigned long spi, unsigned long len);
extern void		zt31xx_spi_read(unsigned long buf, unsigned long spi, unsigned long len);

extern void		zt31xx_fw_update(void);


#ifdef __cplusplus
}
#endif

#endif /* __ZT31XX_H__ */





