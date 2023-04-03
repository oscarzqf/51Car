#include <reg52.h>    //������Ƭ���Ĵ�����ͷ�ļ�
#include "PCF8591.h"
/*
������I2C_Delay()
���ܣ���ʱ��ģ��I2C����ר��
*/
void I2C_Delay()
{
	unsigned char I2C_Delay_t;
	I2C_Delay_t = (I2C_DELAY_VALUE);
	while ( --I2C_Delay_t != 0 );
}


/*
������I2C_Init()
���ܣ�I2C���߳�ʼ����ʹ���ߴ��ڿ���״̬
˵������main()�����Ŀ�ʼ����ͨ��Ӧ��Ҫִ��һ�α�����
*/
void I2C_Init()
{
	I2C_SCL = 1;
	I2C_Delay();
	I2C_SDA = 1;
	I2C_Delay();
}


/*
������I2C_Start()
���ܣ�����I2C���ߵ���ʼ״̬
˵����
	SCL���ڸߵ�ƽ�ڼ䣬��SDA�����½���ʱ����I2C����
	����SDA��SCL����ʲô��ƽ״̬��������������ȷ������ʼ״̬
	������Ҳ�������������ظ���ʼ״̬
	������ִ�к�I2C���ߴ���æ״̬
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
������I2C_Stop()
���ܣ�����I2C���ߵ�ֹͣ״̬
˵����
	SCL���ڸߵ�ƽ�ڼ䣬��SDA����������ʱֹͣI2C����
	����SDA��SCL����ʲô��ƽ״̬��������������ȷ����ֹͣ״̬
	������ִ�к�I2C���ߴ��ڿ���״̬
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
	while ( --t != 0 );		//����һ�β���Start֮ǰ��Ҫ��һ������ʱ
}

/*
������I2C_Write()
���ܣ���I2C����д1���ֽڵ�����
������
	dat��Ҫд�������ϵ�����
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
������I2C_Read()
���ܣ��Ӵӻ���ȡ1���ֽڵ�����
���أ���ȡ��һ���ֽ�����
*/
unsigned char I2C_Read()
{
	unsigned char dat;
	unsigned char t = 8;
	I2C_SDA = 1;	//�ڶ�ȡ����֮ǰ��Ҫ��SDA����
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
������I2C_GetAck()
���ܣ���ȡ�ӻ�Ӧ��λ
���أ�
	0���ӻ�Ӧ��
	1���ӻ���Ӧ��
˵����
	�ӻ����յ�ÿ���ֽڵ����ݺ�Ҫ����Ӧ��λ
	�ӻ����յ����1���ֽڵ����ݺ�һ��Ҫ������Ӧ��λ
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
������I2C_PutAck()
���ܣ���������Ӧ��λ���Ӧ��λ
������
	ack=0����������Ӧ��λ
	ack=1������������Ӧ��λ
˵����
	�����ڽ�����ÿһ���ֽڵ����ݺ󣬶�Ӧ������Ӧ��λ
	�����ڽ��������һ���ֽڵ����ݺ�Ӧ��������Ӧ��λ
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
* ������				: Pcf8591_DaConversion
* ��������			: PCF8591����������ģ����
* ����				: addr��������ַ����channel��ת��ͨ������value��ת������ֵ��
* ���				: ��
******************* *****************************************/
bit Pcf8591_DaConversion(unsigned char addr,unsigned char channel,  unsigned char Val)
{
	I2C_Start();              //��������
	I2C_Write(addr);            //����������ַ
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
   I2C_Write(0x40|channel);              //���Ϳ����ֽ�
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Write(Val);            //����DAC����ֵ  
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Stop();               //��������
  return 0;
}

/************************************************************
* ������			: Pcf8591_SendByte
* ��������		: д��һ����������
* ����			: addr��������ַ����channel��ת��ͨ����
* ���			: ��
************************************************************/
bit PCF8591_SendByte(unsigned char addr,unsigned char channel)
{
  I2C_Start();              //��������
  I2C_Write(addr);            //����������ַ
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Write(0x40|channel);              //���Ϳ����ֽ�
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Stop();               //��������
  return 0;
}

/************************************************************
* ������       	: PCF8591_RcvByte
* ��������   	: ��ȡһ��ת��ֵ
* ����          :
* ���          : dat
************************************************************/
unsigned char PCF8591_RcvByte(unsigned char addr)
{  
  unsigned char dat;

  I2C_Start();          //��������
  I2C_Write(addr+1);      //����������ַ
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
	dat=I2C_Read();         //��ȡ����0

  I2C_PutAck(1);           //���ͷ�Ӧ���ź�
  I2C_Stop();           //��������
  return(dat);
}
