/*
 ******************************************************************************
 * drv_l1_hx8268c.c - 3-wire Serial Interface for ILI8961
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
#include "drv_l1_hx8268c.h"
#include "gplib.h"


#if (defined HX8268C) && (C_DISPLAY_DEVICE == HX8268C)

/*
 * for debug
 */
#define DEBUG_HX8268C		0
#if DEBUG_HX8268C
    #define _D(x)		(x)
#else
    #define _D(x)
#endif


/*
 * I/O definitions
 */
#define SPENB			IO_G15
#define SPCK			IO_C10
#define SPDA			IO_G13


/*
 *****************************************************************************
 *
 *
 *
 *****************************************************************************
 */
#define hx8268c_spenb_hi()	gpio_write_io(SPENB, DATA_HIGH)
#define hx8268c_spenb_lo()	gpio_write_io(SPENB, DATA_LOW)

#define hx8268c_spck_hi()	gpio_write_io(SPCK,  DATA_HIGH)
#define hx8268c_spck_lo()	gpio_write_io(SPCK,  DATA_LOW)

#define hx8268c_spda_hi()	gpio_write_io(SPDA,  DATA_HIGH)
#define hx8268c_spda_lo()	gpio_write_io(SPDA,  DATA_LOW)

#define hx8268c_spda_read()	gpio_read_io(SPDA)

#define hx8268c_dly()		ASM(NOP)


/*
 *****************************************************************************
 *
 *
 *
 *****************************************************************************
 */
void hx8268c_init(void)
{
	int	i;

	_D(DBG_PRINT("hx8268c_init - start\r\n"));

	hx8268c_spenb_hi();
	hx8268c_spck_hi();
	hx8268c_spda_hi();

	gpio_set_port_attribute(SPENB, ATTRIBUTE_HIGH);
	gpio_set_port_attribute(SPCK,  ATTRIBUTE_HIGH);
	gpio_set_port_attribute(SPDA,  ATTRIBUTE_HIGH);

	gpio_init_io(SPENB, GPIO_OUTPUT);
	gpio_init_io(SPCK,  GPIO_OUTPUT);
	gpio_init_io(SPDA,  GPIO_OUTPUT);

	for (i=0; i<16; i++) {
		hx8268c_spck_lo();
		hx8268c_dly();
		hx8268c_spck_hi();
	}

	_D(DBG_PRINT("hx8268c_init - end\r\n"));
}

void hx8268c_write(unsigned char reg, unsigned char val)
{
	int		i;
	unsigned char	addr;

	_D(DBG_PRINT("hx8268c_write - start\r\n"));

	// SPENB = 0
	hx8268c_spenb_lo();

	// send address
	addr  = (reg << 1) & 0x80;
	addr |= (reg & 0x3F);
	addr &= ~0x40;				// bit6 = 0 is write
	for (i=0; i<8; i++) {
		hx8268c_spck_lo();		// SPCK = 0
		if (reg & 0x80) {
			hx8268c_spda_hi();
		} else {
			hx8268c_spda_lo();
		}
		hx8268c_dly();			// for serial data input setup time (tS1), see ili8961 spec
		hx8268c_spck_hi();		// SPCK = 1
		hx8268c_dly();			// for serial data input hold  time (tH1), see ili8961 spec
		addr <<= 1;
	}

	// send data
	for (i=0; i<8; i++) {
		hx8268c_spck_lo();		// SPCK = 0
		if (val & 0x80) {
			hx8268c_spda_hi();
		} else {
			hx8268c_spda_lo();
		}
		hx8268c_dly();			// for serial data input setup time (tS1), see ili8961 spec
		hx8268c_spck_hi();		// SPCK = 1
		hx8268c_dly();			// for serial data input hold  time (tH1), see ili8961 spec
		val <<= 1;
	}

	// SPENB = 1
	hx8268c_spenb_hi();

	_D(DBG_PRINT("hx8268c_write - end\r\n"));
}

unsigned char hx8268c_read(unsigned char reg)
{
	int		i;
	unsigned char	addr;

	_D(DBG_PRINT("hx8268c_read - start\r\n"));

	// SPENB = 0
	hx8268c_spenb_lo();

	// send address
	addr  = (reg << 1) & 0x80;
	addr |= (reg & 0x3F);
	addr |= ~0x40;				// bit6 = 1 is read
	for (i=0; i<8; i++) {
		hx8268c_spck_lo();		// SPCK = 0
		if (reg & 0x80) {
			hx8268c_spda_hi();
		} else {
			hx8268c_spda_lo();
		}
		hx8268c_dly();			// for serial data input setup time (tS1), see ili8961 spec
		hx8268c_spck_hi();		// SPCK = 1
		hx8268c_dly();			// for serial data input hold  time (tH1), see ili8961 spec
		reg <<= 1;
	}

	// receive data
	gpio_init_io(SPDA, GPIO_INPUT);
	for (i=0, val=0; i<8; i++) {
		hx8268c_spck_lo();		// SPCK = 0
		hx8268c_dly();			// for serial data input setup time (tS1), see ili8961 spec
		hx8268c_spck_hi();		// SPCK = 1
		hx8268c_dly();			// for serial data input hold  time (tH1), see ili8961 spec
		val <<= 1;
		val  |= hx8268c_spda_read();
	}

	gpio_init_io(SPDA, GPIO_OUTPUT);

	// SPENB = 1
	hx8268c_spenb_hi();

	_D(DBG_PRINT("hx8268c_read - end\r\n"));
}

#endif // (defined HX8268C) && (C_DISPLAY_DEVICE == HX8268C)
