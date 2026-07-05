#include "init.h"
#include "key.h" 
#include "seg.h" 
#include "led.h"
#include "iic.h"       // 可能包含了一些I2C相关的宏定义或函数声明
#include "intrins.h"   // 包含了Keil C51编译器提供的内部函数，如_nop_()
#include "stdio.h"
#include "string.h"


//按键专用变量
idata unsigned char Key_Val,Key_Old,Key_Up,Key_Down;
//数码管专用变量
pdata unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
idata unsigned char Seg_Pos=0;
//led专用变量
pdata  unsigned char ucled[8]={0,0,0,0,0,0,0,0};
//Timer1 系统滴答计时器               
idata unsigned long int uwTick;

//AD/DA输出
idata unsigned int AD_in_100x;
idata unsigned char DA_Out;
//频率
idata unsigned int Freq;

idata unsigned char Seg_Show_Mode=0;
idata bit DA_Out_Mode=0;
idata bit Led_Mode=1;//是否开启功能  1 启用 0 关闭
idata bit Seg_Mode=1;
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
			DA_Out_Mode^=1;
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
	if(Seg_Mode)
	{
		switch(Seg_Show_Mode)
		{
			case 0://电压
				Seg_Buf[0]=11;
				Seg_Buf[1]=10;
				Seg_Buf[2]=10;
				Seg_Buf[3]=10;
				Seg_Buf[4]=10;
				Seg_Buf[5]=AD_in_100x/100+',';
				Seg_Buf[6]=AD_in_100x/10%10;
				Seg_Buf[7]=AD_in_100x%10;
			
			break;
			
			
			case 1://频率
				Seg_Buf[0]=12;
				Seg_Buf[1]=10;
				Seg_Buf[2]=(Freq/100000%10==0)?0:Freq/100000%10;
				Seg_Buf[3]=((Freq/10000%10==0)&&(Seg_Buf[2]==0))?0:Freq/10000%10;
				Seg_Buf[4]=((Freq/1000%10==0)&&(Seg_Buf[3]==0))?0:Freq/1000%10;
				Seg_Buf[5]=((Freq/100%10==0)&&(Seg_Buf[4]==0))?0:Freq/100%10;
				Seg_Buf[6]=((Freq/10%10==0)&&(Seg_Buf[5]==0))?0:Freq/10%10;
				Seg_Buf[7]=((Freq%10==0)&&(Seg_Buf[6]==0))?0:Freq%10;
			
			break;
		
		
		}
	
	}
	else
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
	
	
	if(Led_Mode)
	{
		ucled[0]=(Seg_Show_Mode==0);
		ucled[1]=(Seg_Show_Mode==1);
		ucled[2]=((DA_Out>=1.5*51)&&(DA_Out<2.5*51)||DA_Out>=3.5*51);
		ucled[3]=(((Freq>=1000)&&(Freq<5000))||(Freq>=10000));
		ucled[4]=DA_Out_Mode;
	
	}
	else
	{
		ucled[0]=0;
		ucled[1]=0;
		ucled[2]=0;
		ucled[3]=0;
		ucled[4]=0;
		ucled[5]=0;
		ucled[6]=0;
		ucled[7]=0;
	
	
	
	
	}	
	Led_Disp(ucled);


}

void AD_DA()
{
	unsigned char temp=Ad_Read(0x43);
	AD_in_100x=temp*100/51;
	
	if(DA_Out_Mode)
	Da_Write(temp);
	else
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
	{AD_DA,50,0},





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
	Timer0_Init();
	Scheduler_Init();

	
	




	while(1)
	{
		Scheduler_Run();
	
	}

}



