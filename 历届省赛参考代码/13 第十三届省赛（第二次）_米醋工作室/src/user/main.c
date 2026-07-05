#include <STC15F2K60S2.H>
#include "key.h"
#include "led.h"
#include "seg.h"
#include "init.h"
#include "iic.h"
#include "ultrasound.h"

/* 变量 */
idata unsigned long int uwTick = 0; // 计时器
// 按键
idata unsigned char Key_Val, Key_Old, Key_Down, Key_Up;
// LED
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// 数码管
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata unsigned char Seg_Pos = 0;

idata unsigned char Seg_Show_Mode = 0;				   // 界面显示 0 电压界面 1 测距界面 2 参数界面
idata bit Setting_Mode = 0;							   // 设置参数上下限 0 电压上限 1 电压下限
idata unsigned int Voltage_RB2_Value_100x = 0;		   // RB2的电压的100倍
idata unsigned char Voltage_RB2_Para_Max_10x = 45;	   // 电压参数上限的10倍
idata unsigned char Voltage_RB2_Para_Min_10x = 5;	   // 电压参数下限的10倍
idata unsigned char Voltage_RB2_Para_Max_Ctrl_10x = 0; // 电压参数上限控制值的10倍
idata unsigned char Voltage_RB2_Para_Min_Ctrl_10x = 0; // 电压参数下限控制值的10倍
idata unsigned char Distance_Value = 0;				   // 测距结果
idata bit Start_Ultrasound = 0;						   // 是否启动测距 0 不启动 1 启动
idata bit Led_Light_Flag = 0;						   // 闪烁标志位
idata unsigned char Time_100Ms = 0;					   // 计时器100ms

/* 按键 */
void Key_Proc()
{
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;
	if (Key_Down == 4)
	{
		// 当前处于测距界面，真实值给控制值
		if (Seg_Show_Mode == 1)
		{
			Voltage_RB2_Para_Max_Ctrl_10x = Voltage_RB2_Para_Max_10x;
			Voltage_RB2_Para_Min_Ctrl_10x = Voltage_RB2_Para_Min_10x;
		}
		// 当前处于参数界面，检验边界，控制值给参数值
		if (Seg_Show_Mode == 2)
		{
			if (Voltage_RB2_Para_Min_Ctrl_10x < Voltage_RB2_Para_Max_Ctrl_10x)
			{
				Voltage_RB2_Para_Max_10x = Voltage_RB2_Para_Max_Ctrl_10x;
				Voltage_RB2_Para_Min_10x = Voltage_RB2_Para_Min_Ctrl_10x;
			}
		}

		Seg_Show_Mode = (++Seg_Show_Mode) % 3;
	}
	// 保证在参数界面
	if (Seg_Show_Mode == 2)
	{
		if (Key_Down == 5)
			Setting_Mode ^= 1;
		if (Key_Down == 6)
		{
			if (Setting_Mode == 0)
				Voltage_RB2_Para_Max_Ctrl_10x = (Voltage_RB2_Para_Max_Ctrl_10x == 50) ? 5 : Voltage_RB2_Para_Max_Ctrl_10x + 5;
			else
				Voltage_RB2_Para_Min_Ctrl_10x = (Voltage_RB2_Para_Min_Ctrl_10x == 50) ? 5 : Voltage_RB2_Para_Min_Ctrl_10x + 5;
		}
		if (Key_Down == 7)
		{
			if (Setting_Mode == 0)
				Voltage_RB2_Para_Max_Ctrl_10x = (Voltage_RB2_Para_Max_Ctrl_10x == 5) ? 50 : Voltage_RB2_Para_Max_Ctrl_10x - 5;
			else
				Voltage_RB2_Para_Min_Ctrl_10x = (Voltage_RB2_Para_Min_Ctrl_10x == 5) ? 50 : Voltage_RB2_Para_Min_Ctrl_10x - 5;
		}
	}
}

/* 数码管 */
void Seg_Proc()
{
	switch (Seg_Show_Mode)
	{
	case 0:
		/* 电压界面 */
		Seg_Buf[0] = 11; // U
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = Voltage_RB2_Value_100x / 100 % 10 + ',';
		Seg_Buf[6] = Voltage_RB2_Value_100x / 10 % 10;
		Seg_Buf[7] = Voltage_RB2_Value_100x % 10;
		break;

	case 1:
		/* 测距界面 */
		Seg_Buf[0] = 13; // L
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		// 如果测距启动
		if (Start_Ultrasound)
		{
			Seg_Buf[5] = (Distance_Value / 100 % 10 == 0) ? 10 : Distance_Value / 100 % 10;
			Seg_Buf[6] = ((Distance_Value / 10 % 10 == 0) && (Seg_Buf[5] == 10)) ? 10 : Distance_Value / 10 % 10;
			Seg_Buf[7] = ((Distance_Value % 10 == 0) && (Seg_Buf[6] == 10)) ? 10 : Distance_Value % 10;
		}
		else
		{
			Seg_Buf[5] = 14; // A
			Seg_Buf[6] = 14; // A
			Seg_Buf[7] = 14; // A
		}
		break;

	case 2:
		/* 参数界面 */
		Seg_Buf[0] = 12; // P
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = Voltage_RB2_Para_Max_Ctrl_10x / 10 + ',';
		Seg_Buf[4] = Voltage_RB2_Para_Max_Ctrl_10x % 10;
		Seg_Buf[5] = 10;
		Seg_Buf[6] = Voltage_RB2_Para_Min_Ctrl_10x / 10 + ',';
		Seg_Buf[7] = Voltage_RB2_Para_Min_Ctrl_10x % 10;
		break;
	}
}

/* LED */
void Led_Proc()
{
	ucLed[0] = (Seg_Show_Mode == 0);
	ucLed[1] = (Seg_Show_Mode == 1);
	ucLed[2] = (Seg_Show_Mode == 2);
	ucLed[7] = Led_Light_Flag;
	Led_Disp(ucLed);
}

/* AD_DA */
void AD_DA()
{
	unsigned char temp_ad = Ad_Read(0x43);
	float temp_da;
	Start_Ultrasound = ((temp_ad * 10 < Voltage_RB2_Para_Max_10x * 51) && (temp_ad * 10 > Voltage_RB2_Para_Min_10x * 51));

	Voltage_RB2_Value_100x = temp_ad * 100 / 51;
	if (Start_Ultrasound)
	{
		if (Distance_Value <= 20)
			Da_Write(1 * 51);
		else if (Distance_Value >= 80)
			Da_Write(5 * 51);
		else
		{
			temp_da = 4 * (float)(Distance_Value - 20) / 60.0 + 1;
			Da_Write(temp_da * 51);
		}
	}
	else
		Da_Write(0);
}

/* 超声波 */
void Get_Distance()
{
	if (Start_Ultrasound)
	{
		Distance_Value = Ut_Wave_Data();
	}
}

/* 定时器 */

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

void Timer1Isr() interrupt 3
{
	uwTick++;
	Seg_Pos = (++Seg_Pos) % 8;
	if (Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
	else
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
	// 如果启动超声波
	if (Start_Ultrasound)
	{
		if (++Time_100Ms == 100)
		{
			Time_100Ms = 0;
			Led_Light_Flag ^= 1;
		}
	}
	else
	{
		Time_100Ms = 0;
		Led_Light_Flag = 0;
	}
}

/* 调度器 */
typedef struct
{
	void (*task_func)(void);   // 任务函数
	unsigned long int rate_ms; // 任务执行周期
	unsigned long int last_ms; // 任务最后一次执行的时间
} task_t;

idata task_t Scheduler_Task[] =
	{
		{Led_Proc, 1, 0},
		{Key_Proc, 10, 0},
		{Seg_Proc, 150, 0},
		{AD_DA, 160, 0},
		{Get_Distance, 200, 0}};

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
		if (now_time >= Scheduler_Task[i].last_ms + Scheduler_Task[i].rate_ms)
		{
			Scheduler_Task[i].last_ms = now_time;
			Scheduler_Task[i].task_func();
		}
	}
}
void main()
{
	System_Init();
	Scheduler_Init();
	Timer1Init();
	while (1)
	{
		Scheduler_Run();
	}
}