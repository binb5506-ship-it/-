#include <STC15F2K60S2.H>
#include "key.h"
#include "init.h"
#include "seg.h"
#include "led.h"
#include "iic.h"

/* 变量 */
idata unsigned long int uwTick = 0; // 定时器时间
idata unsigned int Time_1s = 0;		// 频率采集1s
// 按键
idata unsigned char Key_Val, Key_Old, Key_Down, Key_Up;
// LED
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// 数码管
idata unsigned char Seg_Pos = 0;
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
// 频率
idata unsigned int Freq = 0;					   // 频率值
idata unsigned int Freq_Cache = 0;				   // 频率值缓存
idata unsigned long Cycle = 0;					   // 周期us 1s=10^6us
idata unsigned char Seg_Show_Mode = 0;			   // 显示界面 0 频率界面 1 周期界面 2 电压界面
idata bit Voltage_Channel = 0;					   // 电压通道 0 通道1 1 通道3
idata unsigned int AD_Channel1_Light_100x = 0;	   // 通道1光敏电阻分压100倍
idata unsigned int AD_Channel3_RB2_100x = 0;	   // 通道3电位器分压100倍
idata unsigned int AD_Channel3_RB2_Cache_100x = 0; // 通道3电位器分压缓存100倍
idata bit Led_Show_Flag = 1;					   // Led功能是否开启 0 关闭 1 开启
idata unsigned int Time_Tick = 0;				   // 检验按键按下时间
idata bit Long_Press_Detection = 0;				   // 长按检测的标志
/* 按键 */
void Key_Proc()
{
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;
	if (Key_Down == 4)
	{
		Seg_Show_Mode = (++Seg_Show_Mode) % 3;
		Voltage_Channel = 0;
	}
	if (Key_Down == 5)
	{
		if (Seg_Show_Mode == 2)
			Voltage_Channel ^= 1;
	}
	if (Key_Down == 6)
		AD_Channel3_RB2_Cache_100x = AD_Channel3_RB2_100x; // 通道3电压缓存
	if (Key_Down == 7)
		Long_Press_Detection = 1;
	if (Key_Up == 7)
	{
		Long_Press_Detection = 0;
		if (Time_Tick >= 1000)
			Led_Show_Flag ^= 1;
		else
			Freq_Cache = Freq;
	}
}

/* 数码管 */
void Seg_Proc()
{
	switch (Seg_Show_Mode)
	{
	case 0:
		/* 频率显示 */
		Seg_Buf[0] = 11; // F
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = (Freq / 10000 == 0) ? 10 : Freq / 10000;
		Seg_Buf[4] = ((Freq / 1000 % 10 == 0) && (Seg_Buf[3] == 10))
						 ? 10
						 : Freq / 1000 % 10;
		Seg_Buf[5] = ((Freq / 100 % 10 == 0) && (Seg_Buf[4] == 10))
						 ? 10
						 : Freq / 100 % 10;
		Seg_Buf[6] = ((Freq / 10 % 10 == 0) && (Seg_Buf[5] == 10))
						 ? 10
						 : Freq / 10 % 10;
		Seg_Buf[7] = ((Freq % 10 == 0) && (Seg_Buf[6] == 10))
						 ? 10
						 : Freq % 10;
		break;
	case 1:
		/* 周期显示 */
		Seg_Buf[0] = 12; // n
		Seg_Buf[1] = ((Cycle / 1000000 == 0)) ? 10 : Cycle / 1000000;
		Seg_Buf[2] = ((Cycle / 100000 % 10 == 0) && (Seg_Buf[1] == 10))
						 ? 10
						 : Cycle / 100000 % 10;
		Seg_Buf[3] = ((Cycle / 10000 % 10 == 0) && (Seg_Buf[2] == 10))
						 ? 10
						 : Cycle / 10000 % 10;
		Seg_Buf[4] = ((Cycle / 1000 % 10 == 0) && (Seg_Buf[3] == 10))
						 ? 10
						 : Cycle / 1000 % 10;
		Seg_Buf[5] = ((Cycle / 100 % 10 == 0) && (Seg_Buf[4] == 10))
						 ? 10
						 : Cycle / 100 % 10;
		Seg_Buf[6] = ((Cycle / 10 % 10 == 0) && (Seg_Buf[5] == 10))
						 ? 10
						 : Cycle / 10 % 10;
		Seg_Buf[7] = ((Cycle % 10 == 0) && (Seg_Buf[6] == 10))
						 ? 10
						 : Cycle % 10;
		break;
	case 2:
		/* 电压显示 */
		Seg_Buf[0] = 13; // U
		Seg_Buf[1] = 14; //-
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		// 通道1显示
		if (Voltage_Channel == 0)
		{
			Seg_Buf[2] = 1;
			Seg_Buf[5] = AD_Channel1_Light_100x / 100 + ',';
			Seg_Buf[6] = AD_Channel1_Light_100x / 10 % 10;
			Seg_Buf[7] = AD_Channel1_Light_100x % 10;
		}
		// 通道3显示
		else
		{
			Seg_Buf[2] = 3;
			Seg_Buf[5] = AD_Channel3_RB2_100x / 100 + ',';
			Seg_Buf[6] = AD_Channel3_RB2_100x / 10 % 10;
			Seg_Buf[7] = AD_Channel3_RB2_100x % 10;
		}
		break;
	}
}

/* LED */
void Led_Proc()
{
	if (Led_Show_Flag)
	{
		ucLed[0] = (AD_Channel3_RB2_100x > AD_Channel3_RB2_Cache_100x);
		ucLed[1] = (Freq > Freq_Cache);
		ucLed[2] = (Seg_Show_Mode == 0);
		ucLed[3] = (Seg_Show_Mode == 1);
		ucLed[4] = (Seg_Show_Mode == 2);
		Led_Disp(ucLed);
	}
	else
		Led_Off();
}

/* AD */
void Get_AD()
{
	AD_Channel1_Light_100x = Ad_Read(0x03) * 100 / 51;
	AD_Channel3_RB2_100x = Ad_Read(0x01) * 100 / 51;
}
/* 定时器 */
void Timer0Init(void) // 100微秒@12.000MHz
{
	AUXR &= 0x7F; // 定时器时钟12T模式
	TMOD &= 0xF0; // 设置定时器模式
	TMOD |= 0x05; // 设置定时器模式
	TL0 = 0x00;	  // 设置定时初值
	TH0 = 0x00;	  // 设置定时初值
	TF0 = 0;	  // 清除TF0标志
	TR0 = 1;	  // 定时器0开始计时
}

void Timer1Init(void) // 1毫秒@12.000MHz
{
	AUXR &= 0xBF; // 定时器时钟12T模式
	TMOD &= 0x0F; // 设置定时器模式
	TL1 = 0x18;	  // 设置定时初值
	TH1 = 0xFC;	  // 设置定时初值
	TF1 = 0;	  // 清除TF1标志
	TR1 = 1;	  // 定时器1开始计时
	ET1 = 1;
	EA = 1;
}

void Timer1Isr(void) interrupt 3
{
	uwTick++;
	Seg_Pos = (++Seg_Pos) % 8;
	if (++Time_1s == 1000)
	{
		Time_1s = 0;
		Freq = (TH0 << 8) | TL0;
		Cycle = 1000000 / Freq;
		TH0 = TL0 = 0;
	}
	if (Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
	else
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
	// 如果正在进行长按检测
	if (Long_Press_Detection)
	{
		if (++Time_Tick >= 1000)
			Time_Tick = 1001;
	}
	// 没有进行长按检测
	else
		Time_Tick = 0;
}

/* 调度器 */
typedef struct
{
	void (*task_func)(void);   // 任务函数
	unsigned long int rate_ms; // 任务执行周期
	unsigned long int last_ms; // 任务最后一次执行时间
} task_t;

idata task_t Scheduler_Task[] =
	{
		{Led_Proc, 1, 0},
		{Key_Proc, 10, 0},
		{Seg_Proc, 100, 0},
		{Get_AD, 160, 0}};

idata unsigned char task_num;

void Scheduler_Init()
{
	task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}

void Scheduler_Run()
{
	unsigned char i;
	for (i = 0; i < task_num; i++)
	{
		unsigned long int now_time = uwTick;
		if (now_time >= Scheduler_Task[i].rate_ms + Scheduler_Task[i].last_ms)
		{
			Scheduler_Task[i].last_ms = now_time;
			Scheduler_Task[i].task_func();
		}
	}
}

void main()
{
	System_Init();
	Timer0Init();
	Scheduler_Init();
	Timer1Init();
	while (1)
	{
		Scheduler_Run();
	}
}