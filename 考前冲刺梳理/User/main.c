#include "ds1302.h"











idata unsigned char Seg_Pos=0;
pdata unsigned char Seg_Buf[]={10,10,10,10,10,10,10,10};
idata unsigned long int uwTick=0;
idata unsigned char Ua_Rx_Index=0,Ua_Rx_Tick=0,Ua_Rx_Flag=0;
pdata unsigned char Ua_Rx_Buf[10]={};
void Ua_Proc()
{
	if(Ua_Rx_Index==0) return;
	if(Ua_Rx_Tick>=10)
	{
		Ua_Rx_Flag=0;
		Ua_Rx_Tick=0;
		
		
		
		memset(Ua_Rx_Buf,0,Ua_Rx_Index);
		Ua_Rx_Index=0;
	
	
	}





}











void Uart1_Isr(void) interrupt 4
{

	if (RI)				//检测串口1接收中断
	{
		Ua_Rx_Flag=1;
		Ua_Rx_Tick=0;
		Ua_Rx_Buf[Ua_Rx_Index]=SBUF;
		RI = 0;			//清除串口1接收中断请求位
		if(Ua_Rx_Index>10)
		{
			Ua_Rx_Index=0;
			memset(Ua_Rx_Buf,0,10);
			
		
		}
		
		
	
	
	
	}
}









void Timer1_Isr(void) interrupt 3
{
	uwTick++;
	Seg_Pos=(++Seg_Pos)%8;
	if(Seg_Buf[Seg_Pos]>20)
	{
		Seg_Disp( Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
		
	}
	else
	{
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	}
	if(Ua_Rx_Flag)
		Ua_Rx_Tick++;
	
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

typedef struct
{
	void (*task_func)(void);
	unsigned long int rate_ms;
	unsigned long int last_ms;



}task_t;

idata task_t Scheduler_Task[]=
{


};

idata unsigned char task_num;

void Scheduler_Init()
{
	task_num=sizeof(Scheduler_Task)/sizeof(task_t);



}

void Scheduler_Run()
{
	idata unsigned char i;
	idata unsigned long int now_time=uwTick;
	if(now_time>Scheduler_Task[i].last_ms+Scheduler_Task[i].rate_ms)
	{
		Scheduler_Task[i].last_ms=now_time;
		Scheduler_Task[i].task_func();
	
	}




}


void main()
{
	Scheduler_Init;
	Uart1_Init();
	


	while(1)
	{
		Scheduler_Run();
		
	
	}

}













