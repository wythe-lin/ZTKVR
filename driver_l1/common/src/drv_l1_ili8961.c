/*
 ******************************************************************************
 * drv_l1_ili8961.c - 3-wire Serial Interface for ILI8961
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
#include "drv_l1_ili8961.h"
#include "gplib.h"


#if (defined ILI8961) && (C_DISPLAY_DEVICE == ILI8961)

/*
 * for debug
 */
#define DEBUG_ILI8961		0
#if DEBUG_ILI8961
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
#define ili8961_spenb_hi()	gpio_write_io(SPENB, DATA_HIGH)
#define ili8961_spenb_lo()	gpio_write_io(SPENB, DATA_LOW)

#define ili8961_spck_hi()	gpio_write_io(SPCK,  DATA_HIGH)
#define ili8961_spck_lo()	gpio_write_io(SPCK,  DATA_LOW)

#define ili8961_spda_hi()	gpio_write_io(SPDA,  DATA_HIGH)
#define ili8961_spda_lo()	gpio_write_io(SPDA,  DATA_LOW)

#define ili8961_spda_read()	gpio_read_io(SPDA)

#define ili8961_dly()		ASM(NOP)


/*
 *****************************************************************************
 *
 *
 *
 *****************************************************************************
 */
void ili8961_init(void)
{
	int	i;

	_D(DBG_PRINT("ili8961_init - start\r\n"));

	gpio_set_port_attribute(SPENB, ATTRIBUTE_HIGH);
	gpio_set_port_attribute(SPCK,  ATTRIBUTE_HIGH);
	gpio_set_port_attribute(SPDA,  ATTRIBUTE_HIGH);

	gpio_init_io(SPENB, GPIO_OUTPUT);
	gpio_init_io(SPCK,  GPIO_OUTPUT);
	gpio_init_io(SPDA,  GPIO_OUTPUT);

	ili8961_spenb_hi();
	ili8961_spck_hi();
	ili8961_spda_hi();

	for (i=0; i<16; i++) {
		ili8961_spck_lo();
		ili8961_dly();
		ili8961_spck_hi();
	}

	_D(DBG_PRINT("ili8961_init - end\r\n"));
}

void ili8961_write(unsigned char reg, unsigned char val)
{
	int	i;

	_D(DBG_PRINT("ili8961_write - start\r\n"));

	// SPENB = 0
	ili8961_spenb_lo();

	// send address
	reg &= ~0x40;				// bit6 = 0 is write
	for (i=0; i<8; i++) {
		ili8961_spck_lo();		// SPCK = 0
		if (reg & 0x80) {
			ili8961_spda_hi();
		} else {
			ili8961_spda_lo();
		}
		ili8961_dly();			// for serial data input setup time (tS1), see ili8961 spec
		ili8961_spck_hi();		// SPCK = 1
		ili8961_dly();			// for serial data input hold  time (tH1), see ili8961 spec
		reg <<= 1;
	}

	// send data
	for (i=0; i<8; i++) {
		ili8961_spck_lo();		// SPCK = 0
		if (val & 0x80) {
			ili8961_spda_hi();
		} else {
			ili8961_spda_lo();
		}
		ili8961_dly();			// for serial data input setup time (tS1), see ili8961 spec
		ili8961_spck_hi();		// SPCK = 1
		ili8961_dly();			// for serial data input hold  time (tH1), see ili8961 spec
		val <<= 1;
	}

	// SPENB = 1
	ili8961_spenb_hi();

	_D(DBG_PRINT("ili8961_write - end\r\n"));
}

unsigned char ili8961_read(unsigned char reg)
{
	int		i;
	unsigned char	val;

	// SPENB = 0
	ili8961_spenb_lo();

	// send address
	reg |= 0x40;				// bit6 = 1 is read
	for (i=0; i<8; i++) {
		ili8961_spck_lo();		// SPCK = 0
		if (reg & 0x80) {
			ili8961_spda_hi();
		} else {
			ili8961_spda_lo();
		}
		ili8961_dly();			// for serial data input setup time (tS1), see ili8961 spec
		ili8961_spck_hi();		// SPCK = 1
		ili8961_dly();			// for serial data input hold  time (tH1), see ili8961 spec
		reg <<= 1;
	}

	// receive data
	gpio_init_io(SPDA, GPIO_INPUT);
	for (i=0, val=0; i<8; i++) {
		ili8961_spck_lo();		// SPCK = 0
		ili8961_dly();			// for serial data input setup time (tS1), see ili8961 spec
		ili8961_spck_hi();		// SPCK = 1
		ili8961_dly();			// for serial data input hold  time (tH1), see ili8961 spec
		val <<= 1;
		val  |= ili8961_spda_read();
	}

	// SPENB = 1
	ili8961_spenb_hi();
	gpio_init_io(SPDA, GPIO_OUTPUT);

	return val;
}

#endif // (defined ILI8961) && (C_DISPLAY_DEVICE == ILI8961)
