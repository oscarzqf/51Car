/*
	���ܣ�STC89C52 + AD&DA ģ��ʵ�顣
	1���������ģ��I2C����ʱ������ADC/DAC
  2��������ͨ����ʱADC��DAC���
	����˵���������������ʾͨ��1��ͨ��3�Ĳ�����ֵ��ͬʱ���DAC���
	ע���������ADC��PCF8591��������ת��ͨ�������⣿��������ע�ͺ͡�
  ���ƣ����ӿƼ���ѧʵ������2022��6��
*/
#include<reg52.h>    //������Ƭ���Ĵ�����ͷ�ļ�

//-----------ģ��I2C���ߵıر����忪ʼ---------------
//����I2C����ʱ�ӵ���ʱֵ��Ҫ����ʵ������޸ģ�ȡֵ1��255
//SCL�ź�����ԼΪ(I2C_DELAY_VALUE*4+15)����������
#define I2C_DELAY_VALUE		10

//����I2C����ֹͣ������һ�ο�ʼ֮ǰ�ĵȴ�ʱ�䣬ȡֵ1��65535
//�ȴ�ʱ��ԼΪ(I2C_STOP_WAIT_VALUE*8)����������
//���ڶ�������ȡֵΪ1���ɣ�������ĳЩ������˵���ϳ�����ʱ�Ǳ����
#define I2C_STOP_WAIT_VALUE	1

//ģ��I2C���ߵ����Ŷ���
sbit I2C_SCL = P2^1;		//I2C  ʱ��
sbit I2C_SDA = P2^0;		//I2C  ����
sbit RST_DS = P2^4;   	//���Ϻ���Թص�DS1302ʱ��оƬ���

//����PCF8591 ��ַ
#define  AddWr 0x90
//-----------ģ��I2C���ߵıر��������---------------

//������ʾ���������ɶ�ʱ�жϳ����Զ�ɨ�裩
unsigned char DispBuf[8];

//����С���������ʾ��·�ܽŶ���
sbit SS = P2^6;	   //����ܶ�ѡ�ź�
sbit CS = P2^7;	   //�����λѡ�ź�

code unsigned char Tab[] =
	{//����0123456789AbCdEF��������������ݣ�������ʾ�ַ������м��㣬�确-������������Ϊ0x40
		0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
		0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
	};
	
//������ʱ���������ں�I2C_Delay()

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

/*
������T1INTSVC()
���ܣ���ʱ��T1���жϷ�����
*/
void T1INTSVC() interrupt 3	//��ʱ��1���жϺ�Ϊ��3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//��ʾλ�Ķ˿ڿ����ֽ�
	static unsigned char n = 0;						//n: ɨ����ʾλ����,0-7

//================����ܶ�ʱɨ��������ʾ===============	
	TR1 = 0;
	TH1 = 0xFC;
	TL1 = 0x66;						//���Խ�FC66����0000������ɨ���ٶȣ��۲����⶯̬ɨ��
	TR1 = 1;
	P0 = 0xFF;		//����
	CS = 1;
	CS = 0;
	P0 = DispBuf[n];	//����ɨ����ʾ����
	SS = 1;
	SS = 0;
	P0 = ~com[n];	//������ʾ
	CS = 1;
	CS = 0;
	n++;					 	//ָ����һλɨ����ʾ
	n &= 0x07;
//====================================================
}
/*
������DispClear()
���ܣ��������ܵ�������ʾ
*/
void DispClear()
{
	unsigned char i;
	for ( i=0; i<8; i++ )
	{
		DispBuf[i] = 0x00;	//iֵ��������ܵ�λ���������ں���ĳ���۲�������������0x00���Թر��������ʾ
	}
}
/*
������SysInit()
���ܣ�ϵͳ��ʼ��
*/
void SysInit()
{

	DispClear();	//��ʼΪȫ��

	TMOD = 0x11;	//���ö�ʱ��T0Ϊ16λ��ʱ������ʱ��T1Ϊ16λ��ʱ��
	EA = 0;
	TH1 = 0xFC;		//���ö�ʱ��1�ĳ�ֵ: 0xFC66����Ӧ��ʱʱ��1ms
	TL1 = 0x66;
	ET1 = 1;		//ʹ�ܶ�ʱ��T1�ж�
	EA = 1;			//ʹ�����ж�
	TR1 = 1;		//������ʱ��T1
}
void Display(unsigned char dat1,unsigned char dat2)

{

	DispBuf[7] = Tab[dat1 / 100];   		//��8λ���������ʾ��12345678������
	DispBuf[6] = Tab[dat1 /10 % 10];
	DispBuf[5] = Tab[dat1 % 10];

	DispBuf[2] = Tab[dat2 / 100];
	DispBuf[1] = Tab[dat2 /10 % 10];
	DispBuf[0] = Tab[dat2 % 10];
}
/*
������Delayms()
���ܣ���ʱ
˵����
	����Ƶ��Ϊ11.0592MHz
	��ʱ���� = 1ms * t
*/
void Delayms(unsigned int t)
{
	unsigned int us_ct;
	for (;t > 0;t --)		//ִ�д�������CPUʱ��
		for (us_ct = 113;us_ct > 0;us_ct --);	
}
/*------------------------------------------------
                    ������
------------------------------------------------*/
void main()
{  
	unsigned char light,voltage;//�����м����
	unsigned char chl2,chl4;//�����м����
	unsigned char AD_CHANNEL = 0;
	RST_DS = 0;											//		��ʱ��DS1302
	SysInit();
	I2C_Init();
	//������ʾ12345678�������������ʾ���
	DispBuf[7] = Tab[1];   		//��8λ���������ʾ��12345678������
	DispBuf[6] = Tab[2];
	DispBuf[5] = Tab[3];
	DispBuf[4] = Tab[4];
	DispBuf[3] = Tab[5];
	DispBuf[2] = Tab[6];
	DispBuf[1] = Tab[7];
	DispBuf[0] = Tab[8];
	Delayms(2000);					//����2s
	DispClear();				//�����ʾ��������ϵ���ʾ����ȫ��Ϩ��	
	while(1)
	{

	/********����AD-DA����*************/  
	/*����ADC��PCF8591��������������⣺
		A/Dת�����������ڷ���һ����Ч�Ķ�ģʽ��ַ��PCF8591�豸��ʼ��
		A/Dת��������Ӧ��ʱ������ĺ�Ե����������ÿ��ADC��ȡ������������һ��ת���Ľ����
		��ͨ��ת����ȡʱ������PCF8591_SendByte(AddWr,N)�е�Nָ����Ҫ��һ��ת����ͨ����
		�������ŵ�PCF8591_RcvByte(AddWr);��ȡ�Ĳ�������һ��ָʾ��ͨ��������ǰһ�εġ�
		���粻�Ƕ��ͨ����ʱת����ֻ����һ��ͨ������ôN��ֻ����Ҫת����ͨ�����ˡ�
	*/
		switch(AD_CHANNEL)
		{
			case 0: 
				PCF8591_SendByte(AddWr,1);
			  light = PCF8591_RcvByte(AddWr);  //ADC0 ģ��ת��1	 J8�Ͽɽ��������Ԫ��
			break;  
			
			case 1: 
				PCF8591_SendByte(AddWr,2);
			  chl2 =PCF8591_RcvByte(AddWr);  //ADC1  ģ��ת��2
			break;  
			
			case 2: 
				PCF8591_SendByte(AddWr,3);
				voltage =PCF8591_RcvByte(AddWr);  //ADC2	ģ��ת��3   �ɵ�����SW1
			break;  
			
			case 3: 
				PCF8591_SendByte(AddWr,0);
			  chl4 =PCF8591_RcvByte(AddWr);  //ADC3   ģ��ת��4	 �ɵ�����SW2
			break;
			
			case 4: 
				Pcf8591_DaConversion(AddWr,0, voltage); //DAC	  ��ģת��
			break;			
		}		
		
		if(++AD_CHANNEL > 4) AD_CHANNEL = 0;
	  Display(light,voltage);
		Delayms(50);
   }
}