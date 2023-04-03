/*
	main.c
	ͨ��k1��k2��������PWMռ�ձȣ�����PWM��������LED�����ȣ�PWM����=16ms
	���ȵȼ���0-15, 16������ʾ������ܵ����2λ
*/


#include <reg52.h>

//������ʾ���������ɶ�ʱ�жϳ����Զ�ɨ�裩
unsigned char DispBuf[8];

//����С���������ʾ��·�ܽŶ���
sbit SS = P2^6;	   //����ܶ�ѡ�ź�
sbit CS = P2^7;	   //�����λѡ�ź�
//����С������ܰ�����·�ܽŶ���
sbit KEY1 = P3^4;		//���尴��1��'+'��������Ӧ���İ���K1
sbit KEY2 = P3^5;		//���尴��2��'-'��������Ӧ���İ���K2
code unsigned char Tab[] =
	{//����0123456789AbCdEF��������������ݣ�������ʾ�ַ������м��㣬�确-������������Ϊ0x40
		0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
		0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
	};
	
//����I/O�ӿ�
sbit PWM = P1^0;		//PWM�������LED��
	
//����PWM�������Ҳ���ǵ���LED�����ȵȼ�
#define PWM_MAX		16
//����PWM��������Ϊ0��PWM_MAX-1��
unsigned char PwmValue;
	
/*
������T1INTSVC()
���ܣ���ʱ��T1���жϷ�����
*/
void T1INTSVC() interrupt 3	//��ʱ��1���жϺ�Ϊ��3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//��ʾλ�Ķ˿ڿ����ֽ�
	static unsigned char n = 0;						//n: ɨ����ʾλ����,0-7
	static unsigned char t = 0;

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
//==========���ö�ʱ�ж�ʵ���������====================
	t++;												//ע��t�Ķ��巽ʽ
	if ( t >= PWM_MAX ) t = 0;	//PWM��������Ϊ��PWM_MAX*1ms = 16ms
	if ( t < PwmValue )
	{
		PWM = 0;		//PWM���͵�ƽʱ��(LED��)��PwmValue*1ms
	}
	else
	{
		PWM = 1;		//PWM���ߵ�ƽʱ��(LED��)����PWM_MAX-PwmValue��*1ms
	}
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
������Delay()
���ܣ���ʱ
˵����
	����Ƶ��Ϊ11.0592MHz
	��ʱ���� = 1ms * t
*/
void Delay(unsigned int t)
{
	unsigned int us_ct;
	for (;t > 0;t --)		//ִ�д�������CPUʱ��
		for (us_ct = 113;us_ct > 0;us_ct --);	
}

/*
������KeyScan()
���ܣ�����ɨ��
���أ�ɨ�赽�ļ�ֵ
*/
unsigned char KeyScan()
{
	unsigned char k = '\0';

	if ( KEY1 == 0 ) k = '+';
	if ( KEY2 == 0 ) k = '-';

	return k;
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


void main()
{

	unsigned char k;					//�����ֵ����
	unsigned char cnt = 18;		//�������ֵ�����ĳ�ʼֵ
	
	SysInit();								//ϵͳ��ʼ��
	PwmValue = PWM_MAX / 2;		//����PWM��ֵ���м�ȼ����ȣ�
	PWM = 1;
	
	DispBuf[7] = Tab[1];   		//��8λ���������ʾ��12345678������
	DispBuf[6] = Tab[2];
	DispBuf[5] = Tab[3];
	DispBuf[4] = Tab[4];
	DispBuf[3] = Tab[5];
	DispBuf[2] = Tab[6];
	DispBuf[1] = Tab[7];
	DispBuf[0] = Tab[8];
	Delay(2000);					//����1s

	DispClear();				//�����ʾ��������ϵ���ʾ����ȫ��Ϩ��
	DispBuf[1] = Tab[PwmValue / 10];
	DispBuf[0] = Tab[PwmValue % 10];

	while(1)
	{
		Delay(20);
		k = KeyScan();	//����ɨ��
		if ( k != '\0')
		{
			if ( k == '+') 
			{
				if ( PwmValue < PWM_MAX-1 ) PwmValue++;
			}
			if ( k == '-')
			{
				if ( PwmValue > 0 ) PwmValue--;
			}
			
			DispBuf[1] = Tab[PwmValue / 10];
			DispBuf[0] = Tab[PwmValue % 10];

			while( KeyScan() != '\0');			//�ȴ��ɿ�����
		}
	}
}

