#include "led.h"

unsigned char temp_1=0x00;
unsigned char Temp_1_Old=0xff;


void Led_Disp(unsigned char *ucled)
{
	unsigned char temp=0;
	temp_1=0x00;
	temp_1=ucled[0]<<0|ucled[1]<<1|ucled[2]<<2|ucled[3]<<3|ucled[4]<<4|ucled[5]<<5|ucled[6]<<6|ucled[7]<<7;
	if(temp_1!=Temp_1_Old)
	{
		P0=~temp_1;
		temp=P2&0x1f;
		temp=temp|0x80;
		P2=temp;
		temp=P2&0x1f;
		P2=temp;
		
		Temp_1_Old=temp_1;
		
		
		
		
	
	
	}
	







}
