#include "seg.h"
#include "led.h"
#include "key.h"
#include "init.h"

pdata unsigned char ucled[]={1,0,1,0,1,0,1,0};
pdata unsigned char Seg_Buf[]={10,10,10,10,10,10,10,10};
idata unsigned char Seg_Pos=0;
idata unsigned long int uwTick=0;

idata unsigned char k=0;

void Led_Proc()
{


Led_Disp(ucled);
}


void Seg_Proc()
{
	k=Key_Read();
	if(k!=0)
	{
	Seg_Buf[1] = k/10%10;		// 想显示什么就写到 Seg_Buf，由中断去刷新显示
		Seg_Buf[2] = k%10;
	}
	}
void Timer1_Init(void)		//1����@12.000MHz
{
	AUXR |= 0x40;			//��ʱ��ʱ��1Tģʽ
	TMOD &= 0x0F;			//���ö�ʱ��ģʽ
	TL1 = 0x20;				//���ö�ʱ��ʼֵ
	TH1 = 0xD1;				//���ö�ʱ��ʼֵ
	TF1 = 0;				//���TF1��־
	TR1 = 1;				//��ʱ��1��ʼ��ʱ
	ET1=1;
	EA=1;

}

void Isr_Timer1() interrupt 3
{
	uwTick++;
	Seg_Pos=(++Seg_Pos)%8;
	if(Seg_Buf[Seg_Pos]>20)
	{
		 Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
	
	}
	else
	{
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	}



}

typedef struct
{
	void (*task_func)(void);
	unsigned  long int rate_ms;
	unsigned  long int last_ms;



}task_t;


idata task_t Scheduler_Task[]=
{
	{Led_Proc,20,0},
	{Seg_Proc,10,0},


};
idata unsigned char task_num=0;

void Scheduler_Init()
{
	task_num=sizeof(Scheduler_Task)/sizeof(task_t);
	
	





}


void Scheduler_Run()
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
	

	
	System_Init();
	 Scheduler_Init();
	Timer1_Init();
	 
	
	while(1)
	{
		
	Scheduler_Run();
	}



}
