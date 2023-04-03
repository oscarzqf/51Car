/*
	main.c
	��PWM��������ֱ��������ٶȡ�����������߼������߼����Ʋμ�"�������"���̡�
	������ͨ��������K1~K4��������������PWMռ�ձȣ��Ӷ��������ҵ��ת�١�
*/

#include <reg52.h>


//����I/O�ӿ�
sbit PWM_IN1 = P1^4; 	// �ߵ�ƽ1���������ˣ���ת��
sbit PWM_IN2 = P1^5; 	// �ߵ�ƽ1������ǰ������ת��

sbit PWM_IN3 = P1^6; 	// �ߵ�ƽ1���ҵ��ǰ������ת��	
sbit PWM_IN4 = P1^7; 	// �ߵ�ƽ1���ҵ�����ˣ���ת��

sbit PWM_EN1 = P1^2; 	// �ߵ�ƽ1��ʹ������ 
sbit PWM_EN2 = P1^3; 	// �ߵ�ƽ1��ʹ���ҵ��

sbit KEY1 = P3^4;		//���尴��K1����'+'�����������������٣�����Ӧ���İ���K1
sbit KEY2 = P3^5;		//���尴��K2����'-'�����������������٣�����Ӧ���İ���K2
sbit KEY3 = P3^6;		//���尴��K3����'+'�����������������٣�����Ӧ���İ���K3
sbit KEY4 = P3^7;		//���尴��K4����'+'�����������������٣�����Ӧ���İ���K4

//����PWM�������Ҳ���ǵ���ֱ��������ٶȵȼ�
#define SPEED_MAX		20

//����PWM��������Ϊ0��SPEED_MAX-1��
unsigned char Speed_L;		//����ת�ٵ��ڣ�����PWM��һ������SPEED_MAX*1msʱ���ڣ�������תʱ�䣺Speed_L*1ms��
unsigned char Speed_R;		//�ҵ��ת�ٵ��ڣ�����PWM��һ������SPEED_MAX*1msʱ���ڣ��ҵ����תʱ�䣺Speed_R*1ms��

//������ʾ���������ɶ�ʱ�жϳ����Զ�ɨ�裩
unsigned char DispBuf[8];

//����С���������ʾ��·
sbit SS = P2^6;	   //����ܶ�ѡ�ź�
sbit CS = P2^7;	   //�����λѡ�ź�

code unsigned char Tab[] =		//����������ܵĶ��루���Σ���
	{//����0123456789AbCdEF��������������ݣ�������ʾ�ַ������м��㣬�确-������������Ϊ0x40
		0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
		0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
	};


unsigned char KeyScan();
void DispClear();
void DispInit();
void Delay(unsigned int t);
void SysInit();

void main()
{
	unsigned char k;

	SysInit();

	DispBuf[7] = 0x40;			//-
	DispBuf[6] = Tab[Speed_R / 10];
	DispBuf[5] = Tab[Speed_R % 10];
	DispBuf[4] = 0x40;			//-
	DispBuf[3] = 0x40;			//-
	DispBuf[2] = Tab[Speed_L / 10];
	DispBuf[1] = Tab[Speed_L % 10];
	DispBuf[0] = 0x40;			//-
	PWM_EN1 = 1;
	PWM_EN2 = 1;			//�����Ч

	while(1)
	{
		Delay(50);			//��ʱ50ms
		k = KeyScan();		//����ɨ��
		if ( k != '\0' ) 
		{
			switch ( k )	//������
			{
			case 'A':
				if ( Speed_L < SPEED_MAX-1 ) Speed_L ++;
				break;
			case 'B':
				if ( Speed_L > 0 ) Speed_L --;
				break;
			case 'C':
				if ( Speed_R < SPEED_MAX-1 ) Speed_R ++;
				break;
			case 'D':
				if ( Speed_R > 0 ) Speed_R --;
				break;
			default:
				break;
			}

			DispBuf[6] = Tab[Speed_L / 10];
			DispBuf[5] = Tab[Speed_L % 10];
			DispBuf[2] = Tab[Speed_R / 10];
			DispBuf[1] = Tab[Speed_R % 10];

			while( KeyScan() != '\0');		//�ȴ��ɿ�����
		}
	}
}


/*
������KeyScan()
���ܣ�����ɨ��
���أ�ɨ�赽�ļ�ֵ
*/
unsigned char KeyScan()
{
	unsigned char k;

	k = '\0';
	if ( KEY1 == 0 ) k = 'A';		//k1 ��'+'
	if ( KEY2 == 0 ) k = 'B';		//k2 ��'-'
	if ( KEY3 == 0 ) k = 'C';		//k3 ��'+'
	if ( KEY4 == 0 ) k = 'D';		//k4 ��'-'

	return k;
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
		DispBuf[i] = 0x00;		//8λ�������ʾȫϨ��
	}
}

/*
������DispInit()
���ܣ������ɨ����ʾ��ʼ��
*/
void DispInit()
{
	DispClear();	//��ʼΪȫ��
	EA = 0;
	TH1 = 0xFC;
	TL1 = 0x66;
	TR1 = 1;
	ET1 = 1;
	EA = 1;
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
������SysInit()
���ܣ�ϵͳ��ʼ��
*/
void SysInit()
{
	TMOD = 0x11;		//����T1Ϊ16λ��ʱ��

	PWM_EN1 = 0;
	PWM_EN2 = 0;		//�����Ч
	PWM_IN1 = 0;
	PWM_IN2 = 0; 		//������ͣת
	PWM_IN3 = 0;
	PWM_IN4 = 0;		//�ҵ����ͣת

	Speed_L = 6;		//��������PWM��ֵ����ʼת�٣�
	Speed_R = 6;		//�����ҵ��PWM��ֵ

	DispInit();

}

/*
������T1_INT_SVC()
���ܣ���ʱ��T1���жϷ���������ʱʱ��1ms
*/
void T1INTSVC() interrupt 3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//��ʾλ�Ķ˿ڿ����ֽ�
	static unsigned char n = 0;	//n: ɨ����ʾλ����,0-7
	static unsigned char t = 0;

	TR1 = 0;
	TH1 = 0xFC;					//���ö�ʱ����ֵ0xfc66����Ӧ1ms��ʱ�ж�ʱ��
	TL1 = 0x66;					//
	TR1 = 1;

	P0 = 0xFF;				//������ͨ������8λ����ܵ�λ���ƶˣ���COM�ˣ�����ȫ������
	CS = 1;
	CS = 0;

	P0 = DispBuf[n];	//����ɨ����ʾ����
	SS = 1;
	SS = 0;

	P0 = ~com[n];			//������ʾ
	CS = 1;
	CS = 0;

	n++;					 	//ָ����һλɨ����ʾ
	n &= 0x07;
	
	t++;
	if ( t >= SPEED_MAX ) 
		t = 0;				//PWM��������Ϊ��SPEED_MAX*1ms = 20ms

	if ( t < Speed_L )		//PWM���ߵ�ƽʱ�䣺(Speed_L)*1ms
	{
    PWM_IN1 = 0;
		PWM_IN2 = 1; 		//��������ת
	}
	else					//PWM���͵�ƽʱ�䣺��SPEED_MAX-Speed_L��*1ms
	{
    PWM_IN1 = 0;
		PWM_IN2 = 0; 		//������ͣת
	}
	
	if ( t < Speed_R )		//PWM���ߵ�ƽʱ�䣺Speed_R*1ms
	{
		PWM_IN3 = 1;
		PWM_IN4 = 0;		//�ҵ������ת		
	}
	else					//PWM���͵�ƽʱ�䣺��SPEED_MAX-Speed_R��*1ms
	{
		PWM_IN3 = 0;
		PWM_IN4 = 0;		//�ҵ����ͣת	
	}
}

