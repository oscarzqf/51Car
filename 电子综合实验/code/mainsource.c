#include <reg52.h>
#include "PCF8591.h"
//������������
sbit Echo = P3^2;	//Echo
sbit Trig = P3^3;	//Trig

#define LED_seg	P0			//8λ����ܵĶ����λ������ͨ��P0�˿����� 
#define LED_dig	P0

//����I/O�ӿ�
sbit PWM_IN1 = P1^4; 	// �ߵ�ƽ1���������ˣ���ת��
sbit PWM_IN2 = P1^5; 	// �ߵ�ƽ1������ǰ������ת��

sbit PWM_IN3 = P1^6; 	// �ߵ�ƽ1���ҵ��ǰ������ת��	
sbit PWM_IN4 = P1^7; 	// �ߵ�ƽ1���ҵ�����ˣ���ת��

sbit PWM_EN1 = P1^2; 	// �ߵ�ƽ1��ʹ������ 
sbit PWM_EN2 = P1^3; 	// �ߵ�ƽ1��ʹ���ҵ��
//����PWM�������Ҳ���ǵ���ֱ��������ٶȵȼ�
#define SPEED_MAX		20
//����PWM��������Ϊ0��SPEED_MAX-1��
unsigned char Speed_L;		//����ת�ٵ��ڣ�����PWM��һ������SPEED_MAX*1msʱ���ڣ�������תʱ�䣺Speed_L*1ms��
unsigned char Speed_R;		//�ҵ��ת�ٵ��ڣ�����PWM��һ������SPEED_MAX*1msʱ���ڣ��ҵ����תʱ�䣺Speed_R*1ms��



//������ʾ���������ɶ�ʱ�жϳ����Զ�ɨ�裩
unsigned char DispBuf[8];
//================================================


//===================�Զ������======================
bit display_flag=0;  //����������ʾ��־��200ms��ʾһ��
unsigned int Range_s[4]={0,0,0,0};//���廬��ƽ��ֵ�����ݻ�����
unsigned int target_instance=125; //����С����λ�����ʼֵ12.5cm=125mm

//����С������ܰ�����·�ܽŶ���
sbit KEY1 = P3^4;		//���尴��1��'+'��������Ӧ���İ���K1
sbit KEY2 = P3^5;		//���尴��2��'-'��������Ӧ���İ���K2

sbit led1=P1^0;   
sbit buzzer=P2^3; //����ǰ������ʾ���������������ⱨ��


unsigned char sec = 0;		//����ʱ��������0.1s
bit timer_start_flag = 0;	//�����־λ,���������־
bit motor_flag=1;//����������ת��־��1����0��ת

bit Range_include_first=1;//0��ʾ��һ�ν��룬������Ѿ���ͣ

bit buzzer_led_flag=0;//���ⱨ����־��1����

unsigned char detector3_a=0;//�������μ��ǰ������
unsigned char detector3_b=0;//�������μ����˼���




//==============================================
//����С���������ʾ��·�ܽŶ���
sbit SS = P2^6;	   //����ܶ�ѡ�ź�
sbit CS = P2^7;	   //�����λѡ�ź�

bit Counter_overflag = 0;		//T0��ʱ�������־
bit Echo_Over = 0; 			      //�����������ɱ�־�������յ��ز���û���յ�����Ҫ��λһ��
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

void T1INTSVC() interrupt 3	//��ʱ��1���жϺ�Ϊ��3
{
	code unsigned char com[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	//��ʾλ�Ķ˿ڿ����ֽ�
	static unsigned char n = 0;						//n: ɨ����ʾλ����,0-7
	
	static unsigned int counter_200ms=0;//200ms����������ʾ
	static unsigned int counter_1ms = 0;	//1ms�����������
	static unsigned char t = 0;  //pwm���ټ�������
	static unsigned int counter_400ms=0;//���������˸��ʾ
	static unsigned int counter1_1ms = 0;	//1ms�����������ⱨ��
//================����ܶ�ʱɨ��������ʾ===================	
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
	
//=============ÿ200ms��ʾ��������һ��==================
	if(counter_200ms==199){
		counter_200ms=0;
		display_flag=1;
	}else{
		counter_200ms++;
	}
//======================================================


//==================�����0.1s===================
	if(timer_start_flag)
	{
		counter_1ms ++;	//1ms��1
		
		if(counter_1ms == 100)
		{
			counter_1ms = 0;			//0.1s��
			sec++;
			DispBuf[0] = Tab[sec % 10];			//���ȡ����ʾ���ֶ�Ӧ�Ķ��룬������ʾ����������
			DispBuf[1] = Tab[sec / 10 % 10] | 0x80; 	// "| 0x80"����ʹ��λ��С������ʾ
			DispBuf[2] = Tab[sec / 100];
		}
	}
//=================================================
	
	//=======================pwm����======================
	t++;
	if ( t >= SPEED_MAX ) 
		t = 0;				//PWM��������Ϊ��SPEED_MAX*1ms = 20ms

	if ( t < Speed_L )		//PWM���ߵ�ƽʱ�䣺(Speed_L)*1ms
	{
		if(motor_flag){
			PWM_IN1 = 0;
			PWM_IN2 = 1; 		//��������ת
		}else{
			PWM_IN2 = 0;
			PWM_IN1 = 1;		//�����ķ�ת
		}
	}
	else					//PWM���͵�ƽʱ�䣺��SPEED_MAX-Speed_L��*1ms
	{
    PWM_IN1 = 0;
		PWM_IN2 = 0; 		//������ͣת
	}
	
	if ( t < Speed_R )		//PWM���ߵ�ƽʱ�䣺Speed_R*1ms
	{
		if(motor_flag){
			PWM_IN4 = 0;		//�ҵ������ת	
			PWM_IN3 = 1;
		}else{
			PWM_IN3 = 0;
			PWM_IN4 = 1;	  //�ҵ���ķ�ת
		}			
	}
	else					//PWM���͵�ƽʱ�䣺��SPEED_MAX-Speed_R��*1ms
	{
		PWM_IN3 = 0;
		PWM_IN4 = 0;		//�ҵ����ͣת	
	}
	//===============================================
	
	//========���ֹͣ���ʱ��˸����0.1+��0.3s��===============
		if(Range_include_first==0){
			if(counter_400ms==400){
				counter_400ms=0;
			}else{
				counter_400ms++;
			}
			
			if(counter_400ms<100){
				DispBuf[0] = Tab[sec % 10];			//���ȡ����ʾ���ֶ�Ӧ�Ķ��룬������ʾ����������
				DispBuf[1] = Tab[sec / 10 % 10] | 0x80; 	// "| 0x80"����ʹ��λ��С������ʾ
				DispBuf[2] = Tab[sec / 100];
			}else{
				DispBuf[0] =0x00;
				DispBuf[1] =0x00;
				DispBuf[2] =0x00; 
			}
		}
	//=========================================================
	
	//=============���ⱨ��ģ��=======================
			if(buzzer_led_flag){
				counter1_1ms++;//������ʱ++
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
				//ֱ��else���ܻᵼ�°�����ʾ����Ч
				//�жϽ����һ�η�Χ��˵���Ѿ������˲������ð���
				buzzer=1;
					led1=1;
			}
	//================================================
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
	
	PWM_EN1 = 0;     
	PWM_EN2 = 0;		//�����Ч
	PWM_IN1 = 0;
	PWM_IN2 = 0; 		//������ͣת
	PWM_IN3 = 0;
	PWM_IN4 = 0;		//�ҵ����ͣת
	
	DispClear();
	
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
		DispBuf[4] = 0x40;		  
		DispBuf[5] = 0x40;
		DispBuf[6] = 0x40;			  
		DispBuf[7] = 0x40;
	}
	//����HC-SR04��ָ�꣬���¹�����2cm��450cm�ķ�Χ�ڣ����Ʒ�����ͷ��������
	else		//��ʾ���ݵ�λ������
	{
		DispBuf[4] = Tab[Range % 10];		  
		DispBuf[5] = Tab[Range / 10 % 10] + 0x80;
		DispBuf[6] = Tab[Range / 100 % 10];			  
		DispBuf[7] = Tab[Range / 1000];
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
	Range =Echo_time*11*174/10000;     //��λ�Ǻ���
	
	return (unsigned int)Range;
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



void main()
{
	unsigned char k;			//�����ֵ����
	unsigned char i;//����������ѭ������
	
	//========adc������������========
	unsigned char light;//����AD����ֵ
	RST_DS = 0; //��ʱ��DS1302
	I2C_Init();
  //================
	
	//����Ͷ�ʱ����ʼ��
	SysInit();		
	//====������ʾ��ʼ��λ����==
		DispBuf[0] = Tab[target_instance % 10];		  
		DispBuf[1] = Tab[target_instance / 10 % 10] + 0x80;
		DispBuf[2] = Tab[target_instance / 100 % 10];			  
	//=========================
	while(1)
	{
		Echo = 1;						//IO�ڶ���֮ǰ���1
		TR1=0;
		ET1=0;
		StartModule();		//����ģ��
		while(!Echo);
		ET1=1;
		TR1=1;
		
		//===================������ֵ====================
		for(i=0;i<3;i=i+1){ //����
			Range_s[i+1]=Range_s[i];
		}
		Range_s[0] = Timer_Count();		//�������ߵ�ƽ�����ȼ��㺯��
		
		if(Range_s[0]&&Range_s[1]&&Range_s[2]&&Range_s[3]){//����Ϊ0˵��������������Ч����
			Range=(Range_s[0]+Range_s[1]+Range_s[2]+Range_s[3]+2)/4;//���㻬����ֵ
		}else{
			Range=Range_s[0];//�����������һ�β������;
		}
		//===================================================
		
		
		
		//�����йش�
		
		//========���ݾ�������ٶ�3��(���ϳ�����)=========
			if(Range>500){//50cm
				//��Զ������ʻ�����������Ա���ֱ��
				Speed_L=16;
				Speed_R=16;
			}else if(Range>200){//20cm
				Speed_L=10;
				Speed_R=10;
			}else if(Range>150){//15cm
				Speed_L=4;  //����ģʽ������·�һ��
				Speed_R=4;
			}
		//================================================
			
		//=============���붨λ����====================
		if(Range<=target_instance+5&&Range>=target_instance-5){
			if(Range_include_first){//�Ƿ��һ�ν���
				timer_start_flag=0;//��ʱ�ر�
				Range_include_first=0;//��˸��ʾ��־
			}
				Speed_L=0;//С��ֹͣ
				Speed_R=0;
			
				detector3_a=0;//���������㣬��֤������ε�������
				detector3_b=0;
		}
		if(Range_include_first==0&&(Range>target_instance+5)){
				detector3_a++; //��������
				detector3_b=0;//���㱣֤������
			if(detector3_a==3){//����������ת��������ת
				motor_flag=1;//�����ת
				Speed_L=4;   //������ʻ
				Speed_R=4;
				detector3_a=0;
			}
			
		}
		if(Range_include_first==0&&(Range<target_instance-5)){
				detector3_b++;
				detector3_a=0;
			if(detector3_b==3){//�������η�ת������ת
				buzzer_led_flag=1;//����
				motor_flag=0;//�����ת
				Speed_L=4;   //������ʻ
				Speed_R=4;
				detector3_b=0;
			}
			
		}else{
			buzzer_led_flag=0;
		}
		//========================================================
		
		Delay(10);//10ms��ʱ
		
    //=========������������ʾ==========================
		if(display_flag){//
			Range_Display();
			display_flag=0;
		}
		//==============================
		
		
		//==============��λ�������==================
		//Delay(20);//��������������ǰ�泬��������ʱ���Բ���
		k = KeyScan();			//ɨ�谴��
		if ( k != '\0')			//�״μ�⵽��������
		{
			Delay(20);					//��ʱ20ms������ǰ������
			k = KeyScan();			//�ٴζ�ȡ����״̬
			if ( k != '\0')			//ȷ�ϰ������º�����
			{
				if ( k == '+') 
				{
					buzzer=0;//������ʾ��
					Delay(100);
					buzzer=1;
					
					if ( target_instance < 150 ) 
						target_instance=target_instance+5;
				}
				if ( k == '-')
				{
					buzzer=0;//������ʾ��
					Delay(100);
					buzzer=1;
					
					if ( target_instance > 100 )
						target_instance=target_instance-5;
					
				}
				
				DispBuf[0] = Tab[target_instance % 10];		  
				DispBuf[1] = Tab[target_instance / 10 % 10] + 0x80;
				DispBuf[2] = Tab[target_instance / 100 % 10];	

				while( KeyScan() != '\0');		//�ȴ��ɿ�����
			}
		}
		//==============================================
		
		//=============adc�ɼ���������================
			PCF8591_SendByte(AddWr,0);
			light = PCF8591_RcvByte(AddWr);  		
			Pcf8591_DaConversion(AddWr,0, light);//��ad��������Ϊda���  		
		//======�ڹ�����==================
			if(light>130){
			timer_start_flag=1;//��ʱ����
			PWM_EN1 = 1;			//�����Ч	
			PWM_EN2 = 1;			
		}
		//=========================================
		
		
	}

}

