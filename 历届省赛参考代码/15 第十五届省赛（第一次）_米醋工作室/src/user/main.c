#include <STC15F2K60S2.H>
#include "key.h"
#include "led.h"
#include "seg.h"
#include "init.h"
#include "ds1302.h"
#include "iic.h"

/* 变量 */
idata unsigned long int uwTick = 0; // 定时器
idata unsigned int Time_1s = 0;		// 频率测量
// 按键
idata unsigned char Key_Val, Key_Old, Key_Down, Key_Up;
// LED
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// 数码管
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata unsigned char Seg_Pos = 0;
// 频率
idata unsigned int Freq_Value_Real = 0; // 直接测量得到的频率
idata unsigned int Freq_Value_Cal = 0;	// 频率计算值
idata unsigned int Freq_Max = 0;		// 最大频率
// 时间
idata unsigned char ucRtc[3] = {0, 0, 0};
idata unsigned char Max_Rtc[3] = {0, 0, 0};
idata unsigned char Time_200Ms_Freq_Show = 0;  // 频率界面闪烁计时
idata unsigned char Time_200Ms_Freq_Wring = 0; // 频率超过超限参数计时

// 显示
idata unsigned char Seg_Show_Mode = 0; // 显示界面 0 频率 1 参数 2 时间 3回显
idata bit Para_Mode = 0;			   // 0 超限 1 校准
idata bit Echo_Mode = 0;			   // 0 频率 1 时间

// 数据
idata unsigned int Limit_Para = 2000;	// 超限参数 1000~9000
idata int Calibration_Para = 0;			// 校准参数 -900~900
idata unsigned int Limit_Para_Ctrl = 0; // 超限参数控制值  1000~9000
idata int Calibration_Para_Ctrl = 0;	// 校准参数控制值 -900~900

// 判断
idata bit Freq_Error = 0;	  // 频率测量错误
idata bit Led_Freq_Show = 0;  // 处于频率测量界面闪烁LED
idata bit Led_Freq_Wring = 0; // 频率超过超限参数闪烁LED
idata bit Freq_Wring = 0;	  // 频率超过超限参数

/* 按键 */
void Key_Proc()
{
	Key_Val = Key_Read();
	Key_Down = Key_Val ^ (Key_Val & Key_Old);
	Key_Up = ~Key_Val ^ (Key_Val & Key_Old);
	Key_Old = Key_Val;
	if (Key_Down == 4)
	{
		// 如果处于频率界面 参数给控制 保证界面超限参数
		if (Seg_Show_Mode == 0)
		{
			Calibration_Para_Ctrl = Calibration_Para;
			Limit_Para_Ctrl = Limit_Para;
			Para_Mode = 0;
		}
		// 如果处于参数界面 控制给参数
		if (Seg_Show_Mode == 1)
		{
			Calibration_Para = Calibration_Para_Ctrl;
			Limit_Para = Limit_Para_Ctrl;
		}
		// 如果处于时间界面 保证界面频率回显
		if (Seg_Show_Mode == 2)
			Echo_Mode = 0;
		Seg_Show_Mode = (++Seg_Show_Mode) % 4;
	}
	switch (Seg_Show_Mode)
	{
	case 1:
		/* 参数界面 */
		if (Key_Down == 5)
			Para_Mode ^= 1;
		// 如果是超限参数设置
		if (Para_Mode == 0)
		{
			if (Key_Down == 8)
				Limit_Para_Ctrl = (Limit_Para_Ctrl == 9000)
									  ? 1000
									  : Limit_Para_Ctrl + 1000;
			if (Key_Down == 9)
				Limit_Para_Ctrl = (Limit_Para_Ctrl == 1000)
									  ? 9000
									  : Limit_Para_Ctrl - 1000;
		}
		// 如果是校准值设置
		else
		{
			if (Key_Down == 8)
				Calibration_Para_Ctrl = (Calibration_Para_Ctrl == 900)
											? -900
											: Calibration_Para_Ctrl + 100;
			if (Key_Down == 9)
				Calibration_Para_Ctrl = (Calibration_Para_Ctrl == -900)
											? 900
											: Calibration_Para_Ctrl - 100;
		}
		break;
	case 3:
		/* 回显界面 */
		if (Key_Down == 5)
			Echo_Mode ^= 1;
		break;
	}
}

/* 数码管 */
void Seg_Proc()
{
	if (Freq_Value_Cal > Freq_Max)
	{
		Freq_Max = Freq_Value_Cal;
		Read_Rtc(Max_Rtc);
	}
	switch (Seg_Show_Mode)
	{
	case 0:
		/* 频率界面 */
		Seg_Buf[0] = 11; // F
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		// 如果频率测量错误
		if (Freq_Error)
		{
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 16; // L
			Seg_Buf[7] = 16; // L
		}
		else
		{
			Seg_Buf[3] = (Freq_Value_Cal / 10000 % 10 == 0)
							 ? 10
							 : Freq_Value_Cal / 10000 % 10;
			Seg_Buf[4] = ((Freq_Value_Cal / 1000 % 10 == 0) && (Seg_Buf[3] == 10))
							 ? 10
							 : Freq_Value_Cal / 1000 % 10;
			Seg_Buf[5] = ((Freq_Value_Cal / 100 % 10 == 0) && (Seg_Buf[4] == 10))
							 ? 10
							 : Freq_Value_Cal / 100 % 10;
			Seg_Buf[6] = ((Freq_Value_Cal / 10 % 10 == 0) && (Seg_Buf[5] == 10))
							 ? 10
							 : Freq_Value_Cal / 10 % 10;
			Seg_Buf[7] = ((Freq_Value_Cal % 10 == 0) && (Seg_Buf[6] == 10))
							 ? 10
							 : Freq_Value_Cal % 10;
		}

		break;

	case 1:
		/* 参数界面 */
		Seg_Buf[0] = 12; // P
		Seg_Buf[1] = (Para_Mode) ? 2 : 1;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		// 处于超限参数界面
		if (Para_Mode == 0)
		{
			Seg_Buf[4] = Limit_Para_Ctrl / 1000;
			Seg_Buf[5] = 0;
			Seg_Buf[6] = 0;
			Seg_Buf[7] = 0;
		}
		// 处于校准值界面
		else
		{
			if (Calibration_Para_Ctrl < 0)
			{
				Seg_Buf[4] = 13; //-
				Seg_Buf[5] = (-Calibration_Para_Ctrl) / 100;
				Seg_Buf[6] = 0;
				Seg_Buf[7] = 0;
			}
			else if (Calibration_Para_Ctrl == 0)
			{
				Seg_Buf[4] = 10;
				Seg_Buf[5] = 10;
				Seg_Buf[6] = 10;
				Seg_Buf[7] = 0;
			}
			else
			{
				Seg_Buf[4] = 10;
				Seg_Buf[5] = Calibration_Para_Ctrl / 100;
				Seg_Buf[6] = 0;
				Seg_Buf[7] = 0;
			}
		}
		break;

	case 2:
		/* 时间界面 */
		Seg_Buf[0] = ucRtc[0] / 10;
		Seg_Buf[1] = ucRtc[0] % 10;
		Seg_Buf[2] = 13; //-
		Seg_Buf[3] = ucRtc[1] / 10;
		Seg_Buf[4] = ucRtc[1] % 10;
		Seg_Buf[5] = 13; //-
		Seg_Buf[6] = ucRtc[2] / 10;
		Seg_Buf[7] = ucRtc[2] % 10;

		break;

	case 3:
		/* 回显界面 */
		Seg_Buf[0] = 14;					// H
		Seg_Buf[1] = (Echo_Mode) ? 15 : 11; // A F
		// 频率回显
		if (Echo_Mode == 0)
		{
			Seg_Buf[2] = 10;
			Seg_Buf[3] = (Freq_Max / 10000 % 10 == 0)
							 ? 10
							 : Freq_Max / 10000 % 10;
			Seg_Buf[4] = ((Freq_Max / 1000 % 10 == 0) && (Seg_Buf[3] == 10))
							 ? 10
							 : Freq_Max / 1000 % 10;
			Seg_Buf[5] = ((Freq_Max / 100 % 10 == 0) && (Seg_Buf[4] == 10))
							 ? 10
							 : Freq_Max / 100 % 10;
			Seg_Buf[6] = ((Freq_Max / 10 % 10 == 0) && (Seg_Buf[5] == 10))
							 ? 10
							 : Freq_Max / 10 % 10;
			Seg_Buf[7] = ((Freq_Max % 10 == 0) && (Seg_Buf[6] == 10))
							 ? 10
							 : Freq_Max % 10;
		}
		// 时间回显
		else
		{
			Seg_Buf[2] = Max_Rtc[0] / 10;
			Seg_Buf[3] = Max_Rtc[0] % 10;
			Seg_Buf[4] = Max_Rtc[1] / 10;
			Seg_Buf[5] = Max_Rtc[1] % 10;
			Seg_Buf[6] = Max_Rtc[2] / 10;
			Seg_Buf[7] = Max_Rtc[2] % 10;
		}

		break;
	}
}

/* LED */
void Led_Proc()
{
	ucLed[0] = Led_Freq_Show;
	ucLed[1] = (Freq_Error) ? 1 : Led_Freq_Wring;
	Led_Disp(ucLed);
}

/* AD_DA */
void AD_DA()
{
	float temp_da;
	if (Freq_Error)
		Da_Write(0 * 51);
	else
	{
		if (Freq_Value_Cal < 500)
			Da_Write(1 * 51);
		else if (Freq_Wring)
			Da_Write(5 * 51);
		else if ((Freq_Value_Cal > 500) && (Freq_Wring == 0))
		{
			temp_da = (4 * (float)(Freq_Value_Cal - 500)) / (float)(Limit_Para - 500) + 1;
			Da_Write(temp_da * 51);
		}
	}
}

/* 时间 */
void Get_Time()
{
	Read_Rtc(ucRtc);
}

/* 定时器 */
void Timer0Init(void) //@12.000MHz
{
	AUXR &= 0x7F; // 定时器时钟12T模式
	TMOD &= 0xF0; // 设置定时器模式
	TMOD |= 0x05; // 计数，且不自动重装
	TL0 = 0;	  // 设置定时初值
	TH0 = 0;	  // 设置定时初值
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

void Timer1Isr() interrupt 3
{
	unsigned int Para_Temp = -Calibration_Para;
	uwTick++;
	Seg_Pos = (++Seg_Pos) % 8;
	if (Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
	else
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
	if (++Time_1s == 1000)
	{
		Time_1s = 0;
		Freq_Value_Real = TH0 << 8 | TL0;
		// 校准为负数
		if (Calibration_Para < 0)
		{
			// 校准绝对值大于测频
			if ((Para_Temp > Freq_Value_Real))
			{
				Freq_Error = 1;
				Freq_Value_Cal = 0;
				Freq_Wring = 0;
			}
			else
			{
				Freq_Error = 0;
				Freq_Value_Cal = Freq_Value_Real - Para_Temp;
				Freq_Wring = (Freq_Value_Cal > Limit_Para);
			}
		}
		else
		{
			Freq_Error = 0;
			Freq_Value_Cal = Freq_Value_Real + Calibration_Para;
			Freq_Wring = (Freq_Value_Cal > Limit_Para);
		}
		TH0 = TL0 = 0;
	}
	if (Seg_Show_Mode == 0)
	{
		if (++Time_200Ms_Freq_Show == 200)
		{
			Time_200Ms_Freq_Show = 0;
			Led_Freq_Show ^= 1;
		}
	}
	else
	{
		Time_200Ms_Freq_Show = 0;
		Led_Freq_Show = 0;
	}
	if (Freq_Wring)
	{
		if (++Time_200Ms_Freq_Wring == 200)
		{
			Time_200Ms_Freq_Wring = 0;
			Led_Freq_Wring ^= 1;
		}
	}
	else
	{
		Time_200Ms_Freq_Wring = 0;
		Led_Freq_Wring = 0;
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
		{Seg_Proc, 80, 0},
		{AD_DA, 160, 0},
		{Get_Time, 100, 0}};

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
	Set_Rtc(ucRtc);
	Timer0Init();
	Scheduler_Init();
	Timer1Init();
	while (1)
	{
		Scheduler_Run();
	}
}