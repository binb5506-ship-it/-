#include <STC15F2K60S2.H>
#include "ds1302.h"
#include "key.h"
#include "led.h"
#include "onewire.h"
#include "seg.h"
#include "init.h"
#include "intrins.h"

/* 变量声明 */
// 计时器
idata unsigned long int uwTick = 0; // 调度器计时

// 按键
idata unsigned char Key_Val, Key_Old, Key_Down, Key_Up;

// LED
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// 数码管
idata unsigned char Seg_Pos;
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};

/* 变量 */
idata unsigned char ucRtc[3] = {23, 59, 50};	 // 时钟显示
idata unsigned char ucAlarm[3] = {0, 0, 0};		 // 闹钟
idata unsigned char ucRtc_ctrl[3] = {0, 0, 0};	 // 时钟控制
idata unsigned char ucAlarm_ctrl[3] = {0, 0, 0}; // 闹钟控制
idata unsigned char Seg_Show_Mode = 0;			 // 显示页面 0 时间显示 1 时钟设置 2 闹钟设置 3 温度显示
idata unsigned char Change_Time_Mode = 0;		 // 时钟设置 0 时 1 分 2 秒
idata unsigned char Change_Alarm_Mode = 0;		 // 闹钟设置 0 时 1 分 2 秒
idata unsigned char temperature_value = 0;		 // 温度

idata unsigned int time_500ms = 0;	// 500ms计时
idata unsigned char time_200ms = 0; // 200ms计时
idata unsigned int time_5s = 0;		// 5s计时
idata bit Setting_Mode = 0;			// 0 未设置 1 设置
idata bit Alarming = 0;				// 0 闹钟未触发 1 闹钟触发
idata bit Light_Seg_Mode = 0;		// 闪烁标志位数码管
idata bit Light_Led_Mode = 0;		// 闪烁标志位LED
/* 按键 */
void Key_Proc()
{
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;
	switch (Seg_Show_Mode)
	{
	case 0:
		// 时间显示页面
		switch (Key_Down)
		{
		case 7:
			// 时钟设置
			Seg_Show_Mode = 1;
			ucRtc_ctrl[0] = ucRtc[0];
			ucRtc_ctrl[1] = ucRtc[1];
			ucRtc_ctrl[2] = ucRtc[2];
			break;
		case 6:
			// 闹钟设置
			Seg_Show_Mode = 2;
			ucAlarm_ctrl[0] = ucAlarm[0];
			ucAlarm_ctrl[1] = ucAlarm[1];
			ucAlarm_ctrl[2] = ucAlarm[2];
			break;
		case 4:
			// 温度界面
			Seg_Show_Mode = 3;
			break;
		}
		break;
	case 1:
		// 时钟设置页面
		switch (Key_Down)
		{
		case 7:
			if (Change_Time_Mode == 2)
			{
				Seg_Show_Mode = 0;
				Change_Time_Mode = 0;
				Set_Rtc(ucRtc_ctrl); // 将设定的数据写入芯片
			}
			else
				Change_Time_Mode++;
			break;
		case 5:
			if (Change_Time_Mode == 0)
				// 设置小时
				ucRtc_ctrl[Change_Time_Mode] = (++ucRtc_ctrl[Change_Time_Mode]) % 24; // 0~23循环
			else
				// 设置分秒
				ucRtc_ctrl[Change_Time_Mode] = (++ucRtc_ctrl[Change_Time_Mode]) % 60; // 0~59循环
			break;
		case 4:
			if (Change_Time_Mode == 0)
				// 设置小时
				ucRtc_ctrl[Change_Time_Mode] = (ucRtc_ctrl[Change_Time_Mode] == 0) ? 23 : --ucRtc_ctrl[Change_Time_Mode];
			else
				// 设置分秒
				ucRtc_ctrl[Change_Time_Mode] = (ucRtc_ctrl[Change_Time_Mode] == 0) ? 59 : --ucRtc_ctrl[Change_Time_Mode];
			break;
		}
		break;
	case 2:
		// 闹钟设置页面
		switch (Key_Down)
		{
		case 6:
			if (Change_Alarm_Mode == 2)
			{
				Seg_Show_Mode = 0;
				Change_Alarm_Mode = 0;
				ucAlarm[0] = ucAlarm_ctrl[0];
				ucAlarm[1] = ucAlarm_ctrl[1];
				ucAlarm[2] = ucAlarm_ctrl[2];
			}
			else
				Change_Alarm_Mode++;
			break;
		case 5:
			if (Change_Alarm_Mode == 0)
				// 设置小时
				ucAlarm_ctrl[Change_Alarm_Mode] = (++ucAlarm_ctrl[Change_Alarm_Mode]) % 24; // 0~23循环
			else
				// 设置分秒
				ucAlarm_ctrl[Change_Alarm_Mode] = (++ucAlarm_ctrl[Change_Alarm_Mode]) % 60; // 0~59循环
			break;
		case 4:
			if (Change_Alarm_Mode == 0)
				// 设置小时
				ucAlarm_ctrl[Change_Alarm_Mode] = (ucAlarm_ctrl[Change_Alarm_Mode] == 0) ? 23 : --ucAlarm_ctrl[Change_Alarm_Mode];
			else
				// 设置分秒
				ucAlarm_ctrl[Change_Alarm_Mode] = (ucAlarm_ctrl[Change_Alarm_Mode] == 0) ? 59 : --ucAlarm_ctrl[Change_Alarm_Mode];
			break;
		}
		break;
	case 3:
		// 温度显示页面
		if (Key_Up == 4)
			Seg_Show_Mode = 0;
		break;
	}
	if (Alarming)
	{
		// 如果闹钟响了
		if (Key_Down != 0)
			Alarming = 0;
	}
}
/* 数码管处理 */
void Seg_Proc()
{
	switch (Seg_Show_Mode)
	{
	case 0:
		// 时间显示
		Seg_Buf[0] = ucRtc[0] / 10;
		Seg_Buf[1] = ucRtc[0] % 10;
		Seg_Buf[2] = 11; //-
		Seg_Buf[3] = ucRtc[1] / 10;
		Seg_Buf[4] = ucRtc[1] % 10;
		Seg_Buf[5] = 11; //-
		Seg_Buf[6] = ucRtc[2] / 10;
		Seg_Buf[7] = ucRtc[2] % 10;
		break;
	case 1:
		// 时间设置
		if (Change_Time_Mode == 0)
		{
			Seg_Buf[0] = (Light_Seg_Mode) ? ucRtc_ctrl[0] / 10 : 10;
			Seg_Buf[1] = (Light_Seg_Mode) ? ucRtc_ctrl[0] % 10 : 10;
			Seg_Buf[2] = 11; //-
			Seg_Buf[3] = ucRtc_ctrl[1] / 10;
			Seg_Buf[4] = ucRtc_ctrl[1] % 10;
			Seg_Buf[5] = 11; //-
			Seg_Buf[6] = ucRtc_ctrl[2] / 10;
			Seg_Buf[7] = ucRtc_ctrl[2] % 10;
		}
		else if (Change_Time_Mode == 1)
		{
			Seg_Buf[0] = ucRtc_ctrl[0] / 10;
			Seg_Buf[1] = ucRtc_ctrl[0] % 10;
			Seg_Buf[2] = 11; //-
			Seg_Buf[3] = (Light_Seg_Mode) ? ucRtc_ctrl[1] / 10 : 10;
			Seg_Buf[4] = (Light_Seg_Mode) ? ucRtc_ctrl[1] % 10 : 10;
			Seg_Buf[5] = 11; //-
			Seg_Buf[6] = ucRtc_ctrl[2] / 10;
			Seg_Buf[7] = ucRtc_ctrl[2] % 10;
		}
		else if (Change_Time_Mode == 2)
		{
			Seg_Buf[0] = ucRtc_ctrl[0] / 10;
			Seg_Buf[1] = ucRtc_ctrl[0] % 10;
			Seg_Buf[2] = 11; //-
			Seg_Buf[3] = ucRtc_ctrl[1] / 10;
			Seg_Buf[4] = ucRtc_ctrl[1] % 10;
			Seg_Buf[5] = 11; //-
			Seg_Buf[6] = (Light_Seg_Mode) ? ucRtc_ctrl[2] / 10 : 10;
			Seg_Buf[7] = (Light_Seg_Mode) ? ucRtc_ctrl[2] % 10 : 10;
		}
		break;
	case 2:
		// 闹钟设置
		if (Change_Alarm_Mode == 0)
		{
			Seg_Buf[0] = (Light_Seg_Mode) ? ucAlarm_ctrl[0] / 10 : 10;
			Seg_Buf[1] = (Light_Seg_Mode) ? ucAlarm_ctrl[0] % 10 : 10;
			Seg_Buf[2] = 11; //-
			Seg_Buf[3] = ucAlarm_ctrl[1] / 10;
			Seg_Buf[4] = ucAlarm_ctrl[1] % 10;
			Seg_Buf[5] = 11; //-
			Seg_Buf[6] = ucAlarm_ctrl[2] / 10;
			Seg_Buf[7] = ucAlarm_ctrl[2] % 10;
		}
		else if (Change_Alarm_Mode == 1)
		{
			Seg_Buf[0] = ucAlarm_ctrl[0] / 10;
			Seg_Buf[1] = ucAlarm_ctrl[0] % 10;
			Seg_Buf[2] = 11; //-
			Seg_Buf[3] = (Light_Seg_Mode) ? ucAlarm_ctrl[1] / 10 : 10;
			Seg_Buf[4] = (Light_Seg_Mode) ? ucAlarm_ctrl[1] % 10 : 10;
			Seg_Buf[5] = 11; //-
			Seg_Buf[6] = ucAlarm_ctrl[2] / 10;
			Seg_Buf[7] = ucAlarm_ctrl[2] % 10;
		}
		else if (Change_Alarm_Mode == 2)
		{
			Seg_Buf[0] = ucAlarm_ctrl[0] / 10;
			Seg_Buf[1] = ucAlarm_ctrl[0] % 10;
			Seg_Buf[2] = 11; //-
			Seg_Buf[3] = ucAlarm_ctrl[1] / 10;
			Seg_Buf[4] = ucAlarm_ctrl[1] % 10;
			Seg_Buf[5] = 11; //-
			Seg_Buf[6] = (Light_Seg_Mode) ? ucAlarm_ctrl[2] / 10 : 10;
			Seg_Buf[7] = (Light_Seg_Mode) ? ucAlarm_ctrl[2] % 10 : 10;
		}
		break;
	case 3:
		// 温度显示
		Seg_Buf[0] = 10;
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = temperature_value / 10 % 10;
		Seg_Buf[6] = temperature_value % 10;
		Seg_Buf[7] = 12; // C
		break;
	}
}
/* LED处理 */
void Led_Proc()
{
	Setting_Mode = (Seg_Show_Mode == 1) || (Seg_Show_Mode == 2); // 设置时间的时候 这个mode会为1，否则就是0
	ucLed[0] = Light_Led_Mode;
	Led_Disp(ucLed);
}
/* 温度获取 */
void Get_Temputure()
{
	temperature_value = rd_temperature();
}
/* 时间获取 */
void Get_Time()
{
	Read_Rtc(ucRtc);
	if ((ucRtc[0] == ucAlarm[0]) &&
		(ucRtc[1] == ucAlarm[1]) &&
		(ucRtc[2] == ucAlarm[2]))
		// 闹钟触发
		Alarming = 1;
}
/* 初始化定时器 */
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
/* 定时器中断 */
void Timer1Isr(void) interrupt 3
{
	uwTick++;				   // 系统计时变量增加
	Seg_Pos = (++Seg_Pos) % 8; // 更新数码管位置
	if (Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
	else
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);

	if (Alarming)
	{
		// 闹钟触发
		if (++time_200ms == 200)
		{
			Light_Led_Mode ^= 1;
			time_200ms = 0;
		}
		if (++time_5s == 5000)
		{
			Light_Led_Mode = 0;
			time_200ms = 0;
			time_5s = 0;
			Alarming = 0;
		}
	}
	else
	{
		Light_Led_Mode = 0;
		time_200ms = 0;
		time_5s = 0;
	}
	if (Setting_Mode)
	{
		// 处于设置界面
		if (++time_500ms == 500)
		{
			Light_Seg_Mode ^= 1;
			time_500ms = 0;
		}
	}
	else
	{
		// 不处于设置界面
		time_500ms = 0;
		Light_Seg_Mode = 0;
	}
}
/* 调度器 */
typedef struct
{
	void (*task_func)(void);   // 任务函数
	unsigned long int rate_ms; // 任务执行的周期（毫秒）
	unsigned long int last_ms; // 任务上一次执行的时间
} task_t;

idata task_t Scheduler_Task[] = {
	{Led_Proc, 1, 0},
	{Key_Proc, 10, 0},
	{Get_Time, 200, 0},
	{Get_Temputure, 300, 0},
	{Seg_Proc, 300, 0}};

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
		unsigned long int now_time = uwTick; // 暂存当前时间
		if (now_time >= Scheduler_Task[i].last_ms + Scheduler_Task[i].rate_ms)
		{
			Scheduler_Task[i].last_ms = now_time; // 时间更新
			Scheduler_Task[i].task_func();		  // 任务运行
		}
	}
}
void Delay750ms() //@12.000MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 35;
	j = 51;
	k = 182;
	do
	{
		do
		{
			while (--k)
				;
		} while (--j);
	} while (--i);
}

void main()
{
	System_Init();
	rd_temperature();
	Delay750ms();
	Set_Rtc(ucRtc);
	Scheduler_Init();
	Timer1Init();
	while (1)
	{
		Scheduler_Run();
	}
}