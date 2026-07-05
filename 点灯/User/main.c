#include <STC15F2K60S2.H>
#include "LED.h"
#include "Init.h"
#include "Intrins.h"
unsigned char ucled;

void Delay500us(void)	//@12.000MHz
{
	unsigned char data i, j;

	i = 6;
	j = 211;
	do
	{
		while (--j);
	} while (--i);
}
void main()
{
    	
	per_init();//外设初始化，关闭所有LED灯
	ucled=0x01;


	while(1)
	{
		
		Delay500us();
			ucled=0x01;//第一个灯亮
			led_disp(ucled);	
		Delay500us();Delay500us();
		Delay500us();Delay500us();
		Delay500us();Delay500us();
		Delay500us();Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
			ucled=0x02;//第一个灯亮
			led_disp(ucled);	
		Delay500us();		Delay500us();
	Delay500us();		Delay500us();
			Delay500us();		Delay500us();
		
			Delay500us();		Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		ucled=0x04;//第一个灯亮
			led_disp(ucled);	
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		
			ucled=0x08;//第一个灯亮
			led_disp(ucled);	
		Delay500us();Delay500us();
		Delay500us();Delay500us();
		Delay500us();Delay500us();
		Delay500us();Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		
		
			ucled=0x10;//第一个灯亮
			led_disp(ucled);	
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
			Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
			ucled=0x20;//第一个灯亮
			led_disp(ucled);	
		Delay500us();Delay500us();
		Delay500us();Delay500us();
		Delay500us();Delay500us();
		Delay500us();Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		
		
		
			ucled=0x40;//第一个灯亮
			led_disp(ucled);
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
			ucled=0x80;//第一个灯亮
			led_disp(ucled);
			Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
		Delay500us();	Delay500us();
	}

}
