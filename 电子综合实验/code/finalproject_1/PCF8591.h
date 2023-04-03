/*
	PCF8591.h
	STC89C52��Ƭ������PCF8591�ĳ���ͷ�ļ�
*/
#ifndef _PCF8591_H_
#define _PCF8591_H_

#include <reg52.h>
//-----------ģ��I2C���ߵıر�����---------------

//����I2C����ʱ�ӵ���ʱֵ��Ҫ����ʵ������޸ģ�ȡֵ1��255
//SCL�ź�����ԼΪ(I2C_DELAY_VALUE*4+15)����������
#define I2C_DELAY_VALUE		10

//����I2C����ֹͣ������һ�ο�ʼ֮ǰ�ĵȴ�ʱ�䣬ȡֵ1��65535
//�ȴ�ʱ��ԼΪ(I2C_STOP_WAIT_VALUE*8)����������
//���ڶ�������ȡֵΪ1���ɣ�������ĳЩ������˵����Ҫ�ϳ�����ʱ
#define I2C_STOP_WAIT_VALUE	1

//ģ��I2C���ߵ����Ŷ���
sbit I2C_SCL = P2^1;		//I2C  ʱ��
sbit I2C_SDA = P2^0;		//I2C  ����
sbit RST_DS = P2^4;   	//���Ϻ���Թص�DS1302ʱ��оƬ���

//����PCF8591 ��ַ
#define  AddWr 0x90
//-----------ģ��I2C���ߵıر��������---------------

//-----------�Ӻ�������-------------
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
