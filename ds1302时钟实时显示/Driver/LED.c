#include "LED.h"

void led_disp(unsigned char ucled)
{
	

	
	P0=~ucled;//给P0口P00引脚赋值
	P2=P2&0X1F|0X80;//选通y4
	P2=P2&0X1F;//关闭y4








}








