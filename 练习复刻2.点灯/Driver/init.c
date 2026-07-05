#include "init.h"


void system_init()
{
	
		//关闭led灯
		P0=0XFF;
		P2=P2&0X1F|0X80;//1000 0000
		P2=P2&0X1F;
	
	
		//关闭蜂鸣器，继电器；
		P0=0X00;
		P2=P2&0X1F|0XA0;//1010 0000
		P2=P2&0X1F;



}