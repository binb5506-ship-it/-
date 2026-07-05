#include "led.h"

unsigned char temp_old=0x00;//全灭


void led_disp(unsigned char ucled)//ucled=0x00 ~ucled=0xff 1111 1110
{
	
	
	if(ucled!=temp_old)
	{
		//给p0的p00引脚赋值
		P0=~ucled;//0000 0000//p07 p06...p00//0011 0001
		
		//选通y4
	 
		P2=P2&0X1F|0X80;//0001 1111 1000 0000
	 
		//关闭y4
		P2=P2&0X1F;
		
		temp_old=ucled;
	}
}

unsigned char temp1=0x00;
unsigned char temp1_old=0xff;


void relay(bit enble)
{
	if(enble)
	temp1|=0x10;//0001 0000
	else
	temp1&=~0x10;
	
	if(temp1!=temp1_old)
	{
		P0=temp1;
		P2=P2&0X1F|0XA0;//1011-y5 
		P2=P2&0X1F;
		temp1_old=temp1;		
	}
	
}




