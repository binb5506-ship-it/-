#include "Init.h"

void cls_init(void)
{
	//关闭蜂鸣器，继电器等
	P0=0X00;
	P2=P2&0X1F|0XA0;
	P2=P2&0X1f;
	//关闭LED灯
	P0=0XFF;
	P2=P2&0X1F|0X80;
	P2=P2&0X1F;




}