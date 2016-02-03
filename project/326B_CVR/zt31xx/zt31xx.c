/*
 ******************************************************************************
 * zt31xx.c - zt31xx firmware upgrade from SD card to SPI
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
/* includes */
#include "ztkconfigs.h"
#include "gplib.h"
#include "drv_l1_sensor.h"
#include "task_state_handling.h"
#include "zt31xx.h"

/* for debug */
#define DEBUG_ZT31XX		1
#if DEBUG_ZT31XX
    #define _dmsg(x)		print_string x
#else
    #define _dmsg(x)
#endif

/* definitions */
#define ZT31XX_ST_I2CNACK	(-1)
#define ZT31XX_ST_DONE		(0)
#define ZT31XX_ST_RUNNING	(1)
#define ZT31XX_ST_ERROR		(2)

#define ZT31XX_ID		(0x04)
#define ZT31XX_RESET		IO_C3 //IO_A15

/* macros */
#define msg(x)			print_string x

#define zt31xx_rd_r16d8(r, v)	sccb_read_Reg16Data8(ZT31XX_ID, r, v)
#define zt31xx_wr_r16d8(r, v)	sccb_write_Reg16Data8(ZT31XX_ID, r, v)


/*
 *****************************************************************************
 *
 * zt31xx mailbox function
 *
 *****************************************************************************
 */
/* for debug */
#define DEBUG_ZT31XX_MBOX	1
#if DEBUG_ZT31XX_MBOX
    #define _dmbox(x)		print_string x
#else
    #define _dmbox(x)
#endif

/* function */
static void zt31xx_mbox_cmd(unsigned char id, unsigned char param1, unsigned char param2, unsigned char param3)
{
  	zt31xx_wr_r16d8(0x0081, param1);
	zt31xx_wr_r16d8(0x0082, param2);
	zt31xx_wr_r16d8(0x0083, param3);
	zt31xx_wr_r16d8(0x0080, id);

	_dmbox((LIGHT_BLUE "[zt31xx]: zt31xx_mbox_cmd() - %02x %02x %02x %02x\r\n" NONE, id, param1, param2, param3));
}

static int zt31xx_mbox_ack(void)
{
	unsigned char	ack;

	if (zt31xx_rd_r16d8(0x0080, &ack) < 0) {
		_dmbox((LIGHT_BLUE "[zt31xx]: zt31xx_mbox_ack() - i2c nack\r\n" NONE));
		return ZT31XX_ST_I2CNACK;
	}

	// if zt31xx cannot execute the command successfully, set bit 6.
	if (ack & 0x40) {
		_dmbox((LIGHT_BLUE "[zt31xx]: zt31xx_mbox_ack() - command error\r\n" NONE));
		return ZT31XX_ST_ERROR;
	}

	// if zt31xx have executed the command successfully, set bit 7.
	if (ack & 0x80) {
		return ZT31XX_ST_DONE;
	}

	// if zt31xx is executing the command, bit 6 and 7 are not set.
	_dmbox((LIGHT_BLUE "[zt31xx]: zt31xx_mbox_ack() - command running\r\n" NONE));
	return ZT31XX_ST_RUNNING;
}


/*
 *****************************************************************************
 *
 * zt31xx utility function
 *
 *****************************************************************************
 */
void zt31xx_set_strapped(unsigned char val)
{
	zt31xx_wr_r16d8(0x0028, val);
}

char zt31xx_get_strapped(void)
{
	unsigned char val;

	if (zt31xx_rd_r16d8(0x0028, &val) < 0) {
		msg(("[zt31xx]: zt31xx_get_strapped() - i2c nack\r\n"));
		return (char) (-1);
	}
	return val;
}

void zt31xx_reset(void)
{
	// initial gpio
	gpio_write_io(ZT31XX_RESET, 1);
	gpio_init_io(ZT31XX_RESET, GPIO_OUTPUT);
	gpio_drving_init_io(ZT31XX_RESET, (IO_DRV_LEVEL) IO_DRIVING_4mA);

	// zt31xx hardware reset
	msg(("[zt31xx]: hardware reset"));
	gpio_write_io(ZT31XX_RESET, 0);
	drv_msec_wait(50);
	gpio_write_io(ZT31XX_RESET, 1);
	drv_msec_wait(25);

	// zt31xx i2c splitter configuration register
	zt31xx_wr_r16d8(0x7fff, 0x03);

	// zt31xx read strapped register
	msg((" (strapped=%02x)\r\n", zt31xx_get_strapped()));
}

unsigned char zt31xx_ready(void)
{
	unsigned char	ack;

	// zt31xx check ready
	msg(("[zt31xx]: wait ready -"));
	for (;;) {
		if (zt31xx_rd_r16d8(0x0080, &ack) < 0) {
			msg(("\r\n[zt31xx]: check ready - i2c nack\r\n"));
			return 1;
		}

		msg((" %02x", ack));
		if (ack == 0x80) {
			break;
		}
		drv_msec_wait(50);
	}
	msg(("\r\n"));
	return 0;
}

void zt31xx_set_opmode(unsigned char param1, unsigned char param2, unsigned char param3)
{
  	zt31xx_wr_r16d8(0x0081, param1);
	zt31xx_wr_r16d8(0x0082, param2);
	zt31xx_wr_r16d8(0x0083, param3);
	zt31xx_wr_r16d8(0x0080, 0x01);
}


/*
 *****************************************************************************
 *
 * zt31xx read/write sensor function
 *
 *****************************************************************************
 */
/* for debug */
#define DEBUG_ZT31XX_SENSOR	1
#if DEBUG_ZT31XX_SENSOR
    #define _dsen(x)		print_string x
#else
    #define _dsen(x)
#endif

/* function */
unsigned char zt31xx_rd_sensor_r16d8(unsigned char sen, unsigned short reg, unsigned char *val)
{
	int	st;

	if (sen == 0) {
		zt31xx_mbox_cmd(0x04, (reg & 0xFF), ((reg >> 8) & 0xFF), 0x00);
	} else {
		zt31xx_mbox_cmd(0x06, (reg & 0xFF), ((reg >> 8) & 0xFF), 0x00);
	}

	while ((st = zt31xx_mbox_ack()) == ZT31XX_ST_RUNNING) {
		drv_msec_wait(100);
	}
	if (st != ZT31XX_ST_DONE) {
		msg(("[zt31xx]: zt31xx_rd_sensor_r16d8() - ack fail\r\n"));
		return 1;
	}

	if (zt31xx_rd_r16d8(0x0083, val) < 0) {
		msg(("[zt31xx]: zt31xx_rd_sensor_r16d8() - data fail\r\n"));
		return 2;
	}

	_dmbox((LIGHT_BLUE "[zt31xx]: zt31xx_rd_sensor_r16d8() - reg=%04x, val=%02x\r\n" NONE, reg, *val));
	return 0;
}

unsigned char zt31xx_wr_sensor_r16d8(unsigned char sen, unsigned short reg, unsigned char val)
{
	int	st;

	if (sen == 0) {
		zt31xx_mbox_cmd(0x05, (reg & 0xFF), ((reg >> 8) & 0xFF), val);
	} else {
		zt31xx_mbox_cmd(0x07, (reg & 0xFF), ((reg >> 8) & 0xFF), val);
	}

	while ((st = zt31xx_mbox_ack()) == ZT31XX_ST_RUNNING) {
		drv_msec_wait(100);
	}
	if (st != ZT31XX_ST_DONE) {
		msg(("[zt31xx]: zt3150_wr_sensor_r16d8() - ack fail\r\n"));
		return 1;
	}

	_dmbox((LIGHT_BLUE "[zt31xx]: zt3150_wr_sensor_r16d8() - done\r\n" NONE));
	return 0;
}


/*
 *****************************************************************************
 *
 * zt31xx read/write spi flash function
 *
 *****************************************************************************
 */
/* for debug */
#define DEBUG_ZT31XX_SPI	0
#if DEBUG_ZT31XX_SPI
    #define _dspi(x)		print_string x
#else
    #define _dspi(x)
#endif


/* definitions */
#define ZT3150_TIMEOUT_COUNT	1000

#define SPI_PAGE_SIZE		256

#define SPI_SR_WP		0x80	// write protect
#define SPI_SR_BP		0x1C	// block protect
#define SPI_SR_WEL		0x02	// write enable
#define SPI_SR_WIP		0x01	// write in process

#define SPI_CMD_WREN		0x06	// command : write enable
#define SPI_CMD_WRDI		0x04	// command : write disable
#define SPI_CMD_RDID		0x9F	// command : read id
#define SPI_CMD_RDSR		0x05	// command : read status register
#define SPI_CMD_WRSR		0x01	// command : write status register
#define SPI_CMD_READ		0x03	// command : read data

#define SPI_CMD_CE		0xC7	// command : chip erase
#define SPI_CMD_BE		0xD8	// command : block/sector erase, 10000h bytes erase
#define SPI_CMD_SE		0x20	// command : sector erase, 1000h bytes erase, for MXIC only
#define SPI_CMD_PP		0x02	// command : page program, 100h bytes.


/* macros */
#define zt31xx_spi_start() \
{ \
	zt31xx_wr_r16d8(0x0600, 0x00); \
	zt31xx_wr_r16d8(0x0602, 0x01); \
}

#define zt31xx_spi_stop() \
{\
	zt31xx_wr_r16d8(0x0600, 0x01); \
	zt31xx_wr_r16d8(0x0602, 0x00); \
}

/* command: */
static void zt31xx_spicmd_ewsr(void)
{
	unsigned long	n;
	unsigned char	st;

	zt31xx_wr_r16d8(0x0604, 0x00);
	zt31xx_wr_r16d8(0x0606, 0x50);	// SPI_CMD_EWSR

	zt31xx_spi_start();
	for (n=0; n<ZT3150_TIMEOUT_COUNT; n++) {
		zt31xx_rd_r16d8(0x0612, &st);
		if (st == 0x00) {
			break;
		}
	}
	zt31xx_spi_stop();
	_dspi(("[spi]: zt31xx_spicmd_ewsr() at %ld\r\n", n));
}

/* command: write status register */
static void zt31xx_spicmd_wrsr(unsigned char val)
{
	unsigned long	n;
	unsigned char	st;

	zt31xx_wr_r16d8(0x0604, 0x00);
	zt31xx_wr_r16d8(0x0606, SPI_CMD_WRSR);

	zt31xx_spi_start();
	for (n=0; n<ZT3150_TIMEOUT_COUNT; n++) {
		zt31xx_rd_r16d8(0x0612, &st);
		if (st == 0x00) {
			break;
		}
	}
	_dspi(("[spi]: zt31xx_spicmd_wrsr() - 1 at %ld\r\n", n));

	zt31xx_wr_r16d8(0x060e, val);
	for (n=0; n<ZT3150_TIMEOUT_COUNT; n++) {
		zt31xx_rd_r16d8(0x0612, &st);
		if (st == 0x00) {
			break;
		}
	}
	zt31xx_spi_stop();
	_dspi(("[spi]: zt31xx_spicmd_wrsr() - 2 at %ld\r\n", n));
}

/* command: read status register */
static unsigned char zt31xx_spicmd_rdsr(void)
{
	unsigned long	n;
	unsigned char	st;
	unsigned char	val;

	zt31xx_wr_r16d8(0x0604, 0x00);
	zt31xx_wr_r16d8(0x0606, SPI_CMD_RDSR);

	zt31xx_spi_start();
	for (n=0; n<ZT3150_TIMEOUT_COUNT; n++) {
		zt31xx_rd_r16d8(0x0612, &st);
		if (st == 0x00)
			break;
	}
	zt31xx_rd_r16d8(0x0610, &val);	// dummy read
	zt31xx_rd_r16d8(0x0610, &val);	// real read
	zt31xx_spi_stop();
	_dspi(("[spi]: zt31xx_spicmd_rdsr() = %02x, at %ld\r\n", (unsigned short) val, n));
	return val;
}

/* command: write enable */
static void zt31xx_spicmd_wren(void)
{
	unsigned long	n;
	unsigned char	st;

	zt31xx_wr_r16d8(0x0604, 0x00);
	zt31xx_wr_r16d8(0x0606, SPI_CMD_WREN);

	zt31xx_spi_start();
	for (n=0; n<ZT3150_TIMEOUT_COUNT; n++) {
		zt31xx_rd_r16d8(0x0612, &st);
		if (st == 0x00) {
			break;
		}
	}
	zt31xx_spi_stop();
	_dspi(("[spi]: zt31xx_spicmd_wren() at %ld\r\n", n));
}

/* command: chip erase */
static int zt31xx_spicmd_ce(void)
{
	unsigned long	n;
	unsigned char	st;

	zt31xx_wr_r16d8(0x0604, 0x00);
	zt31xx_wr_r16d8(0x0606, SPI_CMD_CE);

	zt31xx_spi_start();
	for (n=0; n<ZT3150_TIMEOUT_COUNT; n++) {
		zt31xx_rd_r16d8(0x0612, &st);
		if (st == 0x00) {
			break;
		}
	}
	zt31xx_spi_stop();
	_dspi(("[spi]: zt31xx_spicmd_ce() at %ld\r\n", n));

	for (n=0; n<ZT3150_TIMEOUT_COUNT*10; n++) {
		if (!(zt31xx_spicmd_rdsr() & SPI_SR_WIP)) {
			return 1;
		}
	}
	_dspi(("[spi]: zt31xx_spicmd_ce() time-out\r\n"));
	return 0;
}

/*
 *
 */
static int zt31xx_spi_wren(void)
{
	unsigned long	n;

	for (n=0; n<ZT3150_TIMEOUT_COUNT; n++) {
		if (zt31xx_spicmd_rdsr() & SPI_SR_WEL) {
			return 1;
		}
		zt31xx_spicmd_wren();
	}
	_dspi(("[spi]: zt31xx_spi_wren() time-out\r\n"));
	return 0;
}

/*
 *
 */
void zt31xx_spi_enable(void)
{
	msg(("[zt31xx]: enable spi flash \r\n"));

	// disable hardware strapped spi value
	zt31xx_set_strapped(zt31xx_get_strapped() & 0xFE);

	// disable zt31xx, and release 2KB SRAM.
	zt31xx_wr_r16d8(0x0026, 0x02);
	zt31xx_wr_r16d8(0x0027, 0x01);

	// zt31xx statemachine idle.
	zt31xx_wr_r16d8(0x7000, 0x80);
	zt31xx_wr_r16d8(0x7001, 0xFE);

	// start zt31xx at 2KB SRAM
	zt31xx_wr_r16d8(0x0080, 0x80);
	zt31xx_wr_r16d8(0x001A, 0xFF);
	zt31xx_wr_r16d8(0x0605, 0x03);
	zt31xx_wr_r16d8(0x0027, 0x00);
	zt31xx_wr_r16d8(0x0025, 0x01);
	zt31xx_wr_r16d8(0x0026, 0x00);
}

void zt31xx_spi_disable(void)
{
	msg(("[zt31xx]: disable spi flash\r\n"));

	// disable hardware strapped spi value
	zt31xx_set_strapped(zt31xx_get_strapped() & 0xFE);

	// disable zt31xx, and release 2KB SRAM.
	zt31xx_wr_r16d8(0x0026, 0x02);
	zt31xx_wr_r16d8(0x0027, 0x01);
}

/*
 *
 */
void zt31xx_spi_erase(void)
{
	msg(("[zt31xx]: erase spi flash\r\n"));

	zt31xx_spi_wren();
	zt31xx_spicmd_ce();
}

/**
 * @fn      zt31xx_spi_write
 *
 * @brief   write spi flash
 *
 * @param   buf - starting memory buffer at backend SOC
 * @param   spi - starting memory address in ZT31XX SPI flash
 * @param   len - data length to write to ZT31XX SPI flash
 *
 * @return  none
 */
void zt31xx_spi_write(unsigned long buf, unsigned long spi, unsigned long len)
{
	unsigned long	n;
	unsigned long	real_size;
	unsigned long	index;
	unsigned char	val;
	unsigned char	st;
	unsigned char	*ptr;

	// enable spi flash
	zt31xx_spi_enable();

	// erase all
	zt31xx_spi_erase();

	// write spi flash
	msg(("[zt31xx]: write spi flash - buf=%08x, spi=%08x, len=%08x\r\n", buf, spi, len));
	real_size = SPI_PAGE_SIZE - (spi & (SPI_PAGE_SIZE - 1));

	while (len > 0) {
		if (len <= real_size) {
			real_size = len;
		}

		zt31xx_spi_wren();

		// page write
		zt31xx_wr_r16d8(0x0604, 0x32);
		zt31xx_wr_r16d8(0x0606, SPI_CMD_PP);
		zt31xx_wr_r16d8(0x0607, (spi >> 16) & 0xFF);	// spi flash addr byte MSB
		zt31xx_wr_r16d8(0x0608, (spi >>  8) & 0xFF);	// spi flash addr byte 2nd
		zt31xx_wr_r16d8(0x0609, (spi >>  0) & 0xFF);	// spi flash addr byte LSB

		zt31xx_spi_start();
		ptr = (unsigned char *) buf;
		for (index=0; index<real_size; index++) {
			for (n=0; n<ZT3150_TIMEOUT_COUNT; n++) {
				zt31xx_rd_r16d8(0x0612, &st);
				if (st == 0x00) {
					break;
				}
			}

			// move data from memory buffer
			val = *(ptr + index);
			zt31xx_wr_r16d8(0x060e, val);
		}
		zt31xx_spi_stop();

		len      -= index;
		buf      += index;
		spi       = (spi + SPI_PAGE_SIZE) & (-SPI_PAGE_SIZE);
		real_size = SPI_PAGE_SIZE;

		for (n=0; n<ZT3150_TIMEOUT_COUNT; n++) {
			if ((zt31xx_spicmd_rdsr() & SPI_SR_WIP) == 0) {
				msg(("."));
				break;
			}
		}
	}
	msg(("\r\n"));
}

/**
 * @fn      zt31xx_spi_read
 *
 * @brief   read spi flash
 *
 * @param   buf - Starting memory buffer at backend SOC
 * @param   spi - Starting memory address in ZT31XX SPI flash
 * @param   len - data length to write to ZT31XX SPI flash
 *
 * @return  none
 */
void zt31xx_spi_read(unsigned long buf, unsigned long spi, unsigned long len)
{
	unsigned short	n;
	unsigned char	val;
	unsigned char	st;
	unsigned long	index;
	unsigned char	*ptr;


	// enable spi flash
	zt31xx_spi_enable();

	// read spi flash
	msg(("[zt31xx]: read spi flash - buf=%08x, spi=%08x, len=%08x\r\n", buf, spi, len));
#if 0
	// get spi flash data
	spi -= 1;
	zt31xx_wr_r16d8(0x0604, 0x30);
	zt31xx_wr_r16d8(0x0606, SPI_CMD_READ);
	zt31xx_wr_r16d8(0x0607, (spi >> 16) & 0xFF);	// spi flash addr byte MSB
	zt31xx_wr_r16d8(0x0608, (spi >>  8) & 0xFF);	// spi flash addr byte 2nd
	zt31xx_wr_r16d8(0x0609, (spi >>  0) & 0xFF);	// spi flash addr byte LSB

	// cs low
	zt31xx_spi_start();
	n = 0;
	while (n < ZT3150_TIMEOUT_COUNT) {
		n++;
		zt31xx_rd_r16d8(0x0612, &st);
		if (st == 0x00) {
			break;
		}
	}
	zt31xx_rd_r16d8(0x0610, &val);			// dummy read

	ptr = (unsigned char *) buf;
	for (index=0; index<len; index++) {
		zt31xx_rd_r16d8(0x0610, &val);		// real read
		*(ptr + index) = val;

//		n = 0;
//		while (n < ZT3150_TIMEOUT_COUNT) {
//			n++;
//			zt31xx_rd_r16d8(0x0612, &st);
//			if (st == 0x00) {
//				break;
//			}
//		}
	}
	// cs high
	zt31xx_spi_stop();

#else
	ptr  = (unsigned char *) buf;

	for (index=0, spi-=1; index<len; index++, spi++) {
		//get spi flash data
		zt31xx_wr_r16d8(0x0604, 0x30);
		zt31xx_wr_r16d8(0x0606, SPI_CMD_READ);
		zt31xx_wr_r16d8(0x0607, (spi >> 16) & 0xFF);	// spi flash addr byte MSB
		zt31xx_wr_r16d8(0x0608, (spi >>  8) & 0xFF);	// spi flash addr byte 2nd
		zt31xx_wr_r16d8(0x0609, (spi >>  0) & 0xFF);	// spi flash addr byte LSB

		zt31xx_spi_start();
		n = 0;
		while (n < ZT3150_TIMEOUT_COUNT) {
			n++;
			zt31xx_rd_r16d8(0x0612, &st);
			if (st == 0x00) {
				break;
			}
		}
		zt31xx_rd_r16d8(0x0610, &val);			// dummy read
		zt31xx_rd_r16d8(0x0610, &val);			// real read
		zt31xx_spi_stop();

		// move data to memory buffer
		*(ptr + index) = val;

		msg(("."));
	}
#endif
	msg(("\r\n"));
}


/*
 *****************************************************************************
 *
 * zt31xx update firmware function
 *
 *****************************************************************************
 */
/* for debug */
#define DEBUG_ZT31XX_FWUPDATE		1
#if DEBUG_ZT31XX_FWUPDATE
    #define _dfwup(x)			print_string x
#else
    #define _dfwup(x)
#endif

#define SZ_64KB			(64*1024)

extern VIDEO_ARGUMENT		gvarg;		// global video argument

/*
 * upgrade ZT31XX firmware
 * for zt3150_upgrade.bin/zt3120_upgrade.bin
 *
 *
 */
static char			*zt31xx_fwup_filename[] = {
	"C:\\zt3150_upgrade.bin",
	"C:\\zt3120_upgrade.bin",
	NULL
};

void zt31xx_fw_update(void)
{
	struct stat_t		fsta;
	INT16S			fd;
	INT32U			*bincode;
	INT32U			len;
	char			path[80];
	int 			n;

	_dfwup((BROWN "[S]: zt31xx_fw_update()\r\n" NONE));

	// retain the original path
	getcwd(path, sizeof(path));
	msg(("[zt31xx]: current path=%s\r\n", path));

	// check media
	msg(("[zt31xx]: sd card - "));
	if (chdir("C:\\") != 0) {
		msg(("not found\r\n", path));
		goto __exit;
	}
	msg(("found\r\n"));

	// search file
	for (n=0; zt31xx_fwup_filename[n]!=NULL; n++) {
		msg(("[zt31xx]: %s - ", zt31xx_fwup_filename[n]));

		fd = open(zt31xx_fwup_filename[n], O_RDONLY);
		if (fd >= 0) {
			msg(("found\r\n"));
			goto __read;
		}
		msg(("not found\r\n"));
	}
	goto __exit;

__read:
	// read bin file to sdram
	if (fstat(fd, &fsta)) {
		msg(("[zt31xx]: zt31xx_fw_update() - get file size fail\r\n"));
		goto __exit1;
	}
	if (fsta.st_size <= SZ_64KB) {
		len = SZ_64KB;
	} else {
		len = ((fsta.st_size + (SZ_64KB - 1)) / SZ_64KB) * SZ_64KB;
	}
	bincode = (INT32U *) gp_malloc(len);
	if (!bincode) {
		msg(("[zt31xx]: zt31xx_fw_update() - allocte buffer fail\r\n"));
		goto __exit1;
	}
	_dfwup(("[zt31xx]: %s, len=%08x, buf==%08x\r\n", zt31xx_fwup_filename[n], fsta.st_size, len));

	// read bin file
	if (read(fd, (INT32U) bincode, len) <= 0) {
		msg(("[zt31xx]: zt31xx_fw_update() - read file fail\r\n"));
		goto __exit1;
	}

	zt31xx_spi_write((unsigned long) bincode, 0, len);

__exit1:
	close(fd);
__exit:
	chdir(path);
	_dfwup((BROWN "[E]: zt31xx_fw_update()\r\n" NONE));
}


