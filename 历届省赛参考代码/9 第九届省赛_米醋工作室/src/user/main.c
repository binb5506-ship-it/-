#include <STC15F2K60S2.H>
#include "key.h"
#include "led.h"
#include "init.h"
#include "seg.h"
#include "iic.h"

/* 变量声明 */
// 调度器时间
idata unsigned long int uwTick = 0; // 系统计时

// 按键
idata unsigned char Key_Val, Key_Old, Key_Up, Key_Down;

// LED
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// 数码管
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata unsigned char Seg_Pos = 0;

// PWM
idata unsigned char pwm_period = 12;  // 调光的周期
idata unsigned char pwm_compare = 12; // 调光占空比3 6 9 12

// 变量
idata unsigned char Seg_Show_Mode = 0;								// 数码管显示模式 0 待机 1 设置 2 亮度
idata bit Setting_Mode = 0;											// 设置模式 0 编号 1 流转间隔
idata unsigned int Led_Running_Time[4] = {400, 400, 400, 400};		// 各个模式流转时间
idata unsigned int Led_Running_Time_Ctrl[4] = {400, 400, 400, 400}; // 控制设置模式流转时间
idata unsigned char Led_Running_Index = 0;							// 四个模式0~3对应模式1~4
idata unsigned char Led_Running_Now = 0;							// 当前运行模式
idata unsigned char Led_Pos;										// Led计数器
idata unsigned int Time_400Ms = 0;									// 400ms计时器
idata unsigned char EEPROM_Dat[4];									// 数据存储/读取
idata unsigned int Running_Tick = 0;								// 流转时间计数400~1200
idata bit Seg_Light_Flag = 0;										// 数码管闪烁
idata bit System_Flag = 0;											// 系统启动
idata unsigned char Light_Level = 0;								// 亮度等级
idata unsigned char EEPROM_Lock = 5;								// EEPROM校验值
/* 按键控制 */
void Key_Proc()
{
	unsigned char i;
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;
	if (Key_Down == 7)
		System_Flag ^= 1; // 开启/关闭系统
	switch (Seg_Show_Mode)
	{
	case 0:
		/* 待机界面 */
		if (Key_Down == 6)
		{
			Seg_Show_Mode = 1;		// 进入设置界面
			Setting_Mode = 0;		// 保证进入设置界面后会进入运行模式界面
			for (i = 0; i < 4; i++) // 将实际值赋给控制值
				Led_Running_Time_Ctrl[i] = Led_Running_Time[i];
		}
		if (Key_Down == 4)
			Seg_Show_Mode = 2; // 进入亮度界面
		break;
	case 1:
		/* 设置界面 */
		if (Key_Down == 6)
		{
			// 如果当前在流转间隔设置界面
			if (Setting_Mode)
			{
				for (i = 0; i < 4; i++)
				{
					// 将控制值赋给实际值
					Led_Running_Time[i] = Led_Running_Time_Ctrl[i];
					// 将控制值写入EEPROM数组
					EEPROM_Dat[i] = Led_Running_Time_Ctrl[i] / 100;
				}
				EEPROM_Write(EEPROM_Dat, 0, 4);
				EEPROM_Write(&EEPROM_Lock, 8, 1);
				Setting_Mode = 0;
				Seg_Show_Mode = 0;
			}
			// 如果当前在模式编号设置界面
			else
			{
				Setting_Mode = 1;
			}
		}
		if (Key_Down == 5)
		{
			// 如果处于流转间隔模式
			if (Setting_Mode)
				Led_Running_Time_Ctrl[Led_Running_Index] =
					(Led_Running_Time_Ctrl[Led_Running_Index] == 1200) ? 400 : Led_Running_Time_Ctrl[Led_Running_Index] + 100;
			// 如果处于运行模式
			else
				Led_Running_Index = (++Led_Running_Index) % 4; // 保证在0~3中间循环
		}
		if (Key_Down == 4)
		{
			// 如果处于流转间隔模式
			if (Setting_Mode)
				Led_Running_Time_Ctrl[Led_Running_Index] =
					(Led_Running_Time_Ctrl[Led_Running_Index] == 400) ? 1200 : Led_Running_Time_Ctrl[Led_Running_Index] - 100;
			// 如果处于运行模式
			else
				Led_Running_Index = (Led_Running_Index == 0) ? 3 : --Led_Running_Index; // 保证在0~3中间循环
		}
		break;
	case 2:
		/* 亮度界面 */
		if (Key_Up == 4)
			Seg_Show_Mode = 0;
		break;
	}
}
/*数码管控制*/
void Seg_Proc()
{
	switch (Seg_Show_Mode)
	{
	case 0:
		/* 待机界面 */
		Seg_Buf[0] = 10;
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = 10;
		Seg_Buf[6] = 10;
		Seg_Buf[7] = 10;
		break;
	case 1:
		/* 设置界面 */
		// 如果是流转间隔设置
		if (Setting_Mode)
		{
			Seg_Buf[0] = 11; //-
			Seg_Buf[1] = Led_Running_Index + 1;
			Seg_Buf[2] = 11; //-
			Seg_Buf[3] = 10;
			Seg_Buf[4] = (Seg_Light_Flag) ? ((Led_Running_Time_Ctrl[Led_Running_Index] >= 1000) ? Led_Running_Time_Ctrl[Led_Running_Index] / 1000 : 10)
										  : 10;
			Seg_Buf[5] = (Seg_Light_Flag) ? Led_Running_Time_Ctrl[Led_Running_Index] / 100 % 10
										  : 10;
			Seg_Buf[6] = (Seg_Light_Flag) ? 0 : 10;
			Seg_Buf[7] = (Seg_Light_Flag) ? 0 : 10;
		}
		// 如果是运行模式
		else
		{
			Seg_Buf[0] = (Seg_Light_Flag) ? 11 : 10; //-
			Seg_Buf[1] = (Seg_Light_Flag) ? Led_Running_Index + 1 : 10;
			Seg_Buf[2] = (Seg_Light_Flag) ? 11 : 10; //-
			Seg_Buf[3] = 10;
			Seg_Buf[4] = (Led_Running_Time_Ctrl[Led_Running_Index] >= 1000) ? Led_Running_Time_Ctrl[Led_Running_Index] / 1000 : 10;
			Seg_Buf[5] = Led_Running_Time_Ctrl[Led_Running_Index] / 100 % 10;
			Seg_Buf[6] = 0;
			Seg_Buf[7] = 0;
		}
		break;
	case 2:
		/* 亮度界面 */
		Seg_Buf[0] = 10;
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = 10;
		Seg_Buf[6] = 11; //-
		Seg_Buf[7] = Light_Level + 1;
		break;
	}
}
/* LED 控制 */
void Led_Proc()
{
	unsigned char i;
	if (System_Flag)
	{
		switch (Led_Running_Now)
		{
		case 0:
			for (i = 0; i < 8; i++)
				ucLed[i] = (i == Led_Pos);
			break;
		case 1:
			for (i = 0; i < 8; i++)
				ucLed[7 - i] = (i == Led_Pos);
			break;
		case 2:
			for (i = 0; i < 4; i++)
			{
				ucLed[i] = (i == Led_Pos);
				ucLed[7 - i] = (i == Led_Pos);
			}
			break;
		case 3:
			for (i = 0; i < 4; i++)
			{
				ucLed[3 - i] = (i == Led_Pos);
				ucLed[4 + i] = (i == Led_Pos);
			}
			break;
		}
	}
}

/* AD 读取 */
void Get_AD()
{
	unsigned char temp = Ad_Read(0x03);
	Light_Level = temp / 64;			 // 0 1 2 3
	pwm_compare = (3 - Light_Level) * 3; // 9 6 3 0
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
	if (++pwm_period >= 12)
		pwm_period = 0;
	if (pwm_period >= pwm_compare)
		Led_Disp(ucLed);
	else
		Led_Off();
	// 如果系统启动
	if (System_Flag)
	{
		if (++Running_Tick >= Led_Running_Time[Led_Running_Now])
		{
			Running_Tick = 0;
			switch (Led_Running_Now)
			{
			case 0:
			case 1:
				if (++Led_Pos == 8)
				{
					Led_Running_Now++;
					Led_Pos = 0;
				}
				break;
			case 2:
				if (++Led_Pos == 4)
				{
					Led_Running_Now++;
					Led_Pos = 0;
				}
				break;
			case 3:
				if (++Led_Pos == 4)
				{
					Led_Running_Now = 0;
					Led_Pos = 0;
				}
				break;
			}
		}
	}
	else
		Running_Tick = 0;
	// 数码管闪烁
	if (Seg_Show_Mode == 1)
	{
		if (++Time_400Ms == 400)
		{
			Time_400Ms = 0;
			Seg_Light_Flag ^= 1;
		}
	}
	else
	{
		Time_400Ms = 0;
		Seg_Light_Flag = 0;
	}
}
/* 调度器 */
typedef struct
{
	void (*task_func)(void);   // 任务函数
	unsigned long int rate_ms; // 执行一次任务的时间
	unsigned long int last_ms; // 上一次执行任务的时间
} task_t;

idata task_t Scheduler_Task[] = {
	{Led_Proc, 1, 0},
	{Key_Proc, 10, 0},
	{Seg_Proc, 200, 0},
	{Get_AD, 160, 0}};

idata unsigned char task_num; // 任务数量

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
	unsigned char i;
	unsigned char EEPROM_Temp;
	System_Init();
	Scheduler_Init();
	EEPROM_Read(&EEPROM_Temp, 8, 1);
	// 之前写入了数据
	if (EEPROM_Temp == EEPROM_Lock)
	{
		EEPROM_Read(EEPROM_Dat, 0, 4);
		for (i = 0; i < 4; i++)
			Led_Running_Time[i] = EEPROM_Dat[i] * 100;
	}
	Timer1Init();
	while (1)
	{
		Scheduler_Run();
	}
}