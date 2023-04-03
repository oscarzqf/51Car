/*
	功能：数码管的动态扫描显示。
	1、分别用两种办法实现秒表：循环延时，定时中断
	2、学会正计时、倒计时、计时暂停、循环计时
	编制：电子科技大学实验中心2022年6月
*/


#include <reg52.h>

//定义显示缓冲区（由定时中断程序自动扫描）
unsigned char DispBuf[8];

//智能小车数码管显示电路管脚定义
sbit SS = P2^6;	   //数码管段选信号
sbit CS = P2^7;	   //数码管位选信号

code unsigned char Tab[] =
	{//定义0123456789AbCdEF的数码管字型数据，其他显示字符需自行计算，如‘-’的字形数据为0x40
		0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
		0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
	};
	
unsigned char sec = 0;		//100ms计数
bit timer_start_flag = 0;	//定义标志位，控制定时中断跑表的功能是否工作

/*
函数：T1INTSVC()
功能：定时器T1的中断服务函数
*/
void T1INTSVC() interrupt 3	//定时器1的中断号为：3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//显示位的端口控制字节
	static unsigned char n = 0;						//n: 扫描显示位计数,0-7
	static unsigned int counter_1ms = 0;	//1ms计数
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
//==================精度0.1秒的正计时跑表==============
	if(timer_start_flag)
	{
		counter_1ms ++;	//1ms加1
		if(counter_1ms == 100)
		{
			counter_1ms = 0;			//0.1s到
			sec ++;				//0.1秒加1
			
			DispBuf[5] = Tab[sec % 10];			//查表取出显示数字对应的段码，存入显示缓冲器数组
			DispBuf[6] = Tab[sec / 10 % 10] | 0x80; 	// "| 0x80"可以使该位带小数点显示
			DispBuf[7] = Tab[sec / 100];
			
			if(sec == 120)
					sec = 0;		//到12.0s，又从0开始
		}
	}
//思考：如果要做一个倒计时秒表，如何修改程序呢？	
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
	unsigned int msec;		//100毫秒计数

	SysInit();
	//开机显示12345678，用于数码管显示检测
	DispBuf[7] = Tab[1];
	DispBuf[6] = Tab[2];
	DispBuf[5] = Tab[3];
	DispBuf[4] = Tab[4];
	DispBuf[3] = Tab[5];
	DispBuf[2] = Tab[6];
	DispBuf[1] = Tab[7];
	DispBuf[0] = Tab[8];
	Delayms(2000);					//保留显示2s
	
	timer_start_flag = 1;	
	DispClear();				//清除显示（数码管上的显示内容全部熄灭）

	//下面的秒表程序用于对照定时中断内的秒表，实际应用中尽量避免使用！！！
	for ( msec=0; msec<120; msec++ )		//显示00.0--11.9秒
	{
			DispBuf[0] = Tab[msec % 10];			//查表取出显示数字对应的段码，存入显示缓冲器数组
			DispBuf[1] = Tab[msec / 10 % 10] | 0x80; 	// 该位带小数点显示
			DispBuf[2] = Tab[msec / 100];
			Delayms(100);						//毎0.1s(100ms)更新显示一次
	}

	while(1);		//停机
}
