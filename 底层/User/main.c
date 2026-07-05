#include "led.h"
#include "seg.h"
#include "key.h"
#include "init.h"
#include "ultrasound.h"
#include "intrins.h"
#include "uart.h"
#include "string.h"
#include "stdio.h"




idata unsigned char  dat=0; 
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata unsigned char Seg_Pos=0;
pdata unsigned char  Seg_Buf[]={10,10,10,10,10,10,10,10};
idata unsigned long int uwTick=0;
idata unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;
idata unsigned char Uart_Rx_Index;
pdata unsigned char Uart_Rx_Buf[10];
idata unsigned char Uart_Rx_Flag;
idata unsigned char Uart_Rx_Tick;
idata unsigned char led_count=0;
idata bit Mode=0;	//0=本地按键控制  1=远程串口控制

void Key_Proc()
{
	Key_Val=Key_Read();
	Key_Down=~Key_Val&(Key_Val^Key_Old);
	Key_Up=Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;

	if(Key_Down==7)			//S7切换本地/远程模式
		Mode^=1;

	if(Key_Down==4&&Mode==0)	//仅本地模式下按键控制LED
		led_count=(led_count+1)%4;
}

void Led_Proc()
{
	unsigned char i;
	for(i=0;i<4;i++)
		ucled[i]=(i<led_count)?1:0;
	led_disp(ucled);
}



void Seg_Proc()
{
	
	
	Seg_Buf[0]=10;
	Seg_Buf[1]=10;
	Seg_Buf[2]=10;
	Seg_Buf[3]=10;
	Seg_Buf[4]=dat/1000%10;
	Seg_Buf[5]=dat/100%10;
	Seg_Buf[6]=dat/10%10;
	Seg_Buf[7]=dat%10;
	




}


void Ultrasound_Proc()
{
	dat=Ut_Wave_Data();



}



void Uart_Proc()
{
	if(Uart_Rx_Index==0) return;
	if(Uart_Rx_Tick>=10)
	{
		Uart_Rx_Flag=0;
		Uart_Rx_Tick=0;
		
	
	
	if(Mode==1)	//远程控制模式下才响应
	{
		if(Uart_Rx_Buf[0]=='1')      led_count=3;
		else if(Uart_Rx_Buf[0]=='2') led_count=2;
		else if(Uart_Rx_Buf[0]=='3') led_count=1;
	}
	
	memset(Uart_Rx_Buf,0,Uart_Rx_Index);
	Uart_Rx_Index=0;
	

}


}
















void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
	EA=1;
}



void Timer1_Isr() interrupt 3
{
	uwTick++;
	Seg_Pos=(++Seg_Pos)%8;
	if(Seg_Buf[Seg_Pos]>20)
	{
		seg_disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
	
	}
	else
	{
		seg_disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	}
	if(Uart_Rx_Flag)
		Uart_Rx_Tick++;




}
void Uart1_Isr(void) interrupt 4
{
	
	if (RI)				//检测串口1接收中断
	{
		Uart_Rx_Flag=1;
		Uart_Rx_Tick=0;
		Uart_Rx_Buf[Uart_Rx_Index++]=SBUF;
		RI=0;
		if(Uart_Rx_Index>10)
		{
			Uart_Rx_Index=0;
			memset(Uart_Rx_Buf,0,10);
		
		
		}
		
	}
}




typedef struct
{
	void(*task_func)(void);
	unsigned long int rate_ms;
	unsigned long int last_ms;



}task_t;

idata task_t Scheduler_Task[]=
{
	{Led_Proc,1,0},
	{Seg_Proc,20,0},
	{Key_Proc,10,0},
	{Ultrasound_Proc,200,0},
	{Uart_Proc,10,0},


};


idata unsigned char task_num;

void Scheduler_Init()
{
	task_num=sizeof(Scheduler_Task)/sizeof(task_t);


}

void Scheduler_Run()
{
	unsigned char i;
	unsigned long int Time_Now=uwTick;
	for(i=0;i<task_num;i++)
	{
		if(Time_Now>=Scheduler_Task[i].last_ms+Scheduler_Task[i].rate_ms)
		{
			Scheduler_Task[i].last_ms=Time_Now;
			Scheduler_Task[i].task_func();
			
		
		}
	
	}
	




}



void main()
{
	system_init();
	Uart1_Init();
	printf("start\r\n");
	Scheduler_Init();
	Timer1_Init();
	
	




while(1)
{
	Scheduler_Run();

}

}