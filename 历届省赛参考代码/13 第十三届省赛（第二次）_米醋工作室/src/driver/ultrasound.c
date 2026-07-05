#include "ultrasound.h"
#include "intrins.h"

sbit US_TX=P1^0;
sbit US_RX=P1^1;

//延时（不精确）
void Delay12us()		//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 38;//33~38自行调整
	while (--i);
}
/* 初始化超声波 */
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

/* 超声波计算 */

unsigned char Ut_Wave_Data()
{
	unsigned int time;//时间
	CMOD=0x00;//12T模式，PCA工作，禁止中断
	CH=CL=0;//手动给初始值，等待发送
	Ut_Wave_Init();
	CR=1;//开始计时
	while((US_RX==1)&&(CF==0));//没接收到返回并且也没有溢出
	CR=0;
	//接收到返回，没有溢出
	//v=340m/s=3.4*10^4cm/s
	//t=1us=10^(-6)s
	//x=vt/2=1.7*10^(-2)cm=0.017cm
	if(CF==0)
	{
		time=CH<<8|CL;
		return (time*0.017);//cm
	}
	//接收到返回前溢出，测量无效
	else 
	{
		CF=0;
		return 0;
	}
}
