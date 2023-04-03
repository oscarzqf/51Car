/*
	���ܣ����������ʵ�顣
	����ģ�飺HC-SR04����෶Χ��2cm to 450cm
	�������ʾ��xxx.x����λ��cm
	���㹫ʽ���£�
	s = v*t/2 = (340m/s)*t(s)/2 = (340um/us)*t(us)/2 = (0.34mm/us)*t(us)/2 = (0.034cm/us)*t(us)/2;;
	s(cm) = t(us)*0.017(cm/us).
	t is the received Echo high level time from the UltracsonicModule HC-SR04, 
	it messured by T0 timer, t=(12/11.0592)us*(TH0,TL0), initial (TH0,TL0)=0x00 00
	s(cm) =0.0184*(TH0,TL0),	(TH0,TL0)max = 24456 for s =450cm
	UltracsonicModule HC-SR04 VCC(+5V) connect to VCC; 
	Gnd connct to GND; 
	Trig connect to P3.3; 
	Echo connect to P3.2.
	��������෶��
	�������ʾ��������С��255�Σ�������0�����������Ͳ��������λ��cm��
	���ƣ����ӿƼ���ѧʵ������2022��6��
*/


#include <reg52.h>

//������������
sbit Echo = P3^2;	//Echo(���նˣ���ѯ�����ź�)
sbit Trig = P3^3;	//Trig�������ˣ����Ƴ�����������

#define LED_seg	P0			//8λ����ܵĶ����λ������ͨ��P0�˿����� 
#define LED_dig	P0

//������ʾ���������ɶ�ʱ�жϳ����Զ�ɨ�裩
unsigned char DispBuf[8];

//����С���������ʾ��·�ܽŶ���
sbit SS = P2^6;	   //����ܶ�ѡ�ź�
sbit CS = P2^7;	   //�����λѡ�ź�

//���廬��ƽ��ֵ�����ݻ�����
unsigned int Range_s[3];

bit Counter_overflag = 0;		//T0��ʱ�������־
bit Echo_Over = 0; 					//�����������ɱ�־�������յ��ز���û���յ�����Ҫ��λһ��

unsigned int Range = 0;
unsigned long Echo_time = 0;		//T0��ʱ���ϲ���ֵ

code unsigned char Tab[] =
	{//����0123456789AbCdEF��������������ݣ�������ʾ�ַ������м��㣬�确-������������Ϊ0x40
		0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
		0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
	};
//============================//
/*
������EX0INTSVC()
���ܣ��ⲿ�ж�0�жϷ������
��;��Ϊ���ⶨʱ�жϸ��Ų���׼ȷ�ԣ������ⲿ�жϴ���������ֹͣ��ʱ
	    �ⲿ�ж�0���ȼ����ڶ�ʱ�жϣ��������̣����������Ŷ�ʱ�ж�
*/
void EX0INTSVC() interrupt 0
{
	TR0 = 0;					//ֹͣ��ʱ
	Echo_time = TH0 * 256 + TL0; 	//��ȡ��ʱ��0����ֵ
	TH0 = 0; 					//�����ʱ��0�����Ĵ�����Ϊ�´μ�����׼��
	TL0 = 0;
	Echo_Over = 1; 		//��ʾ���γ����������ɣ����������´εĲ���
	EX0 = 0;					//�ر��ⲿ�жϣ����������������һ���ж�
}
//============================//
/*
������T0INTSVC()
���ܣ���ʱ��T0���жϷ�����
��;����������෶Χ����ʱ���޷��յ��ز���
	    �Ѿ�������T0�жϻ������������ö�ʱ�������־���ж�
*/
void T0INTSVC() interrupt 1 		 
{
	TR0 = 0;				//ֹͣ��ʱ
	Counter_overflag = 1;			 //�ж������־��δ�յ��ز�
	Echo_Over = 1;	//��ʾ���γ����������ɣ����������´εĲ���
	EX0 = 0;				//�ر��ⲿ�ж�
}
/*
������T1INTSVC()
���ܣ���ʱ��T1���жϷ�����
*/
//===============������صı���=============


bit display_flag=0;//�������ʾ��־
bit timerstart_flag0=0;//200ms������������־
bit timerstart_flag1=0;//ѭ����ʱ������־
bit timermode=0;//ѭ����ʱ��ʽѡ��0ѭ������ʱ��1ѭ������ʱ
unsigned char sec = 60;		//100ms����������ʼֵ60



//========================================

void T1INTSVC() interrupt 3	//��ʱ��1���жϺ�Ϊ��3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//��ʾλ�Ķ˿ڿ����ֽ�
	static unsigned char n = 0;						//n: ɨ����ʾλ����,0-7
	static unsigned int counter_200ms=0;//200ms����������ʾ
	static unsigned int counter_1ms = 0;	//1ms����

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
//================200ms��ʱģ�飬������ʾ��־=============
if(timerstart_flag0){
	if(counter_200ms==199){
		counter_200ms=0;
		display_flag=1;
	}else{
		counter_200ms++;
	}
}
//=========================ѭ����ʱģ��==============
if(timerstart_flag1)
	{
		counter_1ms ++;	//1ms��1
		if(counter_1ms == 100)
		{
			counter_1ms = 0;			//0.1s��
			if(timermode==1)	
					sec++;
			if(timermode==0)
					sec--;
			
			DispBuf[5] = Tab[sec % 10];			//���ȡ����ʾ���ֶ�Ӧ�Ķ��룬������ʾ����������
			DispBuf[6] = Tab[sec / 10 % 10] | 0x80; 	// "| 0x80"����ʹ��λ��С������ʾ
			DispBuf[7] = Tab[sec / 100];
			
			if(timermode==1&&sec == 119)//ѭ������ʱ
					sec = 0;		//��11.9s���ִ�0��ʼ
			if(timermode==0&&sec==0)//ѭ������ʱ
				  sec=119;
		}
	}

//======================================================
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
������SysInit()
���ܣ�ϵͳ��ʼ��
*/
void SysInit()
{

	DispClear();	//��ʼ����ʾ����
	TMOD = 0x11;	//���ö�ʱ��T0Ϊ16λ��ʱ������ʱ��T1Ϊ16λ��ʱ��
	EA = 0;				//�ر����жϣ�����ʼ���������ٴ�
//======��ʱ������T0��ʼ�������ڻ�ȡ����������ʱ�䣬����ʱ������򳬳���෶Χ
	TH0 = 0;		
	TL0 = 0;
	ET0 = 1;
//======��ʱ������T1��ʼ�������ڻ�ȡ1ms��ʱ�ж�=====
	TH1 = 0xFC;		//���ö�ʱ��1�ĳ�ֵ: 0xFC66����Ӧ��ʱʱ��1ms
	TL1 = 0x66;
	ET1 = 1;			//ʹ�ܶ�ʱ��T1�ж�
	TR1 = 1;			//������ʱ��T1
//======��ʱ������T0��T1��ʼ�����=================	
	EX0 = 0;			//�ر��ⲿ�ж�
	IT0 = 0;			//�ⲿ�ж�0���õ�ƽ����ģʽ���͵�ƽ����
	EA = 1;				//ʹ�����ж�
}

/*
������StartModule()
���ܣ�����ģ�飬���� IO ������࣬������10us �ĸߵ�ƽ�ź�;
*/
void  StartModule()
{
	unsigned char i;
	Trig = 1;			                  //����һ��ģ��
	for(i = 0;i < 10;i ++);					//�����������ӳ�10us����;
	Trig = 0;
}
/*
������Range_Display()
���ܣ�������������ʾ����
˵������������������ʾ��- - - -��
*/
 void Range_Display()
{
	
	//����������Χ�����ϰ���̫������Ƕ�̫С�����޷��յ��ز�������ʾ��- - - -��
	if((Range >= 4000) || Counter_overflag == 1)
	{	 
		Counter_overflag = 0;
		DispBuf[0] = 0x40;		  
		DispBuf[1] = 0x40;
		DispBuf[2] = 0x40;			  
		DispBuf[3] = 0x40;
	}
	//����HC-SR04��ָ�꣬���¹�����2cm��450cm�ķ�Χ�ڣ����Ʒ�����ͷ��������
	else		//��ʾ���ݵ�λ������
	{
		DispBuf[0] = Tab[Range % 10];		  
		DispBuf[1] = Tab[Range / 10 % 10] + 0x80;
		DispBuf[2] = Tab[Range / 100 % 10];			  
		DispBuf[3] = Tab[Range / 1000];
	}
}
/*
������Timer_Count()
���ܣ��������ߵ�ƽ�����ȼ��㺯��
��ע�����ò�ѯģʽ
˵����������ģ���ڵȴ��ز���ʱ�򣬾�������ʱ�жϴ�ϣ����»ز�����ʱ���������ʱ�������˳�������������
�Ľ��İ취�Ƿֱ��������������䣨��ʱ���Σ�ȡ�����ο����������գ�
	TR1 = 0;		//��ͣ��ʱ��T1�������൱�ڹرն�ʱ�ж�T1
  TR1 = 1;		//������ʱ��T1�������൱�ڴ򿪶�ʱ�ж�T1
��֮����ʲô�µ������أ�����������ԭ��
*/
unsigned int  Timer_Count()		
{
	
	unsigned long Range;
	TR0 = 1;		  //��������
	EX0 = 1;			//�����ⲿ�ж�
	while(!Echo_Over);	//�ȴ��ز�����Echo_OverΪ1�������յ��ز��򳬳���෶Χ
	
	Echo_Over = 0;			//���Echo_Over��׼����һ�β��
	//����������Ѿ��õ��˳���������Ӧ����ֵ��������ڱ���Echo_time�ڣ�Echo_time * 1.1us�õ���Ӧʱ��
	//���軷���¶�26.5���϶ȣ�����Echo_time��ֵ���м�����ĳ��ȣ���λ�����ף����滻����Ķ�ֵ���ʽ
	//ע�⣺�����ö������㣬����Ϊ���ף�
	//Range = 1234;     //��λ�Ǻ���
	Range =Echo_time*11*174/10000;     //��λ�Ǻ���
	return (unsigned int)Range;
}
void main()
{
	
	SysInit();		//��ʱ����ʼ��
	
	DispBuf[7] = Tab[1];   		//��8λ���������ʾ��12345678������
	DispBuf[6] = Tab[2];
	DispBuf[5] = Tab[3];
	DispBuf[4] = Tab[4];
	DispBuf[3] = Tab[5];
	DispBuf[2] = Tab[6];
	DispBuf[1] = Tab[7];
	DispBuf[0] = Tab[8];
	Delay(2000);					
	DispClear();				//�����ʾ��������ϵ���ʾ����ȫ��Ϩ��
	
	Range_s[0]=0;//������ݻ�����
	Range_s[1]=0;
	Range_s[2]=0;
	timerstart_flag0=1;//����200ms������

	while(1)
	{
		Echo = 1;						//IO�ڶ���֮ǰ���1
		StartModule();		//����ģ��
		while(!Echo);
		
		Range_s[2]=Range_s[1];
		Range_s[1]=Range_s[0];
		Range_s[0]=Timer_Count();//�²����������ݻ�����
		
		if(Range_s[0]&&Range_s[1]&&Range_s[2]){//����Ϊ0˵�����������β���
			Range=(Range_s[0]+Range_s[1]+Range_s[2])/3;//���㻬����ֵ
		}else{
			Range=Range_s[0];//�����������һ�β������;
		}
		
		if(Range<120){
			timermode=0;//����ʱ
			timerstart_flag1=1;//����ѭ��������
		}else if(Range>150){
			timermode=1;//����ʱ
			timerstart_flag1=1;
		}else{
			timerstart_flag1=0;//ֹͣ��ʱ
		}
		
		
    //Range = Timer_Count();		//�������ߵ�ƽ�����ȼ��㺯��
		if(display_flag){//������������ʾ
			Range_Display();
			display_flag=0;//�����ʾ��־
		}
		Delay(5);//�ʵ���ʱ
		//Delay(200);							//ÿ��0.2���ȡһ�β���ʾһ�β������ݣ������Ӿ��۲�
	}

}

