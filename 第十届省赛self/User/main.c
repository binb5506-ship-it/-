#include "init.h"
#include "seg.h"
#include "iic.h"       // 可能包含了一些I2C相关的宏定义或函数声明
#include "intrins.h"   // 包含了Keil C51编译器提供的内部函数，如_nop_()
#include "key.h"
#include "led.h"
#include "stdio.h"
#include "string.h"




idata unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;
idata unsigned char Seg_Pos=0;
pdata unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
pdata unsigned char ucled[8]={0,0,0,0,0,0,0,0};
idata unsigned long int uwTick=0;
idata unsigned char Seg_Show_Mode=0;//0为电压测量  1为频率测量
idata bit Led_Mode=0;//0为led关闭状态 1为启用状态
idata bit Seg_Mode=0;//0为数码管关闭状态 1为启用状态
idata unsigned int AD_in_100x=0;
idata unsigned long Freq;
idata bit Da_Mode=0;//0固定 2 ；1跟随
//按键处理函数
void Key_Proc()
{
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	
	switch(Key_Down)
	{
		case 4:
			Seg_Show_Mode=(++Seg_Show_Mode)%2;
			
		break;
		case 5:
			Da_Mode^=1;
			
		break;
		
		case 6:
				Led_Mode^=1;
		break;
		case 7:
				Seg_Mode^=1;
		break;
	
	
	
	}
	
	
	
	

}



void Seg_Proc()
{
	
	if(Seg_Mode==1)
{
	if(Seg_Show_Mode==0)//电压测量
	{
		Seg_Buf[0]=12;		
		Seg_Buf[1]=10;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=AD_in_100x/100%10+',';
		Seg_Buf[6]=AD_in_100x/10%10;
		Seg_Buf[7]=AD_in_100x%10;
		
	}
	else if(Seg_Show_Mode==1)//频率测量
		
	{
			Seg_Buf[0] = 11; // F
			Seg_Buf[1] = 10;
			Seg_Buf[2] = (Freq / 100000 % 10 == 0) ? 10 : Freq / 100000 % 10;
			Seg_Buf[3] = ((Freq / 10000 % 10 == 0) && (Seg_Buf[2] == 10)) ? 10 : Freq / 10000 % 10;
			Seg_Buf[4] = ((Freq / 1000 % 10 == 0) && (Seg_Buf[3] == 10)) ? 10 : Freq / 1000 % 10;
			Seg_Buf[5] = ((Freq / 100 % 10 == 0) && (Seg_Buf[4] == 10)) ? 10 : Freq / 100 % 10;
			Seg_Buf[6] = ((Freq / 10 % 10 == 0) && (Seg_Buf[5] == 10)) ? 10 : Freq / 10 % 10;
			Seg_Buf[7] = ((Freq % 10 == 0) && (Seg_Buf[6] == 10)) ? 10 : Freq % 10;
		
		
		
	
	
	}
}

	else if(Seg_Mode==0)
	{
		Seg_Buf[0]=10;
		Seg_Buf[1]=10;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=10;
		Seg_Buf[6]=10;
		Seg_Buf[7]=10;
	
	
	}



}


void Led_Proc()
{
	if(Led_Mode==1)
	{
	ucled[0]=(Seg_Show_Mode==0);
	ucled[1]=(Seg_Show_Mode==1);
	ucled[2] = (((AD_in_100x >= 150) && (AD_in_100x < 250)) ||
					(AD_in_100x >= 350));
		ucled[3] = (((Freq >= 1000) && (Freq < 5000)) ||
					(Freq >= 10000));
		ucled[4] = Da_Mode;

	}
	
	else if(Led_Mode==0)
	{
		 Led_Off();
	}
	Led_Disp(ucled);
}

void Da_Proc()
{
	unsigned char temp = Ad_Read(0x43);
	AD_in_100x = temp * 100 / 51;
	// 如果DA跟随AD
	if (Da_Mode)
		Da_Write(temp);
	// 如果DA固定输出
	else
		Da_Write(2 * 51);



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