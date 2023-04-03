/*
	main.c
	用PWM方波控制直流电机的速度。电机驱动接线及驱动逻辑控制参见"电机测试"例程。
	本例程通过主板上K1~K4独立按键，调节PWM占空比，从而调节左右电机转速。
*/

#include <reg52.h>


//定义I/O接口
sbit PWM_IN1 = P1^4; 	// 高电平1：左电机后退（反转）
sbit PWM_IN2 = P1^5; 	// 高电平1：左电机前进（正转）

sbit PWM_IN3 = P1^6; 	// 高电平1：右电机前进（正转）	
sbit PWM_IN4 = P1^7; 	// 高电平1：右电机后退（反转）

sbit PWM_EN1 = P1^2; 	// 高电平1：使能左电机 
sbit PWM_EN2 = P1^3; 	// 高电平1：使能右电机

sbit KEY1 = P3^4;		//定义按键K1，左'+'按键（控制左电机加速），对应核心板上K1
sbit KEY2 = P3^5;		//定义按键K2，左'-'按键（控制左电机减速），对应核心板上K2
sbit KEY3 = P3^6;		//定义按键K3，右'+'按键（控制左电机加速），对应核心板上K3
sbit KEY4 = P3^7;		//定义按键K4，右'+'按键（控制左电机减速），对应核心板上K4

//定义PWM最大级数，也就是调节直流电机的速度等级
#define SPEED_MAX		20

//定义PWM级数，分为0～SPEED_MAX-1级
unsigned char Speed_L;		//左电机转速调节（调节PWM的一个周期SPEED_MAX*1ms时间内，左电机正转时间：Speed_L*1ms）
unsigned char Speed_R;		//右电机转速调节（调节PWM的一个周期SPEED_MAX*1ms时间内，右电机正转时间：Speed_R*1ms）

//定义显示缓冲区（由定时中断程序自动扫描）
unsigned char DispBuf[8];

//智能小车数码管显示电路
sbit SS = P2^6;	   //数码管段选信号
sbit CS = P2^7;	   //数码管位选信号

code unsigned char Tab[] =		//共阴极数码管的段码（字形）表
	{//定义0123456789AbCdEF的数码管字型数据，其他显示字符需自行计算，如‘-’的字形数据为0x40
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
	PWM_EN2 = 1;			//电机有效

	while(1)
	{
		Delay(50);			//延时50ms
		k = KeyScan();		//键盘扫描
		if ( k != '\0' ) 
		{
			switch ( k )	//处理按键
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

			while( KeyScan() != '\0');		//等待松开按键
		}
	}
}


/*
函数：KeyScan()
功能：键盘扫描
返回：扫描到的键值
*/
unsigned char KeyScan()
{
	unsigned char k;

	k = '\0';
	if ( KEY1 == 0 ) k = 'A';		//k1 左'+'
	if ( KEY2 == 0 ) k = 'B';		//k2 左'-'
	if ( KEY3 == 0 ) k = 'C';		//k3 右'+'
	if ( KEY4 == 0 ) k = 'D';		//k4 右'-'

	return k;
}

/*
函数：DispClear()
功能：清除数码管的所有显示
*/
void DispClear()
{
	unsigned char i;
	for ( i=0; i<8; i++ )
	{
		DispBuf[i] = 0x00;		//8位数码管显示全熄灭
	}
}

/*
函数：DispInit()
功能：数码管扫描显示初始化
*/
void DispInit()
{
	DispClear();	//初始为全灭
	EA = 0;
	TH1 = 0xFC;
	TL1 = 0x66;
	TR1 = 1;
	ET1 = 1;
	EA = 1;
}

/*
函数：Delay()
功能：延时
说明：
	晶振频率为11.0592MHz
	延时长度 = 1ms * t
*/
void Delay(unsigned int t)
{
	unsigned int us_ct;
	for (;t > 0;t --)		//执行代码消耗CPU时间
		for (us_ct = 113;us_ct > 0;us_ct --);	
}


/*
函数：SysInit()
功能：系统初始化
*/
void SysInit()
{
	TMOD = 0x11;		//设置T1为16位定时器

	PWM_EN1 = 0;
	PWM_EN2 = 0;		//电机无效
	PWM_IN1 = 0;
	PWM_IN2 = 0; 		//左电机的停转
	PWM_IN3 = 0;
	PWM_IN4 = 0;		//右电机的停转

	Speed_L = 6;		//设置左电机PWM初值（初始转速）
	Speed_R = 6;		//设置右电机PWM初值

	DispInit();

}

/*
函数：T1_INT_SVC()
功能：定时器T1的中断服务函数，定时时间1ms
*/
void T1INTSVC() interrupt 3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//显示位的端口控制字节
	static unsigned char n = 0;	//n: 扫描显示位计数,0-7
	static unsigned char t = 0;

	TR1 = 0;
	TH1 = 0xFC;					//设置定时器初值0xfc66，对应1ms定时中断时间
	TL1 = 0x66;					//
	TR1 = 1;

	P0 = 0xFF;				//消隐（通过控制8位数码管的位控制端，即COM端，让其全不亮）
	CS = 1;
	CS = 0;

	P0 = DispBuf[n];	//更新扫描显示数据
	SS = 1;
	SS = 0;

	P0 = ~com[n];			//重新显示
	CS = 1;
	CS = 0;

	n++;					 	//指向下一位扫描显示
	n &= 0x07;
	
	t++;
	if ( t >= SPEED_MAX ) 
		t = 0;				//PWM波的周期为：SPEED_MAX*1ms = 20ms

	if ( t < Speed_L )		//PWM波高电平时间：(Speed_L)*1ms
	{
    PWM_IN1 = 0;
		PWM_IN2 = 1; 		//左电机的正转
	}
	else					//PWM波低电平时间：（SPEED_MAX-Speed_L）*1ms
	{
    PWM_IN1 = 0;
		PWM_IN2 = 0; 		//左电机的停转
	}
	
	if ( t < Speed_R )		//PWM波高电平时间：Speed_R*1ms
	{
		PWM_IN3 = 1;
		PWM_IN4 = 0;		//右电机的正转		
	}
	else					//PWM波低电平时间：（SPEED_MAX-Speed_R）*1ms
	{
		PWM_IN3 = 0;
		PWM_IN4 = 0;		//右电机的停转	
	}
}

