#include <reg52.h>
#include "PCF8591.h"
//定义驱动引脚
sbit Echo = P3^2;	//Echo
sbit Trig = P3^3;	//Trig

#define LED_seg	P0			//8位数码管的段码和位码驱动通过P0端口锁存 
#define LED_dig	P0

//定义I/O接口
sbit PWM_IN1 = P1^4; 	// 高电平1：左电机后退（反转）
sbit PWM_IN2 = P1^5; 	// 高电平1：左电机前进（正转）

sbit PWM_IN3 = P1^6; 	// 高电平1：右电机前进（正转）	
sbit PWM_IN4 = P1^7; 	// 高电平1：右电机后退（反转）

sbit PWM_EN1 = P1^2; 	// 高电平1：使能左电机 
sbit PWM_EN2 = P1^3; 	// 高电平1：使能右电机
//定义PWM最大级数，也就是调节直流电机的速度等级
#define SPEED_MAX		20
//定义PWM级数，分为0～SPEED_MAX-1级
unsigned char Speed_L;		//左电机转速调节（调节PWM的一个周期SPEED_MAX*1ms时间内，左电机正转时间：Speed_L*1ms）
unsigned char Speed_R;		//右电机转速调节（调节PWM的一个周期SPEED_MAX*1ms时间内，右电机正转时间：Speed_R*1ms）



//定义显示缓冲区（由定时中断程序自动扫描）
unsigned char DispBuf[8];
//================================================


//===================自定义变量======================
bit display_flag=0;  //测量距离显示标志，200ms显示一次
unsigned int Range_s[4]={0,0,0,0};//定义滑动平均值的数据缓存区
unsigned int target_instance=125; //定义小车定位距离初始值12.5cm=125mm

//智能小车数码管按键电路管脚定义
sbit KEY1 = P3^4;		//定义按键1，'+'按键，对应核心板上K1
sbit KEY2 = P3^5;		//定义按键2，'-'按键，对应核心板上K2

sbit led1=P1^0;   
sbit buzzer=P2^3; //启动前按键提示，启动后用于声光报警


unsigned char sec = 0;		//秒表计时器，精度0.1s
bit timer_start_flag = 0;	//定义标志位,秒表启动标志
bit motor_flag=1;//定义电机正反转标志，1正，0反转

bit Range_include_first=1;//0表示第一次进入，且秒表已经暂停

bit buzzer_led_flag=0;//声光报警标志，1报警

unsigned char detector3_a=0;//连续三次检测前进计数
unsigned char detector3_b=0;//连续三次检测后退计数




//==============================================
//智能小车数码管显示电路管脚定义
sbit SS = P2^6;	   //数码管段选信号
sbit CS = P2^7;	   //数码管位选信号

bit Counter_overflag = 0;		//T0定时器溢出标志
bit Echo_Over = 0; 			      //超声波测距完成标志，无论收到回波或没有收到，总要置位一次
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

void T1INTSVC() interrupt 3	//定时器1的中断号为：3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//显示位的端口控制字节
	static unsigned char n = 0;						//n: 扫描显示位计数,0-7
	
	static unsigned int counter_200ms=0;//200ms计数用于显示
	static unsigned int counter_1ms = 0;	//1ms计数用于秒表
	static unsigned char t = 0;  //pwm调速计数变量
	static unsigned int counter_400ms=0;//用于秒表闪烁显示
	static unsigned int counter1_1ms = 0;	//1ms计数用于声光报警
//================数码管定时扫描驱动显示===================	
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
	
//=============每200ms显示测量距离一次==================
	if(counter_200ms==199){
		counter_200ms=0;
		display_flag=1;
	}else{
		counter_200ms++;
	}
//======================================================


//==================秒表精度0.1s===================
	if(timer_start_flag)
	{
		counter_1ms ++;	//1ms加1
		
		if(counter_1ms == 100)
		{
			counter_1ms = 0;			//0.1s到
			sec++;
			DispBuf[0] = Tab[sec % 10];			//查表取出显示数字对应的段码，存入显示缓冲器数组
			DispBuf[1] = Tab[sec / 10 % 10] | 0x80; 	// "| 0x80"可以使该位带小数点显示
			DispBuf[2] = Tab[sec / 100];
		}
	}
//=================================================
	
	//=======================pwm调速======================
	t++;
	if ( t >= SPEED_MAX ) 
		t = 0;				//PWM波的周期为：SPEED_MAX*1ms = 20ms

	if ( t < Speed_L )		//PWM波高电平时间：(Speed_L)*1ms
	{
		if(motor_flag){
			PWM_IN1 = 0;
			PWM_IN2 = 1; 		//左电机的正转
		}else{
			PWM_IN2 = 0;
			PWM_IN1 = 1;		//左电机的反转
		}
	}
	else					//PWM波低电平时间：（SPEED_MAX-Speed_L）*1ms
	{
    PWM_IN1 = 0;
		PWM_IN2 = 0; 		//左电机的停转
	}
	
	if ( t < Speed_R )		//PWM波高电平时间：Speed_R*1ms
	{
		if(motor_flag){
			PWM_IN4 = 0;		//右电机的正转	
			PWM_IN3 = 1;
		}else{
			PWM_IN3 = 0;
			PWM_IN4 = 1;	  //右电机的反转
		}			
	}
	else					//PWM波低电平时间：（SPEED_MAX-Speed_R）*1ms
	{
		PWM_IN3 = 0;
		PWM_IN4 = 0;		//右电机的停转	
	}
	//===============================================
	
	//========秒表停止后计时闪烁（亮0.1+暗0.3s）===============
		if(Range_include_first==0){
			if(counter_400ms==400){
				counter_400ms=0;
			}else{
				counter_400ms++;
			}
			
			if(counter_400ms<100){
				DispBuf[0] = Tab[sec % 10];			//查表取出显示数字对应的段码，存入显示缓冲器数组
				DispBuf[1] = Tab[sec / 10 % 10] | 0x80; 	// "| 0x80"可以使该位带小数点显示
				DispBuf[2] = Tab[sec / 100];
			}else{
				DispBuf[0] =0x00;
				DispBuf[1] =0x00;
				DispBuf[2] =0x00; 
			}
		}
	//=========================================================
	
	//=============声光报警模块=======================
			if(buzzer_led_flag){
				counter1_1ms++;//报警计时++
				if(counter1_1ms==400)
					counter1_1ms=0;
				if(counter1_1ms<=100){
					buzzer=0;
					led1=0;
				}else{
					buzzer=1;
					led1=1;
				}
			}else if(Range_include_first==0){
				//直接else可能会导致按键提示音无效
				//判断进入过一次范围，说明已经启动了不用再用按键
				buzzer=1;
					led1=1;
			}
	//================================================
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
	
	PWM_EN1 = 0;     
	PWM_EN2 = 0;		//电机无效
	PWM_IN1 = 0;
	PWM_IN2 = 0; 		//左电机的停转
	PWM_IN3 = 0;
	PWM_IN4 = 0;		//右电机的停转
	
	DispClear();
	
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
		DispBuf[4] = 0x40;		  
		DispBuf[5] = 0x40;
		DispBuf[6] = 0x40;			  
		DispBuf[7] = 0x40;
	}
	//按照HC-SR04的指标，大致工作在2cm—450cm的范围内，与产品质量和反射面相关
	else		//显示数据单位：厘米
	{
		DispBuf[4] = Tab[Range % 10];		  
		DispBuf[5] = Tab[Range / 10 % 10] + 0x80;
		DispBuf[6] = Tab[Range / 100 % 10];			  
		DispBuf[7] = Tab[Range / 1000];
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
	Range =Echo_time*11*174/10000;     //单位是毫米
	
	return (unsigned int)Range;
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



void main()
{
	unsigned char k;			//定义键值变量
	unsigned char i;//缓存区滑动循环变量
	
	//========adc启动必须条件========
	unsigned char light;//定义AD测量值
	RST_DS = 0; //关时钟DS1302
	I2C_Init();
  //================
	
	//电机和定时器初始化
	SysInit();		
	//====开机显示初始定位距离==
		DispBuf[0] = Tab[target_instance % 10];		  
		DispBuf[1] = Tab[target_instance / 10 % 10] + 0x80;
		DispBuf[2] = Tab[target_instance / 100 % 10];			  
	//=========================
	while(1)
	{
		Echo = 1;						//IO口读入之前输出1
		TR1=0;
		ET1=0;
		StartModule();		//启动模块
		while(!Echo);
		ET1=1;
		TR1=1;
		
		//===================滑动均值====================
		for(i=0;i<3;i=i+1){ //滑动
			Range_s[i+1]=Range_s[i];
		}
		Range_s[0] = Timer_Count();		//超声波高电平脉冲宽度计算函数
		
		if(Range_s[0]&&Range_s[1]&&Range_s[2]&&Range_s[3]){//都不为0说明进行了三次有效测量
			Range=(Range_s[0]+Range_s[1]+Range_s[2]+Range_s[3]+2)/4;//计算滑动均值
		}else{
			Range=Range_s[0];//否则采用最新一次测量结果;
		}
		//===================================================
		
		
		
		//距离有关处
		
		//========根据距离调节速度3档(需上车调试)=========
			if(Range>500){//50cm
				//较远高速行驶，调节两者以保持直线
				Speed_L=16;
				Speed_R=16;
			}else if(Range>200){//20cm
				Speed_L=10;
				Speed_R=10;
			}else if(Range>150){//15cm
				Speed_L=4;  //低速模式最好与下方一致
				Speed_R=4;
			}
		//================================================
			
		//=============进入定位距离====================
		if(Range<=target_instance+5&&Range>=target_instance-5){
			if(Range_include_first){//是否第一次进入
				timer_start_flag=0;//计时关闭
				Range_include_first=0;//闪烁显示标志
			}
				Speed_L=0;//小车停止
				Speed_R=0;
			
				detector3_a=0;//检测计数清零，保证检测三次的连续性
				detector3_b=0;
		}
		if(Range_include_first==0&&(Range>target_instance+5)){
				detector3_a++; //次数递增
				detector3_b=0;//清零保证连续性
			if(detector3_a==3){//连续三次正转需求则正转
				motor_flag=1;//电机正转
				Speed_L=4;   //低速行驶
				Speed_R=4;
				detector3_a=0;
			}
			
		}
		if(Range_include_first==0&&(Range<target_instance-5)){
				detector3_b++;
				detector3_a=0;
			if(detector3_b==3){//连续三次反转需求则反转
				buzzer_led_flag=1;//报警
				motor_flag=0;//电机反转
				Speed_L=4;   //低速行驶
				Speed_R=4;
				detector3_b=0;
			}
			
		}else{
			buzzer_led_flag=0;
		}
		//========================================================
		
		Delay(10);//10ms延时
		
    //=========超声波距离显示==========================
		if(display_flag){//
			Range_Display();
			display_flag=0;
		}
		//==============================
		
		
		//==============定位距离调节==================
		//Delay(20);//按键后延消抖，前面超声波有延时可以不用
		k = KeyScan();			//扫描按键
		if ( k != '\0')			//首次检测到按键按下
		{
			Delay(20);					//延时20ms，按键前沿消抖
			k = KeyScan();			//再次读取按键状态
			if ( k != '\0')			//确认按键按下后处理按键
			{
				if ( k == '+') 
				{
					buzzer=0;//按键提示音
					Delay(100);
					buzzer=1;
					
					if ( target_instance < 150 ) 
						target_instance=target_instance+5;
				}
				if ( k == '-')
				{
					buzzer=0;//按键提示音
					Delay(100);
					buzzer=1;
					
					if ( target_instance > 100 )
						target_instance=target_instance-5;
					
				}
				
				DispBuf[0] = Tab[target_instance % 10];		  
				DispBuf[1] = Tab[target_instance / 10 % 10] + 0x80;
				DispBuf[2] = Tab[target_instance / 100 % 10];	

				while( KeyScan() != '\0');		//等待松开按键
			}
		}
		//==============================================
		
		//=============adc采集光敏数据================
			PCF8591_SendByte(AddWr,0);
			light = PCF8591_RcvByte(AddWr);  		
			Pcf8591_DaConversion(AddWr,0, light);//将ad输入结果作为da输出  		
		//======遮光启动==================
			if(light>130){
			timer_start_flag=1;//计时启动
			PWM_EN1 = 1;			//电机有效	
			PWM_EN2 = 1;			
		}
		//=========================================
		
		
	}

}

