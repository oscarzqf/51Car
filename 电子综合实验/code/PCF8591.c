#include <reg52.h>    //包含单片机寄存器的头文件
#include "PCF8591.h"
/*
函数：I2C_Delay()
功能：延时，模拟I2C总线专用
*/
void I2C_Delay()
{
	unsigned char I2C_Delay_t;
	I2C_Delay_t = (I2C_DELAY_VALUE);
	while ( --I2C_Delay_t != 0 );
}


/*
函数：I2C_Init()
功能：I2C总线初始化，使总线处于空闲状态
说明：在main()函数的开始处，通常应当要执行一次本函数
*/
void I2C_Init()
{
	I2C_SCL = 1;
	I2C_Delay();
	I2C_SDA = 1;
	I2C_Delay();
}


/*
函数：I2C_Start()
功能：产生I2C总线的起始状态
说明：
	SCL处于高电平期间，当SDA出现下降沿时启动I2C总线
	不论SDA和SCL处于什么电平状态，本函数总能正确产生起始状态
	本函数也可以用来产生重复起始状态
	本函数执行后，I2C总线处于忙状态
*/
void I2C_Start()
{
	I2C_SDA = 1;
	I2C_Delay();
	I2C_SCL = 1;
	I2C_Delay();
	I2C_SDA = 0;
	I2C_Delay();
	I2C_SCL = 0;
	I2C_Delay();
}

/***********************************************************
函数：I2C_Stop()
功能：产生I2C总线的停止状态
说明：
	SCL处于高电平期间，当SDA出现上升沿时停止I2C总线
	不论SDA和SCL处于什么电平状态，本函数总能正确产生停止状态
	本函数执行后，I2C总线处于空闲状态
***********************************************************/
void I2C_Stop()
{
	unsigned int t = I2C_STOP_WAIT_VALUE;
	I2C_SDA = 0;
	I2C_Delay();
	I2C_SCL = 1;
	I2C_Delay();
	I2C_SDA = 1;
	I2C_Delay();
	while ( --t != 0 );		//在下一次产生Start之前，要加一定的延时
}

/*
函数：I2C_Write()
功能：向I2C总线写1个字节的数据
参数：
	dat：要写到总线上的数据
*/
void I2C_Write(unsigned char dat)
{
	unsigned char t = 8;
	do
	{
		I2C_SDA = (bit)(dat & 0x80);
		dat <<= 1;
		I2C_SCL = 1;
		I2C_Delay();
		I2C_SCL = 0;
		I2C_Delay();
	} while ( --t != 0 );
}


/*
函数：I2C_Read()
功能：从从机读取1个字节的数据
返回：读取的一个字节数据
*/
unsigned char I2C_Read()
{
	unsigned char dat;
	unsigned char t = 8;
	I2C_SDA = 1;	//在读取数据之前，要把SDA拉高
	do
	{
		I2C_SCL = 1;
		I2C_Delay();
		dat <<= 1;
		if ( I2C_SDA ) dat |= 0x01;
		I2C_SCL = 0;
		I2C_Delay();
	} while ( --t != 0 );
	return dat;
}


/*
函数：I2C_GetAck()
功能：读取从机应答位
返回：
	0：从机应答
	1：从机非应答
说明：
	从机在收到每个字节的数据后，要产生应答位
	从机在收到最后1个字节的数据后，一般要产生非应答位
*/
bit I2C_GetAck()
{
	bit ack;
	I2C_SDA = 1;
	I2C_Delay();
	I2C_SCL = 1;
	I2C_Delay();
	ack = I2C_SDA;
	I2C_SCL = 0;
	I2C_Delay();
	return ack;
}


/*
函数：I2C_PutAck()
功能：主机产生应答位或非应答位
参数：
	ack=0：主机产生应答位
	ack=1：主机产生非应答位
说明：
	主机在接收完每一个字节的数据后，都应当产生应答位
	主机在接收完最后一个字节的数据后，应当产生非应答位
*/
void I2C_PutAck(bit ack)
{
	I2C_SDA = ack;
	I2C_Delay();
	I2C_SCL = 1;
	I2C_Delay();
	I2C_SCL = 0;
	I2C_Delay();
}

/************************************************************
* 函数名				: Pcf8591_DaConversion
* 函数功能			: PCF8591的输出端输出模拟量
* 输入				: addr（器件地址），channel（转换通道），value（转换的数值）
* 输出				: 无
******************* *****************************************/
bit Pcf8591_DaConversion(unsigned char addr,unsigned char channel,  unsigned char Val)
{
	I2C_Start();              //启动总线
	I2C_Write(addr);            //发送器件地址
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
   I2C_Write(0x40|channel);              //发送控制字节
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Write(Val);            //发送DAC的数值  
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Stop();               //结束总线
  return 0;
}

/************************************************************
* 函数名			: Pcf8591_SendByte
* 函数功能		: 写入一个控制命令
* 输入			: addr（器件地址），channel（转换通道）
* 输出			: 无
************************************************************/
bit PCF8591_SendByte(unsigned char addr,unsigned char channel)
{
  I2C_Start();              //启动总线
  I2C_Write(addr);            //发送器件地址
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Write(0x40|channel);              //发送控制字节
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Stop();               //结束总线
  return 0;
}

/************************************************************
* 函数名       	: PCF8591_RcvByte
* 函数功能   	: 读取一个转换值
* 输入          :
* 输出          : dat
************************************************************/
unsigned char PCF8591_RcvByte(unsigned char addr)
{  
  unsigned char dat;

  I2C_Start();          //启动总线
  I2C_Write(addr+1);      //发送器件地址
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
	dat=I2C_Read();         //读取数据0

  I2C_PutAck(1);           //发送非应答信号
  I2C_Stop();           //结束总线
  return(dat);
}
