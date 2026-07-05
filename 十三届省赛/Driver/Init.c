#include "Init.h"

void cls_init()
{
	
	//外设关闭
	P0=0X00;
	P2=P2&0X1F|0XA0;    //1010 0000 选通y5
	P2=P2&0X1F;//关闭选通y5
	
	//上电关闭所有led
	P0=0XFF;
	P2=P2&0X1F|0X80;    //1010 0000 选通y4
	P2=P2&0X1F;
	
}




