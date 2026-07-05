#include "led.h"
#include "seg.h"
idata unsigned long int uwTick=0;
pdata unsigned char ucled[]={0,1,0,1,0,1,0,1};
idata unsigned char Seg_Pos=0;
idata unsigned char Seg_Buf[]={10,10,10,10,10,10,10,10};




void led_proc()
{

led_disp(ucled);
}

void seg_proc()
{
	Seg_Buf[1]=2+',';



}











void Timer1_Isr(void) interrupt 3
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
	void(*task_func)(void);
	unsigned long int rate_ms;
	unsigned long int last_ms;





}task_t;

idata task_t Scheduler_Task[]=
{
	{led_proc,20,0},
	{seg_proc,50,0},






};

idata unsigned char task_num;

void Scheduler_Init()
{
	task_num=sizeof(Scheduler_Task)/sizeof(task_t);





}


void Scheduler_Run(void)
{
	unsigned char i;
	unsigned long int now_time=uwTick;
	for(i=0;i<task_num;i++)
	{
		if(now_time>=Scheduler_Task[i].last_ms+Scheduler_Task[i].rate_ms)
		{
			Scheduler_Task[i].last_ms=now_time;
			Scheduler_Task[i].task_func();
		
		
		}
	
	
	
	}





}











void main()
{
	Scheduler_Init();
	Timer1_Init();

	
	while(1)
	{
		Scheduler_Run();
		
	
	
	
	}

}


