#include <STC15F2K60S2.H>
#include "LED.h"
#include "Init.h"
#include <z_init.h>
//led专用变量
unsigned char ucled;


void  main()
{
	
	z_init();
	per_init();//外设初始化
	while (1)
	{
	
	
	
	
	}


}


void isr1(void)  interrupt 0
{
	//s5按下之后led1进行反转
	ucled^=0X01;//相同为0，不同为1
	led_disp(ucled);





}


void isr2(void) interrupt 2
{
	//s4按下之后led2进行反转
	ucled^=0X02;//相同为0，不同为1
	led_disp(ucled);






}