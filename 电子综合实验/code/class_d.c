/*
	功能：超声波测距实验。
	功能模块：HC-SR04，测距范围：2cm to 450cm
	数码管显示：xxx.x，单位：cm
	计算公式如下：
	s = v*t/2 = (340m/s)*t(s)/2 = (340um/us)*t(us)/2 = (0.34mm/us)*t(us)/2 = (0.034cm/us)*t(us)/2;;
	s(cm) = t(us)*0.017(cm/us).
	t is the received Echo high level time from the UltracsonicModule HC-SR04, 
	it messured by T0 timer, t=(12/11.0592)us*(TH0,TL0), initial (TH0,TL0)=0x00 00
	s(cm) =0.0184*(TH0,TL0),	(TH0,TL0)max = 24456 for s =450cm
	UltracsonicModule HC-SR04 VCC(+5V) connect to VCC; 
	Gnd connct to GND; 
	Trig connect to P3.3; 
	Echo connect to P3.2.
	超声波测距范例
	数码管显示测距次数（小于255次，超出从0继续计数）和测距结果（单位：cm）
	编制：电子科技大学实验中心2022年6月
*/


#include <reg52.h>

//定义驱动引脚
sbit Echo = P3^2;	//Echo(接收端，查询回响信号)
sbit Trig = P3^3;	//Trig（触发端，控制超声波触发）

#define LED_seg	P0			//8位数码管的段码和位码驱动通过P0端口锁存 
#define LED_dig	P0

//定义显示缓冲区（由定时中断程序自动扫描）
unsigned char DispBuf[8];

//智能小车数码管显示电路管脚定义
sbit SS = P2^6;	   //数码管段选信号
sbit CS = P2^7;	   //数码管位选信号

//定义滑动平均值的数据缓存区
unsigned int Range_s[3];

bit Counter_overflag = 0;		//T0定时器溢出标志
bit Echo_Over = 0; 					//超声波测距完成标志，无论收到回波或没有收到，总要置位一次

unsigned int Range = 0;
unsigned long Echo_time = 0;		//T0定时器合并数值

code unsigned char Tab[] =
	{//定义0123456789AbCdEF的数码管字型数据，其他显示字符需自行计算，如‘-’的字形数据为0x40
		0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
		0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
	};
//============================//
/*
函数：EX0INTSVC()
功能：外部中断0中断服务程序
用途：为避免定时中断干扰测距的准确性，采用外部中断触发后立即停止计时
	    外部中断0优先级高于定时中断，程序尽量短，避免过多干扰定时中断
*/
void EX0INTSVC() interrupt 0
{
	TR0 = 0;					//停止计时
	Echo_time = TH0 * 256 + TL0; 	//读取定时器0计数值
	TH0 = 0; 					//清除定时器0计数寄存器，为下次计数做准备
	TL0 = 0;
	Echo_Over = 1; 		//表示本次超声波测距完成，可以启动下次的测量
	EX0 = 0;					//关闭外部中断，否则会马上引起下一次中断
}
//============================//
/*
函数：T0INTSVC()
功能：定时器T0的中断服务函数
用途：若超过测距范围，长时间无法收到回波，
	    已经启动的T0中断会计数溢出，利用定时器溢出标志来判断
*/
void T0INTSVC() interrupt 1 		 
{
	TR0 = 0;				//停止计时
	Counter_overflag = 1;			 //中断溢出标志，未收到回波
	Echo_Over = 1;	//表示本次超声波测距完成，可以启动下次的测量
	EX0 = 0;				//关闭外部中断
}
/*
函数：T1INTSVC()
功能：定时器T1的中断服务函数
*/
//===============计数相关的变量=============


bit display_flag=0;//测距结果显示标志
bit timerstart_flag0=0;//200ms计数器启动标志
bit timerstart_flag1=0;//循环计时启动标志
bit timermode=0;//循环计时方式选择，0循环倒计时，1循环正计时
unsigned char sec = 60;		//100ms计数，秒表初始值60



//========================================

void T1INTSVC() interrupt 3	//定时器1的中断号为：3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//显示位的端口控制字节
	static unsigned char n = 0;						//n: 扫描显示位计数,0-7
	static unsigned int counter_200ms=0;//200ms计数用于显示
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
//================200ms计时模块，更新显示标志=============
if(timerstart_flag0){
	if(counter_200ms==199){
		counter_200ms=0;
		display_flag=1;
	}else{
		counter_200ms++;
	}
}
//=========================循环计时模块==============
if(timerstart_flag1)
	{
		counter_1ms ++;	//1ms加1
		if(counter_1ms == 100)
		{
			counter_1ms = 0;			//0.1s到
			if(timermode==1)	
					sec++;
			if(timermode==0)
					sec--;
			
			DispBuf[5] = Tab[sec % 10];			//查表取出显示数字对应的段码，存入显示缓冲器数组
			DispBuf[6] = Tab[sec / 10 % 10] | 0x80; 	// "| 0x80"可以使该位带小数点显示
			DispBuf[7] = Tab[sec / 100];
			
			if(timermode==1&&sec == 119)//循环正计时
					sec = 0;		//到11.9s，又从0开始
			if(timermode==0&&sec==0)//循环倒计时
				  sec=119;
		}
	}

//======================================================
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

	DispClear();	//初始化显示缓存
	TMOD = 0x11;	//设置定时器T0为16位定时器，定时器T1为16位定时器
	EA = 0;				//关闭总中断，待初始化结束后再打开
//======定时计数器T0初始化，用于获取超声波作用时间，若定时溢出，则超出测距范围
	TH0 = 0;		
	TL0 = 0;
	ET0 = 1;
//======定时计数器T1初始化，用于获取1ms定时中断=====
	TH1 = 0xFC;		//设置定时器1的初值: 0xFC66，对应定时时间1ms
	TL1 = 0x66;
	ET1 = 1;			//使能定时器T1中断
	TR1 = 1;			//启动定时器T1
//======定时计数器T0和T1初始化完毕=================	
	EX0 = 0;			//关闭外部中断
	IT0 = 0;			//外部中断0采用电平触发模式，低电平出发
	EA = 1;				//使能总中断
}

/*
函数：StartModule()
功能：启动模块，采用 IO 触发测距，给至少10us 的高电平信号;
*/
void  StartModule()
{
	unsigned char i;
	Trig = 1;			                  //启动一次模块
	for(i = 0;i < 10;i ++);					//超声波启动延迟10us以上;
	Trig = 0;
}
/*
函数：Range_Display()
功能：超声波距离显示函数
说明：若超出距离则显示“- - - -”
*/
 void Range_Display()
{
	
	//超出测量范围或者障碍物太近反射角度太小导致无法收到回波，都显示“- - - -”
	if((Range >= 4000) || Counter_overflag == 1)
	{	 
		Counter_overflag = 0;
		DispBuf[0] = 0x40;		  
		DispBuf[1] = 0x40;
		DispBuf[2] = 0x40;			  
		DispBuf[3] = 0x40;
	}
	//按照HC-SR04的指标，大致工作在2cm—450cm的范围内，与产品质量和反射面相关
	else		//显示数据单位：厘米
	{
		DispBuf[0] = Tab[Range % 10];		  
		DispBuf[1] = Tab[Range / 10 % 10] + 0x80;
		DispBuf[2] = Tab[Range / 100 % 10];			  
		DispBuf[3] = Tab[Range / 1000];
	}
}
/*
函数：Timer_Count()
功能：超声波高电平脉冲宽度计算函数
备注：采用查询模式
说明：超声波模块在等待回波的时候，经常被定时中断打断，导致回波到达时间测量不及时，干扰了超声波测量精度
改进的办法是分别加入下面两条语句（暂时屏蔽，取消屏蔽可以用来对照）
	TR1 = 0;		//暂停定时器T1计数，相当于关闭定时中断T1
  TR1 = 1;		//重启定时器T1计数，相当于打开定时中断T1
随之带来什么新的问题呢？分析产生的原因。
*/
unsigned int  Timer_Count()		
{
	
	unsigned long Range;
	TR0 = 1;		  //开启计数
	EX0 = 1;			//开启外部中断
	while(!Echo_Over);	//等待回波，当Echo_Over为1，表明收到回波或超出测距范围
	
	Echo_Over = 0;			//清除Echo_Over，准备下一次测距
	//程序到这里就已经得到了超声波的响应计数值，结果存在变量Echo_time内，Echo_time * 1.1us得到响应时间
	//假设环境温度26.5摄氏度，根据Echo_time的值自行计算测距的长度（单位：毫米）并替换下面的定值表达式
	//注意：必须用定点运算，精度为毫米！
	//Range = 1234;     //单位是毫米
	Range =Echo_time*11*174/10000;     //单位是毫米
	return (unsigned int)Range;
}
void main()
{
	
	SysInit();		//定时器初始化
	
	DispBuf[7] = Tab[1];   		//在8位数码管上显示“12345678”字样
	DispBuf[6] = Tab[2];
	DispBuf[5] = Tab[3];
	DispBuf[4] = Tab[4];
	DispBuf[3] = Tab[5];
	DispBuf[2] = Tab[6];
	DispBuf[1] = Tab[7];
	DispBuf[0] = Tab[8];
	Delay(2000);					
	DispClear();				//清除显示（数码管上的显示内容全部熄灭
	
	Range_s[0]=0;//清除数据缓存区
	Range_s[1]=0;
	Range_s[2]=0;
	timerstart_flag0=1;//启动200ms计数器

	while(1)
	{
		Echo = 1;						//IO口读入之前输出1
		StartModule();		//启动模块
		while(!Echo);
		
		Range_s[2]=Range_s[1];
		Range_s[1]=Range_s[0];
		Range_s[0]=Timer_Count();//新测量放入数据缓存区
		
		if(Range_s[0]&&Range_s[1]&&Range_s[2]){//都不为0说明进行了三次测量
			Range=(Range_s[0]+Range_s[1]+Range_s[2])/3;//计算滑动均值
		}else{
			Range=Range_s[0];//否则采用最新一次测量结果;
		}
		
		if(Range<120){
			timermode=0;//倒计时
			timerstart_flag1=1;//启动循环计数器
		}else if(Range>150){
			timermode=1;//正计时
			timerstart_flag1=1;
		}else{
			timerstart_flag1=0;//停止计时
		}
		
		
    //Range = Timer_Count();		//超声波高电平脉冲宽度计算函数
		if(display_flag){//超声波距离显示
			Range_Display();
			display_flag=0;//清除显示标志
		}
		Delay(5);//适当延时
		//Delay(200);							//每隔0.2秒读取一次并显示一次测量数据，便于视觉观察
	}

}

