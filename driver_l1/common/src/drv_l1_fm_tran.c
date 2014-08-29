#include "drv_l1_fm_tran.h"
#define _DRV_L1_FM_TX 1

#if (defined _DRV_L1_FM_TX) && (_DRV_L1_FM_TX == 1)

#define DEV_ADDR 0x1d

void SetFrequency(INT32U freq);
void Chip_initialization(void);

INT32U fm_tx_ready,fm_power=1;

//register initialization value
//please revise the following array according to your hardware configuration!!!!!!
INT32U HW_Reg[]=
{
/*0x02e286bd,//MSB// reg1 high byte(0x02); reg1 low byte(0xe2);reg0 high byte(0x86); reg0 low byte(0xbd);//LSB//  frequency*2^21 /3.8=reg1,reg0
0x000782e3,//MSB// reg3 high byte(0x00); reg3 low byte(0x07);reg2 high byte(0x82); reg2 low byte(0xe3);//LSB//
//reg4 0x10c0800c for 32.768K, 0x10c0800e for freq higher than 4M
0x10c0800c,//MSB// reg4/reg5 byte1(0x10); reg4/reg5 byte0(0xc0); reg4/reg5 byte3(0x80); reg4/reg5 byte2(0x0e);//LSB//
//reg6 0x00402398 for 32.768K, 0x39fc2398 for 7.6M
0x00402398,//MSB// reg6/reg7 byte1(0x23); reg6/reg7 byte0(0x98); reg6/reg7 byte3(0x00); reg6/reg7 byte2(0x40);//LSB//
*/

//0x17f286bc,//MSB// reg1 high byte(0x02); reg1 low byte(0xe2);reg0 high byte(0x86); reg0 low byte(0xbd);//LSB//  frequency*2^21 /3.8=reg1,reg0
0x02e286bd,
//0x17f286bc,
0x000782e3,//MSB// reg3 high byte(0x00); reg3 low byte(0x07);reg2 high byte(0x82); reg2 low byte(0xe3);//LSB//
//reg4 0x10c0800c for 32.768K, 0x10c0800e for freq higher than 4M
/*0x00c0810c,
0x00402198
*/
0x10c0800e,//MSB// reg4/reg5 byte1(0x10); reg4/reg5 byte0(0xc0); reg4/reg5 byte3(0x80); reg4/reg5 byte2(0x0e);//LSB//
////0x00c0810c,
//reg6 0x00402398 for 32.768K, 0x39fc2398 for 7.6M, 0x5b8e2398 for 12M
0x5b8e2398//MSB// reg6/reg7 byte1(0x23); reg6/reg7 byte0(0x98); reg6/reg7 byte3(0x00); reg6/reg7 byte2(0x40);//LSB//
////0x00402198
//0x11d839fc

};

/**************************************************
Function: Delay1us
Description:
	Delay 1 us
Parameter:
	None
Return:
	None
**************************************************/
void Delay1us()//please revise this function according to your system
{
	INT16U i;
	for(i=0;i<(1<<8);i++)
		i=i;
}


#define BK_DATA_HIGH()   	gpio_write_io(FM_SDA, DATA_HIGH)
#define BK_DATA_LOW()    	gpio_write_io(FM_SDA, DATA_LOW)
#define BK_DATA_READ()  	gpio_read_io(FM_SDA)

#define SDADIROUT()       	gpio_init_io(FM_SDA, GPIO_OUTPUT);
#define SDADIRIN()        	gpio_init_io(FM_SDA, GPIO_INPUT);
#define SCLDIROUT()       	gpio_init_io(FM_SCL, GPIO_OUTPUT);

#define BK_CLK_HIGH()     	gpio_write_io(FM_SCL, DATA_HIGH)
#define BK_CLK_LOW()     	 gpio_write_io(FM_SCL, DATA_LOW)


/*************************************************
  Function:       BEKEN_I2C_init
  Description:    BEKEN I2C initialize
  Input:          None

  Output:         None
  Return:         None
*************************************************/
void BEKEN_I2C_init(void)
{
    SDADIROUT();                  //SDA output
    SCLDIROUT();                  //SCL output
    BK_CLK_HIGH();
    BK_DATA_HIGH();
}

/*************************************************
  Function:       BEKEN_I2C_Start
  Description:    BEKEN I2C transaction start
  Input:          None

  Output:         None
  Return:         None
*************************************************/
void BEKEN_I2C_Start(void)
{
    BEKEN_I2C_init();
    Delay1us();
    BK_DATA_LOW();
    Delay1us();
    BK_CLK_LOW();
    Delay1us();
    BK_DATA_HIGH();
}

/*************************************************
  Function:       BEKEN_I2C_Stop
  Description:    BEKEN I2C transaction end
  Input:          None

  Output:         None
  Return:         None
*************************************************/
void BEKEN_I2C_Stop(void)
{
    SDADIROUT();
    BK_DATA_LOW();
    Delay1us();
    BK_CLK_HIGH();
    Delay1us();
    BK_DATA_HIGH();
    Delay1us();
}

/*************************************************
  Function:       BEKEN_I2C_ack
  Description:    send ACK,accomplish a operation
  Input:          None

  Output:         None
  Return:         None
*************************************************/
void BEKEN_I2C_ack(void)
{
    SDADIROUT();

    BK_CLK_LOW();
    BK_DATA_LOW();
    Delay1us();

    BK_CLK_HIGH();
    Delay1us();
    BK_CLK_LOW();
}

/*************************************************
  Function:       BEKEN_I2C_nack
  Description:    send NACK,accomplish a operation
  Input:          None

  Output:         None
  Return:         None
*************************************************/
void BEKEN_I2C_nack(void)
{
    SDADIROUT();

    BK_CLK_LOW();
    BK_DATA_HIGH();
    Delay1us();

    BK_CLK_HIGH();
    Delay1us();
    BK_CLK_LOW();
}

/*************************************************
  Function:       BEKEN_I2C_ReceiveACK
  Description:    wait ACK,accomplish a operation
  Input:          None

  Output:         None
  Return:         ack flag
*************************************************/
INT8U BEKEN_I2C_ReceiveACK(void)
{
    INT32U ackflag;
    SDADIRIN();
    Delay1us();
    BK_CLK_HIGH();
    Delay1us();
    ackflag = (INT8U)BK_DATA_READ();
    BK_CLK_LOW();
    Delay1us();
    return ackflag;
}

/*************************************************
  Function:       BEKEN_I2C_sendbyte
  Description:    write a byte
  Input:          a byte written

  Output:         None
  Return:         None
*************************************************/
void BEKEN_I2C_sendbyte(INT8U I2CSendData)
{

    INT8U  i;
        SDADIROUT();
    Delay1us();;
    for(i = 0;i < 8;i++)
    {
        if(I2CSendData & 0x80)
        {
            BK_DATA_HIGH();     //if high bit is 1,SDA= 1
        }
        else
            BK_DATA_LOW();                               //else SDA=0

       	Delay1us();
        BK_CLK_HIGH();
       	Delay1us();
        I2CSendData <<= 1;                          //shift left 1 bit
        BK_CLK_LOW();
    }
}

/*************************************************
  Function:       BEKEN_I2C_readbyte
  Description:    read a byte
  Input:          None

  Output:         None
  Return:         a byte read
*************************************************/
INT8U BEKEN_I2C_readbyte(void)
{
    INT8U i;
    INT8U ucRDData = 0;                     //return value

    SDADIRIN();
    Delay1us();;
    for(i = 0;i < 8;i++)
    {
        BK_CLK_HIGH();
        ucRDData <<= 1;
        Delay1us();
        if(BK_DATA_READ())
            ucRDData|=0x01;
        BK_CLK_LOW();
        Delay1us();
    }
    return(ucRDData);
}

INT32U Wire2_Spi0_Read_32Bit(INT8U addr)
{
    INT32U data = 0;
    BEKEN_I2C_Start();

    BEKEN_I2C_sendbyte(DEV_ADDR);
    BEKEN_I2C_ReceiveACK();
    addr=(addr<<1)|0x01;
    BEKEN_I2C_sendbyte(addr);
    BEKEN_I2C_ReceiveACK();

	data |= BEKEN_I2C_readbyte()<<8;
    BEKEN_I2C_ack();

	data |= BEKEN_I2C_readbyte();
    BEKEN_I2C_ack();

    data |= BEKEN_I2C_readbyte()<<24;
    BEKEN_I2C_ack();

    data |= BEKEN_I2C_readbyte()<<16;
    BEKEN_I2C_nack();

    BEKEN_I2C_Stop();
    return data;
}
/**************************************************
Function: Wire2_Spi0_Write_32Bit
Description:
	write a 32 bit register.

Parameter:
	addr:  register address*2
	value: register value
Return:
	None
**************************************************/
void Wire2_Spi0_Write_32Bit(INT8U addr,INT32U value)
{
	INT32U /*temp,*/ data;	//wwj mark
	INT8U add;
	add = addr;
	data = value;
    BEKEN_I2C_Start();

    BEKEN_I2C_sendbyte(DEV_ADDR);
    BEKEN_I2C_ReceiveACK();
    addr=addr<<1;
    BEKEN_I2C_sendbyte(addr);
    BEKEN_I2C_ReceiveACK();

    BEKEN_I2C_sendbyte((value>>8)&0xff);
    BEKEN_I2C_ReceiveACK();

    BEKEN_I2C_sendbyte((value)&0xff);
    BEKEN_I2C_ReceiveACK();

    BEKEN_I2C_sendbyte((value>>24)&0xff);
    BEKEN_I2C_ReceiveACK();

    BEKEN_I2C_sendbyte((value>>16)&0xff);
   	BEKEN_I2C_ReceiveACK();

    BEKEN_I2C_Stop();
}



/**************************************************
Function: Chip_initialization
Description:
	BK1085 chip initialize
Parameter:
	none
Return:
	None
**************************************************/
void Chip_initialization(void)
{
	INT32U temp,tmp,i;
	
	fm_tx_ready = 0;
	return;

	tmp = R_SPI0_CTRL;
	R_SPI0_CTRL = 0;
//	R_SPI0_CTRL &= ~(1 << 15);	//spi  disable
	R_MEM_IO_CTRL &= ~(1<<2);	//set xa20 as gpio,use to fm_SCK
/*
	Wire2_Spi0_Write_32Bit(0*2,HW_Reg[0]);//reg01
	temp = Wire2_Spi0_Read_32Bit(0*2);
	*/
#if (defined MINI_DVR_BOARD_VERSION) && (MINI_DVR_BOARD_VERSION == GPL32680_MINI_DVR_EMU_V2_0)
	fm_tx_ready = 1;
	return;
#endif
/*
	if(temp == HW_Reg[0]) {
		fm_tx_ready = 1;
	}
	else {
		fm_tx_ready = 0;
		return;
	}
	*/

	for(i=0;i<5;i++){
		Wire2_Spi0_Write_32Bit(0*2,HW_Reg[0]);//reg01
		temp = Wire2_Spi0_Read_32Bit(0*2);
		if(temp == HW_Reg[0]) {
			fm_tx_ready = 1;
			break;
		}
		else if(i == 5) {
			fm_tx_ready = 0;
			return;
		}
	}

	Wire2_Spi0_Write_32Bit(2*2,HW_Reg[2]);//reg45
	Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();

	Wire2_Spi0_Write_32Bit(1*2,HW_Reg[1]);//reg23

	Wire2_Spi0_Write_32Bit(3*2,HW_Reg[3]);//reg67

	Wire2_Spi0_Write_32Bit(3*2,0x5b8e2198);//reg67

	Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();

	Wire2_Spi0_Write_32Bit(3*2,0x5b8e2188);//reg67
	//DBG_PRINT("write=0x%x\r\n",0x5b8e2188);
	//temp = Wire2_Spi0_Read_32Bit(3*2);
	//DBG_PRINT("read =0x%x\r\n",temp);
	Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();
	//Wire2_Spi0_Write_32Bit(3*2,0x5b8e2198);//reg67,12M
	Wire2_Spi0_Write_32Bit(3*2,0x00402198);//reg67,32768

	Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();Delay1us();
	//Wire2_Spi0_Write_32Bit(2*2,0x10c0f00e);//reg45
	Wire2_Spi0_Write_32Bit(2*2,0x10c0800c);//reg45,xtal lower than 4M

	//R_SPI0_CTRL |= (1 << 15);	//spi  enable
	R_SPI0_CTRL = tmp;
}

/**************************************************
Function: SetFrequency
Description:
	ser a frequency
Parameter:
	freq:real frequency*10;such as 1077,977,
Return:
	None
**************************************************/
void SetFrequency(INT32U freq)
{
	INT32U tmp;
//	R_SPI0_CTRL &= ~(1 << 15);	//spi  disable

	tmp = R_SPI0_CTRL;
	R_SPI0_CTRL = 0;

//(frequency*10) *2^21 /38
	freq=freq<<21;

	freq=freq/38;

	Wire2_Spi0_Write_32Bit(0,freq);
//	R_SPI0_CTRL |= (1 << 15);	//spi  enable
	R_SPI0_CTRL = tmp;
}
/**************************************************
Function: AudioGain
Description:
	change voice volume
Parameter:
	volume:0-3
Return:
	None
**************************************************/
void AudioGain(INT8U volume)
{
	INT32U value;
	value=HW_Reg[2]&0xffffffcf; //clear bit5:bit4
//reg4<21:20>
	volume=volume<<4;
	value|=volume;

	Wire2_Spi0_Write_32Bit(2*2,value);

}

/**************************************************
Function: Chip_powerdown
Description:
	Power down chip,
	Please call AudioGain to set volume again when the next power up.

Parameter:
	none
Return:
	None
**************************************************/
void Chip_powerdown(void)
{
	INT32U value,tmp;
//	value=0x10F3830E;
//	value=0x10f3f00c;
	value=0x10F3830C;
	OSSchedLock();
//R_SPI0_CTRL &= ~(1 << 15);	//spi  disable
	tmp = R_SPI0_CTRL;
	R_SPI0_CTRL = 0;
	Wire2_Spi0_Write_32Bit(2*2,value);

//R_SPI0_CTRL |= (1 << 15);	//spi  enable
	R_SPI0_CTRL = tmp;
	OSSchedUnlock();
	fm_power = 0;
}
void  Chip_powerup(void)
{
//	OS_CPU_SR cpu_sr;	//wwj mark
	INT32U value,tmp;
	if(fm_power == 1){
		return;
	}
	//value=0x10F3830E;
//	value=0x10c0f00c;
	value=0x10c0800C;
	OSSchedLock();
//	OS_ENTER_CRITICAL();

	tmp = R_SPI0_CTRL;
	R_SPI0_CTRL = 0;
	Wire2_Spi0_Write_32Bit(2*2,value);

	R_SPI0_CTRL = tmp;
    OSSchedUnlock();
}

INT32U fm_tx_status_get(void)
{
	return fm_tx_ready;
}

#endif