#include "init.h"
#include "Seg.h"
#include "Key.h"
#include "led.h"
#include "onrwire.h"
#include "ultrasound.h"
#include "iic.h"
pdata unsigned char Seg_Buf[]={10,10,10,10,10,10,10,10};
idata unsigned char Seg_Pos=0;
idata unsigned long int uwTick=0;
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata unsigned char Key_Val=0,Key_Down=0,Key_Old=0,Key_Up=0;
idata unsigned char Temperature=23;
idata unsigned long int  velocity=0;
unsigned char  Mode=0;//0 单次测量 1 累加测量  2 测距界面
idata unsigned long int  Distance=0;
idata unsigned  long int Distance_1=0;
idata unsigned char  times=3;//次数
idata unsigned char  time=2;
idata unsigned char Seg_Show_Mode=0;






















void Key_Proc()
{
	Key_Val  = Key_Read();
    Key_Down = Key_Val  & (Key_Val  ^ Key_Old); // 按下沿：当前为1，上次为0
    Key_Up   = ~Key_Val & (Key_Val  ^ Key_Old); // 松开沿：当前为0，上次为1
    Key_Old  = Key_Val;  
	switch(Key_Down)
	{
		case 4:
			if(Seg_Show_Mode==0)
			Mode=0;
			
		break;
		case 5:
			if(Seg_Show_Mode==0)
			Mode=1;
			
		break;
		case 8:
			Seg_Show_Mode=(++Seg_Show_Mode)%3;
			
		break;
		case 9:
			if(Seg_Show_Mode==0)//时间参数
			{
				time=(time>6)?4:time+2;
				
				
			
			
			
			}
			else if(Seg_Show_Mode==1)
			{
			
			
			
			}
			
		
		break;
		case 12:
			Distance=0;
			Mode=2;
		break;
	
	
	}
	
	
	





}


void Led_Proc()
{
	idata unsigned char i;
	for(i=0;i<8;i++)
	{
		ucled[i]=0;
	}
	if(Seg_Show_Mode==0)
	{
		ucled[0]=1;
	
	}

	if(Mode==0&&(Distance<20|Distance>80))
	{
		ucled[1]=1;
	
	}
	else if(Mode==1&&(Distance_1<20|Distance_1>80))
	{
		ucled[1]=1;
	}
	if(Mode==1)
	{
		ucled[2]=(uwTick/100)%2;
	
	}






led_disp(ucled);
}


void Seg_Proc()
{
	switch(Seg_Show_Mode)
		
	{
		case 0:
			if(Mode==0)
			{
				Seg_Buf[0]=11;
				Seg_Buf[1]=12;
				Seg_Buf[2]=Temperature/10%10;
				Seg_Buf[3]=Temperature%10;
				Seg_Buf[4]=((Distance/1000)%10==0)?10:(Distance/1000)%10;
				Seg_Buf[5]=((Distance/100)%10==0)?10:(Distance/100)%10;
				Seg_Buf[6]=((Distance/10)%10==0)?10:(Distance/10)%10;
				Seg_Buf[7]=Distance%10;
			}
			else if (Mode==1)
			{
				Seg_Buf[0]=13;
				Seg_Buf[1]=12;
				Seg_Buf[2]=Temperature/10%10;
				Seg_Buf[3]=Temperature%10;
				Seg_Buf[4]=((Distance_1/1000)%10==0)?10:(Distance_1/1000)%10;
				Seg_Buf[5]=((Distance_1/100)%10==0)?10:(Distance_1/100)%10;
				Seg_Buf[6]=((Distance_1/10)%10==0)?10:(Distance_1/10)%10;
				Seg_Buf[7]=Distance_1%10;
				
			
			
			
			}
			else if(Mode==2)
			{
				Seg_Buf[0]=12;
				Seg_Buf[1]=12;
				Seg_Buf[2]=Temperature/10%10;
				Seg_Buf[3]=Temperature%10;
				Seg_Buf[4]=10;
				Seg_Buf[5]=10;
				Seg_Buf[6]=10;
				Seg_Buf[7]=10;
			
			
			
			}
		break;
		case 1:
		Seg_Buf[0]=14;
		Seg_Buf[1]=1;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=10;
		Seg_Buf[6]=10;
		Seg_Buf[7]=time;
			
		break;
		case 2:
		Seg_Buf[0]=14;
		Seg_Buf[1]=2;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=10;
		Seg_Buf[6]=10;
		Seg_Buf[7]=times;
			
		break;
		
	
	
	
	}







}


void Da_Proc()
{
	if(Distance<=20)
	{
		Da_Write(51);
		
	
	
	}
	else if(Distance>20&&Distance<100)
	{
		Da_Write(51*((Distance-20)/20+1));
	}
	else if(Distance>100)
	{
		Da_Write(51*5);
	
	}







}
void Ultrasound_Proc()
{
	Temperature= Rd_Temperature();
	Distance=Ut_Wave_Dat();
	Distance_1=Ut_Wave_Dat();
	



}










void Timer1_Init(void)		//1ms@12.000MHz
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


void Timer1_Isr()  interrupt 3
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
		 unsigned long int  rate_ms;
		 unsigned long int last_ms;
	


}task_t;

idata task_t Scheduler_Task[]=
{
	{Led_Proc,1,0},
	{Key_Proc,10,0},
	{Seg_Proc,20,0},
	{Ultrasound_Proc,1000,0},
	{Da_Proc,50,0},



};

idata unsigned char task_num=0;

void Scheduler_Init()
{
	task_num=sizeof(Scheduler_Task)/sizeof(task_t);



}


void Scheduler_Run()
{
	idata unsigned char i;
	
	for(i=0;i<task_num;i++)
	{
			idata unsigned  long int  now_time=uwTick;
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
	
		{
			Scheduler_Run();

		}


	}


}
