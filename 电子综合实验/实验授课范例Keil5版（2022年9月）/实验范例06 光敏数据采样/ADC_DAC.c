/*
	功能：STC89C52 + AD&DA 模块实验。
	1、利用软件模拟I2C总线时序驱动ADC/DAC
  2、利用四通道分时ADC和DAC输出
	程序说明：在数码管上显示通道1和通道3的采样数值，同时输出DAC结果
	注意事项：关于ADC（PCF8591）函数的转换通道如何理解？见程序中注释和。
  编制：电子科技大学实验中心2022年6月
*/
#include<reg52.h>    //包含单片机寄存器的头文件

//-----------模拟I2C总线的必备定义开始---------------
//定义I2C总线时钟的延时值，要根据实际情况修改，取值1～255
//SCL信号周期约为(I2C_DELAY_VALUE*4+15)个机器周期
#define I2C_DELAY_VALUE		10

//定义I2C总线停止后在下一次开始之前的等待时间，取值1～65535
//等待时间约为(I2C_STOP_WAIT_VALUE*8)个机器周期
//对于多数器件取值为1即可；但对于某些器件来说，较长的延时是必须的
#define I2C_STOP_WAIT_VALUE	1

//模拟I2C总线的引脚定义
sbit I2C_SCL = P2^1;		//I2C  时钟
sbit I2C_SDA = P2^0;		//I2C  数据
sbit RST_DS = P2^4;   	//加上后可以关掉DS1302时钟芯片输出

//定义PCF8591 地址
#define  AddWr 0x90
//-----------模拟I2C总线的必备定义结束---------------

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
	
//定义延时变量，用于宏I2C_Delay()

/*
函数：I2C_Delay()
功能：延时，模拟I2C总线专用
*/
void I2C_Delay()
{
	unsigned char I2C_Delay_t;
	I2C_Delay_t = (I2C_DELAY_VALUE);
	while ( --I2C_Delay_t != 0 );
}


/*
函数：I2C_Init()
功能：I2C总线初始化，使总线处于空闲状态
说明：在main()函数的开始处，通常应当要执行一次本函数
*/
void I2C_Init()
{
	I2C_SCL = 1;
	I2C_Delay();
	I2C_SDA = 1;
	I2C_Delay();
}


/*
函数：I2C_Start()
功能：产生I2C总线的起始状态
说明：
	SCL处于高电平期间，当SDA出现下降沿时启动I2C总线
	不论SDA和SCL处于什么电平状态，本函数总能正确产生起始状态
	本函数也可以用来产生重复起始状态
	本函数执行后，I2C总线处于忙状态
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
函数：I2C_Stop()
功能：产生I2C总线的停止状态
说明：
	SCL处于高电平期间，当SDA出现上升沿时停止I2C总线
	不论SDA和SCL处于什么电平状态，本函数总能正确产生停止状态
	本函数执行后，I2C总线处于空闲状态
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
	while ( --t != 0 );		//在下一次产生Start之前，要加一定的延时
}

/*
函数：I2C_Write()
功能：向I2C总线写1个字节的数据
参数：
	dat：要写到总线上的数据
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
函数：I2C_Read()
功能：从从机读取1个字节的数据
返回：读取的一个字节数据
*/
unsigned char I2C_Read()
{
	unsigned char dat;
	unsigned char t = 8;
	I2C_SDA = 1;	//在读取数据之前，要把SDA拉高
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
函数：I2C_GetAck()
功能：读取从机应答位
返回：
	0：从机应答
	1：从机非应答
说明：
	从机在收到每个字节的数据后，要产生应答位
	从机在收到最后1个字节的数据后，一般要产生非应答位
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
函数：I2C_PutAck()
功能：主机产生应答位或非应答位
参数：
	ack=0：主机产生应答位
	ack=1：主机产生非应答位
说明：
	主机在接收完每一个字节的数据后，都应当产生应答位
	主机在接收完最后一个字节的数据后，应当产生非应答位
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
* 函数名				: Pcf8591_DaConversion
* 函数功能			: PCF8591的输出端输出模拟量
* 输入				: addr（器件地址），channel（转换通道），value（转换的数值）
* 输出				: 无
******************* *****************************************/
bit Pcf8591_DaConversion(unsigned char addr,unsigned char channel,  unsigned char Val)
{
	I2C_Start();              //启动总线
	I2C_Write(addr);            //发送器件地址
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
   I2C_Write(0x40|channel);              //发送控制字节
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Write(Val);            //发送DAC的数值  
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Stop();               //结束总线
  return 0;
}

/************************************************************
* 函数名			: Pcf8591_SendByte
* 函数功能		: 写入一个控制命令
* 输入			: addr（器件地址），channel（转换通道）
* 输出			: 无
************************************************************/
bit PCF8591_SendByte(unsigned char addr,unsigned char channel)
{
  I2C_Start();              //启动总线
  I2C_Write(addr);            //发送器件地址
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Write(0x40|channel);              //发送控制字节
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
  I2C_Stop();               //结束总线
  return 0;
}

/************************************************************
* 函数名       	: PCF8591_RcvByte
* 函数功能   	: 读取一个转换值
* 输入          :
* 输出          : dat
************************************************************/
unsigned char PCF8591_RcvByte(unsigned char addr)
{  
  unsigned char dat;

  I2C_Start();          //启动总线
  I2C_Write(addr+1);      //发送器件地址
	if ( I2C_GetAck() )
	{
		I2C_Stop();
		return 1;
	}
	dat=I2C_Read();         //读取数据0

  I2C_PutAck(1);           //发送非应答信号
  I2C_Stop();           //结束总线
  return(dat);
}

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
void Display(unsigned char dat1,unsigned char dat2)

{

	DispBuf[7] = Tab[dat1 / 100];   		//在8位数码管上显示“12345678”字样
	DispBuf[6] = Tab[dat1 /10 % 10];
	DispBuf[5] = Tab[dat1 % 10];

	DispBuf[2] = Tab[dat2 / 100];
	DispBuf[1] = Tab[dat2 /10 % 10];
	DispBuf[0] = Tab[dat2 % 10];
}
/*
函数：Delayms()
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
/*------------------------------------------------
                    主函数
------------------------------------------------*/
void main()
{  
	unsigned char light,voltage;//定义中间变量
	unsigned char chl2,chl4;//定义中间变量
	unsigned char AD_CHANNEL = 0;
	RST_DS = 0;											//		关时钟DS1302
	SysInit();
	I2C_Init();
	//开机显示12345678，用于数码管显示检测
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
	while(1)
	{

	/********以下AD-DA处理*************/  
	/*关于ADC（PCF8591）函数的理解问题：
		A/D转换周期总是在发送一个有效的读模式地址到PCF8591设备后开始。
		A/D转换周期在应答时钟脉冲的后缘触发，所以每次ADC读取的数据总是上一次转换的结果。
		多通道转换读取时，函数PCF8591_SendByte(AddWr,N)中的N指向需要下一次转换的通道。
		而紧接着的PCF8591_RcvByte(AddWr);读取的不是它上一句指示的通道，而是前一次的。
		假如不是多个通道分时转换，只考虑一个通道，那么N就只是需要转换的通道号了。
	*/
		switch(AD_CHANNEL)
		{
			case 0: 
				PCF8591_SendByte(AddWr,1);
			  light = PCF8591_RcvByte(AddWr);  //ADC0 模数转换1	 J8上可接任意电阻元件
			break;  
			
			case 1: 
				PCF8591_SendByte(AddWr,2);
			  chl2 =PCF8591_RcvByte(AddWr);  //ADC1  模数转换2
			break;  
			
			case 2: 
				PCF8591_SendByte(AddWr,3);
				voltage =PCF8591_RcvByte(AddWr);  //ADC2	模数转换3   可调电阻SW1
			break;  
			
			case 3: 
				PCF8591_SendByte(AddWr,0);
			  chl4 =PCF8591_RcvByte(AddWr);  //ADC3   模数转换4	 可调电阻SW2
			break;
			
			case 4: 
				Pcf8591_DaConversion(AddWr,0, voltage); //DAC	  数模转换
			break;			
		}		
		
		if(++AD_CHANNEL > 4) AD_CHANNEL = 0;
	  Display(light,voltage);
		Delayms(50);
   }
}