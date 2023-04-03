/*
	PCF8591.h
	STC89C52单片机驱动PCF8591的程序头文件
*/
#ifndef _PCF8591_H_
#define _PCF8591_H_

#include <reg52.h>
//-----------模拟I2C总线的必备定义---------------

//定义I2C总线时钟的延时值，要根据实际情况修改，取值1～255
//SCL信号周期约为(I2C_DELAY_VALUE*4+15)个机器周期
#define I2C_DELAY_VALUE		10

//定义I2C总线停止后在下一次开始之前的等待时间，取值1～65535
//等待时间约为(I2C_STOP_WAIT_VALUE*8)个机器周期
//对于多数器件取值为1即可；但对于某些器件来说，需要较长的延时
#define I2C_STOP_WAIT_VALUE	1

//模拟I2C总线的引脚定义
sbit I2C_SCL = P2^1;		//I2C  时钟
sbit I2C_SDA = P2^0;		//I2C  数据
sbit RST_DS = P2^4;   	//加上后可以关掉DS1302时钟芯片输出

//定义PCF8591 地址
#define  AddWr 0x90
//-----------模拟I2C总线的必备定义结束---------------

//-----------子函数声明-------------
void I2C_Delay();

void I2C_Init();

void I2C_Start();

void I2C_Stop();

void I2C_Write(unsigned char dat);

unsigned char I2C_Read();

bit I2C_GetAck();

void I2C_PutAck(bit ack);

bit Pcf8591_DaConversion(unsigned char addr,unsigned char channel,  unsigned char Val);

bit PCF8591_SendByte(unsigned char addr,unsigned char channel);

unsigned char PCF8591_RcvByte(unsigned char addr);


#endif	//_PCF8591_H_
