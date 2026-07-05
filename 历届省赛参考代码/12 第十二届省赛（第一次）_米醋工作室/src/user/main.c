#include <STC15F2K60S2.H>
#include "key.h"
#include "led.h"
#include "seg.h"
#include "init.h"
#include "onewire.h"
#include "iic.h"
#include "intrins.h"

/* 变量 */
idata unsigned long int uwTick=0;//计时器
//按键
idata unsigned char Key_Val,Key_Old,Key_Down,Key_Up;
//LED
pdata unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
//数码管
idata unsigned char Seg_Pos=0;
pdata unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};

idata unsigned char Seg_Show_Mode=0;//界面显示 0 温度显示 1 参数设置 2 DAC输出
idata bit DAC_Out_Mode=0;//DAC输出模式 0 高于温度参数输出5，低于温度参数输出0 1 根据温度线性输出
idata unsigned int Temputure_Value_100x=0;//温度测量100倍
idata unsigned char Temputure_Para=25;//温度参数
idata unsigned char Temputure_Para_Ctrl=0;//温度参数控制值
idata unsigned int DAC_Out_100x=0;//DAC输出的模拟电压
/* 按键 */
void Key_Proc()
{
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	if(Key_Down==4)
	{
		//处于温度显示界面，将参数真实值给控制值
		if(Seg_Show_Mode==0)
			Temputure_Para_Ctrl=Temputure_Para;
		//处于参数设置界面，将参数控制值给真实值
		else if(Seg_Show_Mode==1)
			Temputure_Para=Temputure_Para_Ctrl;
		Seg_Show_Mode=(++Seg_Show_Mode)%3;
	}		
	if(Key_Down==5)
		DAC_Out_Mode^=1;
	//当前在参数设置界面
	if(Seg_Show_Mode==1)
	{
		if(Key_Down==8)
			Temputure_Para_Ctrl=(Temputure_Para_Ctrl==0)?99:Temputure_Para_Ctrl-1;
		else if(Key_Down==9)
			Temputure_Para_Ctrl=(Temputure_Para_Ctrl==99)?0:Temputure_Para_Ctrl+1;
	}
}

/* 数码管 */
void Seg_Proc()
{
	switch(Seg_Show_Mode)
	{
		case 0:
			/* 温度显示 */
		Seg_Buf[0]=11;//C
		Seg_Buf[1]=10;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=Temputure_Value_100x/1000%10;
		Seg_Buf[5]=Temputure_Value_100x/100%10+',';
		Seg_Buf[6]=Temputure_Value_100x/10%10;
		Seg_Buf[7]=Temputure_Value_100x%10;
		break;
		case 1:
			/* 参数设置 */
		Seg_Buf[0]=12;//P
		Seg_Buf[1]=10;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=10;
		Seg_Buf[6]=Temputure_Para_Ctrl/10%10;
		Seg_Buf[7]=Temputure_Para_Ctrl%10;
		break;
		case 2:
			/* DAC输出 */
		Seg_Buf[0]=13;//A
		Seg_Buf[1]=10;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=DAC_Out_100x/100%10+',';
		Seg_Buf[6]=DAC_Out_100x/10%10;
		Seg_Buf[7]=DAC_Out_100x%10;
		break;
	}
}

/* LED */
void Led_Proc()
{
	ucLed[0]=(!DAC_Out_Mode);
	ucLed[1]=(Seg_Show_Mode==0);
	ucLed[2]=(Seg_Show_Mode==1);
	ucLed[3]=(Seg_Show_Mode==2);
	Led_Disp(ucLed);
}

/* 温度 */
void Get_Temputure()
{
	Temputure_Value_100x=rd_temputure()*100;
}
/* AD_DA*/
void AD_DA()
{
	float DA_Out_temp=0;
	//模式1
	if(DAC_Out_Mode==0)
	{
		if(Temputure_Value_100x>=Temputure_Para*100)
		{
			DAC_Out_100x=500;
			Da_Write(5*51);
		}		
		else 
		{
			DAC_Out_100x=0;
			Da_Write(0*51);
		}
	}
	//模式2
	else 
	{
		if(Temputure_Value_100x<=2000)DA_Out_temp=1;
		else if(Temputure_Value_100x>=4000)DA_Out_temp=4;
		else DA_Out_temp =3*(float)(Temputure_Value_100x-2000)/2000.0f+1;
		Da_Write(DA_Out_temp*51);
		DAC_Out_100x=DA_Out_temp*100;
	}
}

/* 定时器 */
void Timer1Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;		//定时器时钟12T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x18;		//设置定时初值
	TH1 = 0xFC;		//设置定时初值
	TF1 = 0;		//清除TF1标志
	TR1 = 1;		//定时器1开始计时
	ET1 = 1;
	EA =1;
}

void Timer1Isr()interrupt 3
{
	uwTick++;
	Seg_Pos=(++Seg_Pos)%8;
	if(Seg_Buf[Seg_Pos]>20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
	else 
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
}

/* 调度器 */
typedef struct
{
	void(*task_func)(void);//任务函数
	unsigned long int rate_ms;//任务执行周期
	unsigned long int last_ms;//任务最后一次时间
}task_t;

idata task_t Scheduler_Task[]=
{
	{Led_Proc,1,0},
	{Key_Proc,10,0},
	{Seg_Proc,200,0},
	{Get_Temputure,300,0},
	{AD_DA,160,0}
};

idata unsigned char task_num;

void Scheduler_Init()
{
	task_num=sizeof(Scheduler_Task)/sizeof(task_t);
}

void Scheduler_Run()
{
	unsigned char i;
	for(i=0;i<task_num;i++)
	{
		unsigned long int now_time=uwTick;
		if(now_time>Scheduler_Task[i].rate_ms+Scheduler_Task[i].last_ms)
		{
			Scheduler_Task[i].last_ms=now_time;
			Scheduler_Task[i].task_func();
		}
	}
}

void Delay750ms()		//@12.000MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 35;
	j = 51;
	k = 182;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void main()
{
	System_Init();
	rd_temputure();
	Delay750ms();
	Scheduler_Init();
	Timer1Init();
	while(1)
	{
		Scheduler_Run();
	}
}