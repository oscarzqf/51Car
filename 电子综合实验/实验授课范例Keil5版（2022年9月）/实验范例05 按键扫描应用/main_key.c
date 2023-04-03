/*
	功能：按键扫描及按键加减数值显示。
	1、按键消抖原理
	2、按键抬起确认
	编制：电子科技大学实验中心2022年6月
*/


#include <reg52.h>

//定义显示缓冲区（由定时中断程序自动扫描）
unsigned char DispBuf[8];

//智能小车数码管显示电路管脚定义
sbit SS = P2^6;	   //数码管段选信号
sbit CS = P2^7;	   //数码管位选信号
//智能小车数码管按键电路管脚定义
sbit KEY1 = P3^4;		//定义按键1，'+'按键，对应核心板上K1
sbit KEY2 = P3^5;		//定义按键2，'-'按键，对应核心板上K2
code unsigned char Tab[] =
	{//定义0123456789AbCdEF的数码管字型数据，其他显示字符需自行计算，如‘-’的字形数据为0x40
		0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
		0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
	};
	

/*
函数：T1INTSVC()
功能：定时器T1的中断服务函数
*/
void T1INTSVC() interrupt 3	//定时器1的中断号为：3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//显示位的端口控制字节
	static unsigned char n = 0;						//n: 扫描显示位计数,0-7

//================数码管定时扫描驱动显示===============	
	TR1 = 0;
	TH1 = 0xFC;
	TL1 = 0x66;						//可以将FC66换成0000，降低扫描速度，观察和理解动态扫描
	TR1 = 1;
	P0 = 0xFF;		//消隐
	CS = 1;
	CS = 0;
	P0 = DispBuf[n];	//更新扫描显示数据
	SS = 1;
	SS = 0;
	P0 = ~com[n];	//重新显示
	CS = 1;
	CS = 0;
	n++;					 	//指向下一位扫描显示
	n &= 0x07;
//====================================================

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
		DispBuf[i] = 0x00;	//i值代表数码管的位数，可以在后面的程序观察是左起还是右起，0x00可以关闭数码管显示
	}
}

/*
函数：Delay()
功能：延时
说明：
	晶振频率为11.0592MHz
	延时长度 = 1ms * t
*/
void Delayms(unsigned int t)
{
	unsigned int us_ct;
	for (;t > 0;t --)		//执行代码消耗CPU时间
		for (us_ct = 113;us_ct > 0;us_ct --);	
}

/*
函数：KeyScan()
功能：键盘扫描
返回：扫描到的键值
*/
unsigned char KeyScan()
{
	unsigned char k = '\0';

	if ( KEY1 == 0 ) k = '+';
	if ( KEY2 == 0 ) k = '-';

	return k;
}

/*
函数：SysInit()
功能：系统初始化
*/
void SysInit()
{

	DispClear();	//初始为全灭

	TMOD = 0x11;	//设置定时器T0为16位定时器，定时器T1为16位定时器
	EA = 0;
	TH1 = 0xFC;		//设置定时器1的初值: 0xFC66，对应定时时间1ms
	TL1 = 0x66;
	ET1 = 1;		//使能定时器T1中断
	EA = 1;			//使能总中断
	TR1 = 1;		//启动定时器T1
}


void main()
{

	unsigned char k;			//定义键值变量
	unsigned char cnt = 18;		//定义计数值变量的初始值

	SysInit();					//系统初始化

	DispBuf[7] = Tab[1];   		//在8位数码管上显示“12345678”字样
	DispBuf[6] = Tab[2];
	DispBuf[5] = Tab[3];
	DispBuf[4] = Tab[4];
	DispBuf[3] = Tab[5];
	DispBuf[2] = Tab[6];
	DispBuf[1] = Tab[7];
	DispBuf[0] = Tab[8];
	Delayms(2000);					//保留2s
	

	DispClear();				//清除显示（数码管上的显示内容全部熄灭
	DispBuf[1] = Tab[cnt / 10];
	DispBuf[0] = Tab[cnt % 10];
	while(1)
	{
		Delayms(20);				//扫描延时20ms，也做按键后延消抖
		k = KeyScan();			//扫描按键

		if ( k != '\0')			//首次检测到按键按下
		{
			Delayms(20);					//延时20ms，按键前沿消抖
			k = KeyScan();			//再次读取按键状态
			if ( k != '\0')			//确认按键按下后处理按键
			{
				if ( k == '+') 
				{
					if ( cnt < 31 ) cnt++;
				}
				if ( k == '-')
				{
					if ( cnt > 0 ) cnt--;
				}
				
				DispBuf[1] = Tab[cnt / 10];
				DispBuf[0] = Tab[cnt % 10];

				while( KeyScan() != '\0');		//等待松开按键
			}
		}
	}
}
//思考：等待按键抬起的处理方式带来什么优势？本程序的对应语句存在什么隐患？

