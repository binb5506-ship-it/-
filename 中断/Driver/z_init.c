#include <z_init.h>


void z_init(void)
{
	IT0=1;//选择下降沿触发
	EX0=1;//打开外部中断0的中断开关
	
	
	IT1=1;//选择下降沿触发
	EX1=1;//打开外部中断1的中断开发
	EA=1;//打开总中断





}