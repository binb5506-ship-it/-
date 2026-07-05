#include "ultrasound.h"
#include "intrins.h"


sbit US_TX=P1^0;
sbit US_RX=P1^1;

void Delay12us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	i = 38;
	while (--i);
}



void Ultra_Data_Init()
{
	unsigned char i=0;
	EA=0;
	for(i=0;i<8;i++)
	{
		US_TX=1;
		Delay12us();
		US_TX=0;
		Delay12us();
	
	
	}
	EA=1;







}



unsigned int  Ultra_Data_Wave()
{
	unsigned long int time=0;
	unsigned char i=0;
	 Ultra_Data_Init();
	CMOD=0X00;
	CH=CL=0;
	CR=1;
	while((CF==0)&&(US_RX==1));
	CR=0;
	if(CF==0)
	{
		time=CH<<8|CL;
		return time*0.017+2;
		
	
	}
	else
	{
		CF=0;
		return 999;
		
		
	
	
	}
	










}








