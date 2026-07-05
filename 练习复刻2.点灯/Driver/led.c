# include "led.h"


unsigned  char  temp_old=0x00;//全灭

void led_disp(unsigned char ucled)//ucled=0x00 ~ucled=0xff  
{
	if (ucled!=temp_old)
	{
		//给P0的P0引脚赋值
		P0=~ucled;
		//选通y4
		P2=P2&0X1F|0X80;
		//关闭y4
		P2=P2&0X1F;
		
		temp_old=ucled;
	}



}