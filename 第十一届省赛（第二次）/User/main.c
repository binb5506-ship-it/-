#include "init.h"
#include "led.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "iic.h"       // 可能包含了一些I2C相关的宏定义或函数声明
#include "intrins.h"   // 包含了Keil C51编译器提供的内部函数，如_nop_()
#include "stdio.h"
#include "string.h"



idata unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;
idata unsigned char Seg_Pos=0;
pdata unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
pdata unsigned char ucled[8]={0,0,0,0,0,0,0,0};
idata unsigned long int uwTick=0;
idata bit Seg_Show_Mode=0;//0 数据显示  1 参数设置
idata unsigned char Temp=0;//0 低温  1  高温
idata unsigned char T_Max=30,T_Min=20;
idata unsigned char wendu=0;//测的温度



void Key_Proc()
{
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;

	
	switch(Key_Down)
	{
		case 4:
			Seg_Show_Mode^=1;
			Temp=0;
		break;
		case 5:
			Temp^=1;
		break;
		
		
	
	
	}
	
	if(Seg_Show_Mode==1)//参数设置界面
	{
		
		if(Temp==0)//低温模式
	{
		switch(Key_Down)
		{
			case 6:
				T_Min=(T_Min>=100)?0:T_Min+1;
			break;
		case 7:
				T_Min=(T_Min<0)?99:T_Min-1;
			break;
		
		}
	
	}
	
		else if(Temp==1)//高温模式
		{
			switch(Key_Down)
			{
			case 6:
				T_Max=(T_Max>=100)?0:T_Max+1;
			break;
		case 7:
				T_Max=(T_Max<0)?99:T_Max-1;
			break;
			}
			
			if(T_Min<T_Max)
				return;
			
			
		
		}


	}
}



void Seg_Proc()
{
	
	if(Seg_Show_Mode==1)//参数设置界面
	{
		Seg_Buf[0]=12;
		Seg_Buf[1]=10;
		Seg_Buf[2]=10;
		Seg_Buf[3]=(T_Max/10%10==0)?10:T_Max/10%10;
		Seg_Buf[4]=T_Max%10;
		Seg_Buf[5]=10;
		Seg_Buf[6]=(T_Min/10%10==0)?10:T_Min/10%10;;
		Seg_Buf[7]=T_Min%10;
		
	
	
	}
	
	else if(Seg_Show_Mode==0)//数据显示
	{
		wendu=rd_temperature();
		Seg_Buf[0]=11;
		Seg_Buf[1]=10;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=10;
		Seg_Buf[6]=(wendu/10%10==0)?10:wendu/10%10;
		Seg_Buf[7]=wendu%10;
	
	
	
	}
	
	






}



void Led_Proc()
{
	if(wendu>T_Max)
		ucled[0]=1;
	else if(wendu<=T_Max&&wendu>=T_Min)
		ucled[1]=1;
	else if(wendu<T_Min)
		ucled[2]=1;
	else if(T_Min>=T_Max)
		ucled[3]=1;

	



 Led_Disp(ucled);
}



void Da_Proc()
{
	if(wendu>T_Max)
		Da_Write(4*51);
	else if(wendu<=T_Max&&wendu>=T_Min)
		Da_Write(3*51);
	else if(wendu<T_Min)
		Da_Write(2*51);




}


void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1=1;
	EA=1;

}


void Timer1_Isr(void) interrupt 3
{
	uwTick++;
	Seg_Pos=(++Seg_Pos)%8;
	
	if(Seg_Buf[Seg_Pos]>20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
	else 
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);

	





}


typedef struct
{
	void (*task_func)(void);
	unsigned long int rate_ms;
	unsigned long int last_ms;




}task_t;


idata task_t Scheduler_Task[]=
{
	{Led_Proc,1,0},
	{Key_Proc,10,0},
	{Seg_Proc,20,0},
	{Da_Proc,50,0},
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
		unsigned long int now_Time=uwTick;
		if(now_Time>=(Scheduler_Task[i].rate_ms+Scheduler_Task[i].last_ms))
		{
				Scheduler_Task[i].last_ms=now_Time;
				Scheduler_Task[i].task_func();
		
		}
	
	}



}




void main()
{
	
	System_Init();
	Scheduler_Init();
	Timer1_Init();


	while(1)
	{
				Scheduler_Run();
	
	
	}


}