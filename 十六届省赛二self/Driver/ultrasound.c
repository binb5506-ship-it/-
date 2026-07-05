#include "ultrasound.h"
#include "intrins.h"
#include "onrwire.h"
sbit US_TX=P1^1;
sbit US_RX=P1^0;

void Delay12us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	i = 3;
	while (--i);
}


void Ut_Wave_Init()
{
	unsigned char i;
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


unsigned char Ut_Wave_Dat()
{
	idata unsigned char temperature=Rd_Temperature();

	idata unsigned long int time=0;
	 Ut_Wave_Init();
	CMOD=0X00;
	CH=CL=0;
	CR=1;
	while((CF==0)&&(US_RX==1));
	CR=0;
	if(CF==0)
	{
		time=(CH<<8|CL);
		return time*(0.015+0.00003*temperature);
		
	
	}
	else
	{
		time=0;
		return 0;
	
	}





}

