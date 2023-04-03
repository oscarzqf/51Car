#include<reg52.h>
#define uint unsigned int 
#define uchar unsigned char
void delayms(uint i);
sbit led1=P1^0;
sbit led2=P1^1;
sbit key1=P3^4;
sbit key2=P3^5;
void main(){
	uchar flag=0;
	while(1){
		
		if(key2==0){
			delayms(10);
			if(key2==0){
				flag=!flag;
				while(!key2);//µÈ´ýÊÍ·Å
			}
		}
		
		if(flag==1){
		led1=0;
		delayms(900);
		led1=1;
		delayms(100);
		}else{//°´ÏÂÉÁË¸ÔÝÍ£ÇÒÏ¨Ãð
		led1=1;
		}
		if(key1==0) led2=!led2;
	}
}
void delayms(uint i){
	uint j;
	for(i;i>0;i--){
	for(j=110;j>0;j--);
	}
}