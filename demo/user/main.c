// 包含 STC15F2K60S2 单片机的头文件，提供了寄存器定义等
#include <STC15F2K60S2.H>

// 包含自定义的初始化、外设驱动和功能模块的头文件
#include "init.h" // 系统初始化
#include "led.h"  // LED 模块
#include "seg.h"  // 数码管模块
#include "key.h"  // 按键模块

#include "ds1302.h"		// DS1302 实时时钟模块
#include "onewire.h"	// 单总线协议，用于温度传感器
#include "iic.h"		// I2C 协议，用于 ADC/DAC 和 EEPROM
#include "ultrasound.h" // 超声波模块
#include "uart.h"		// 串口通信模块

// 包含标准库头文件
#include "string.h" // 字符串处理函数
#include "stdio.h"	// 标准输入输出函数，如 printf 和 sscanf

// 全局变量定义

// `idata` 存储在内部高速 RAM 中，访问速度快
// 系统滴答计时器，由 Timer1 中断每毫秒增加一次
idata unsigned long int uwTick;

// `pdata` 存储在外部 RAM 的分页区域，访问速度较快
// LED灯的状态数组，对应8个LED的亮灭
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// 数码管显示缓冲区，存储要在8位数码管上显示的数字或符号
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10}; // 10代表不显示
// 当前正在刷新的数码管位置索引 (0-7)
idata unsigned char Seg_Pos = 0;

// 按键状态变量
idata unsigned char Key_Val;  // 当前按键值
idata unsigned char Key_Old;  // 上一次的按键值，用于检测边沿
idata unsigned char Key_Up;	  // 按键抬起事件标志
idata unsigned char Key_Down; // 按键按下事件标志

// `pdata` 存储在外部 RAM 的分页区域
// RTC实时时钟数据数组 [时, 分, 秒]
pdata unsigned char ucRtc[3] = {11, 12, 13};

// `idata` 存储在内部高速 RAM 中
// 存储放大10倍的温度值，以处理一位小数
idata unsigned int Temperature_10x;

// AD转换通道1和通道3的数据，同样乘以100以处理两位小数
idata unsigned int AD_1_Data_100x, AD_3_Data_100x;

// `pdata` 存储在外部 RAM 的分页区域
// 准备写入EEPROM的数据
pdata unsigned char EEPROM_Data_W[8] = {1, 2, 3, 4, 5, 6, 7, 8};
// 从EEPROM读取数据的缓冲区
pdata unsigned char EEPROM_Data_R[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// `idata` 存储在内部高速 RAM 中
// 存储超声波测量的距离
idata unsigned char Distance;

// 测量的频率值
idata unsigned int Freq;
// 1秒的计时器，用于频率测量
idata unsigned int Time_1s;

// PWM（脉冲宽度调制）相关变量
idata unsigned char pwm_period;		 // PWM周期内的累加计数器
idata unsigned char pwm_compare = 6; // PWM比较值，决定占空比，控制LED亮度 (0-9)

// 串口接收相关变量
idata unsigned char Uart_Rx_Index;									  // 串口接收缓冲区索引
pdata unsigned char Uart_Rx_Buf[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // 串口接收缓冲区
idata unsigned char Uart_Rx_Flag;									  // 串口接收到数据的标志
idata unsigned char Uart_Rx_Tick;									  // 串口接收超时计时器

// 数码管显示模式
// 0:时间, 1:温度, 2:AD值, 3:超声波距离, 4:频率, 5:PWM参数
idata unsigned char Seg_Show_Mode;

/**
 * @brief 按键处理函数
 * 检测按键的按下和抬起事件，并根据按键执行相应操作。
 */
void Key_Proc()
{
	Key_Val = Key_Read(); // 读取当前按key的状态
	// 使用异或运算检测按键状态变化，& Key_Val 检测下降沿（按下）
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	// 使用异或运算检测按键状态变化，& ~Key_Val 检测上降沿（抬起）
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val; // 更新旧的按键值

	// 如果S4键被按下
	if (Key_Down == 4)
		// PWM比较值加1，并限制在0-9之间，用于调整LED亮度
		pwm_compare = (++pwm_compare) % 10;

	// 如果有任何按键被按下，通过串口打印按键值
	if (Key_Down != 0)
		printf("Key_Down: %bu", Key_Down);

	// 如果S5键被按下
	if (Key_Down == 5)
		// 切换数码管的显示模式，在6个模式之间循环
		Seg_Show_Mode = (++Seg_Show_Mode) % 6;
}

/**
 * @brief 数码管显示处理函数
 * 根据当前的 Seg_Show_Mode，准备要显示在数码管上的数据。
 */
void Seg_Proc()
{
	// 根据显示模式来填充数码管显示缓冲区 Seg_Buf
	switch (Seg_Show_Mode)
	{
	case 0: // 时间界面
		// 格式: HH-MM-SS
		Seg_Buf[0] = ucRtc[0] / 10; // 时 - 十位
		Seg_Buf[1] = ucRtc[0] % 10; // 时 - 个位
		Seg_Buf[2] = 0 + ',';		// 显示'-' (通过特殊处理实现)
		Seg_Buf[3] = ucRtc[1] / 10; // 分 - 十位
		Seg_Buf[4] = ucRtc[1] % 10; // 分 - 个位
		Seg_Buf[5] = 0 + ',';		// 显示'-'
		Seg_Buf[6] = ucRtc[2] / 10; // 秒 - 十位
		Seg_Buf[7] = ucRtc[2] % 10; // 秒 - 个位
		break;

	case 1: // 温度界面
		// 格式: XX.X
		Seg_Buf[0] = Temperature_10x / 100;			  // 温度 - 十位
		Seg_Buf[1] = Temperature_10x / 10 % 10 + ','; // 温度 - 个位，并附加小数点
		Seg_Buf[2] = Temperature_10x % 10;			  // 温度 - 小数位
		// 熄灭其余的数码管
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = 10;
		Seg_Buf[6] = 10;
		Seg_Buf[7] = 10;
		break;

	case 2: // AD转换界面
		// 格式: X.XX (通道1)   X.XX (通道3)
		Seg_Buf[0] = AD_1_Data_100x / 100 % 10 + ','; // 通道1(个位)并附加小数点
		Seg_Buf[1] = AD_1_Data_100x / 10 % 10;		  // 通道1(十分位)
		Seg_Buf[2] = AD_1_Data_100x % 10;			  // 通道1(百分位)
		Seg_Buf[3] = 10;							  // 熄灭
		Seg_Buf[4] = 10;							  // 熄灭
		Seg_Buf[5] = AD_3_Data_100x / 100 % 10 + ','; // 通道3(个位)并附加小数点
		Seg_Buf[6] = AD_3_Data_100x / 10 % 10;		  // 通道3(十分位)
		Seg_Buf[7] = AD_3_Data_100x % 10;			  // 通道3(百分位)
		break;

	case 3: // 超声波界面
		// 格式: XXX (距离值)
		Seg_Buf[0] = Distance / 100;	 // 距离 - 百位
		Seg_Buf[1] = Distance / 10 % 10; // 距离 - 十位
		Seg_Buf[2] = Distance % 10;		 // 距离 - 个位
		// 熄灭其余的数码管
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = 10;
		Seg_Buf[6] = 10;
		Seg_Buf[7] = 10;
		break;

	case 4: // 频率界面
		// 显示8位频率值
		Seg_Buf[0] = (Freq > 10000000) ? Freq / 10000000 % 10 : 10;
		Seg_Buf[1] = (Freq > 1000000) ? Freq / 1000000 % 10 : 10;
		Seg_Buf[2] = (Freq > 100000) ? Freq / 100000 % 10 : 10;
		Seg_Buf[3] = (Freq > 10000) ? Freq / 10000 % 10 : 10;
		Seg_Buf[4] = (Freq > 1000) ? Freq / 1000 % 10 : 10;
		Seg_Buf[5] = (Freq > 100) ? Freq / 100 % 10 : 10;
		Seg_Buf[6] = (Freq > 10) ? Freq / 10 % 10 : 10;
		Seg_Buf[7] = Freq % 10;
		break;

	case 5: // PWM界面
		// 显示当前的PWM比较值 (亮度等级)
		Seg_Buf[0] = pwm_compare;
		// 熄灭其余的数码管
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = 10;
		Seg_Buf[6] = 10;
		Seg_Buf[7] = 10;
		break;
	}
}

/**
 * @brief LED处理函数
 * 设置LED的状态，这里是固定的交替亮灭模式。
 */
void Led_Proc()
{
	ucLed[0] = 1; // 亮
	ucLed[1] = 0; // 灭
	ucLed[2] = 1;
	ucLed[3] = 0;
	ucLed[4] = 1;
	ucLed[5] = 0;
	ucLed[6] = 1;
	ucLed[7] = 0;
	// Led_Disp(ucLed); // 实际的显示操作在Timer1中断中根据PWM进行
}

/**
 * @brief 获取DS1302实时时钟的时间
 */
void Get_Time()
{
	Read_Rtc(ucRtc); // 调用DS1302驱动函数读取时间
}

/**
 * @brief 获取DS18B20温度
 */
void Get_Temperature()
{
	// 调用单总线驱动函数读取温度，并放大10倍
	Temperature_10x = rd_temperature() * 10;
}

/**
 * @brief ADC/DAC处理函数
 * 读取光敏电阻和电位器的AD值，并设置DAC输出。
 */
void AD_DA()
{
	// 读取AIN1通道(光敏电阻)的AD值，并进行转换
	// 0x41是PCF8591 AIN1通道的控制字节
	// 原始值0-255，转换为0-5.0V，结果乘以100得到0-500
	AD_1_Data_100x = (float)Ad_Read(0x41) / 51.0f * 100;
	// 读取AIN3通道(电位器)的AD值，同上
	AD_3_Data_100x = (float)Ad_Read(0x43) / 51.0f * 100;
	// 设置DAC输出一个固定的电压值 (3V)，3*51
	Da_Write(3 * 51);
}

/**
 * @brief 获取超声波测量的距离
 */
void Get_Distance()
{
	Distance = Ut_Wave_Data(); // 调用超声波驱动函数
}

/**
 * @brief 串口数据处理函数
 * 处理接收到的串口数据，实现超时判断和数据解析。
 */
void Uart_Proc()
{
	unsigned char x, y;
	// 如果没有接收到数据，直接返回
	if (Uart_Rx_Index == 0)
		return;

	// 如果距离上次接收到数据超过10ms（超时）
	if (Uart_Rx_Tick >= 10)
	{
		Uart_Rx_Flag = 0; // 清除接收标志
		Uart_Rx_Tick = 0; // 复位超时计时器

		printf("%s", Uart_Rx_Buf); // 将收到的数据回显到串口，用于调试

		// 尝试按"(x,y)"的格式解析数据
		if (sscanf(Uart_Rx_Buf, "(%bu,%bu)", &x, &y) == 2)
			// 解析成功，打印解析出的x和y
			printf("\r\nI Get x=%bu,y=%bu\r\n", x, y);
		else
			// 解析失败，打印错误信息
			printf("\r\nERROR\r\n");

		// 清空接收缓冲区，为下次接收做准备
		memset(Uart_Rx_Buf, 0, Uart_Rx_Index);
		Uart_Rx_Index = 0;
	}
}

/**
 * @brief 定时器0初始化函数
 * 配置为计数器模式，用于测量外部脉冲频率。
 */
void Timer0_Init(void) // 1毫秒@12.000MHz
{
	AUXR &= 0x7F; // 定时器时钟12T模式
	TMOD &= 0xF0; // 清除T0的模式位
	TMOD |= 0x05; // 设置为计数器模式(M1=0, M0=1, C/T=1)，外部引脚T0(P3.4)的下降沿计数
	TL0 = 0x00;	  // 设置计数器初始值
	TH0 = 0x00;	  // 设置计数器初始值
	TF0 = 0;	  // 清除TF0溢出标志
	TR0 = 1;	  // 定时器0开始工作
}

/**
 * @brief 定时器1初始化函数
 * 配置为1ms中断的定时器，作为系统的心跳。
 */
void Timer1_Init(void) // 1毫秒@12.000MHz
{
	AUXR &= 0xBF; // 定时器时钟12T模式
	TMOD &= 0x0F; // 设置定时器模式 (清空T1的设置)
	TL1 = 0x18;	  // 设置定时初始值 (65536-1000) -> FC18H
	TH1 = 0xFC;	  // 设置定时初始值
	TF1 = 0;	  // 清除TF1溢出标志
	TR1 = 1;	  // 定时器1开始计时
	ET1 = 1;	  // 使能定时器1中断
	EA = 1;		  // 开启总中断
}

/**
 * @brief 定时器1中断服务函数
 * 每1ms执行一次，处理需要定时执行的任务。
 */
void Timer1_Isr(void) interrupt 3
{
	uwTick++; // 系统滴答计时器加1

	// 数码管动态扫描
	Seg_Pos = (++Seg_Pos) % 8; // 切换到下一位数码管
	// 判断显示缓冲区的值是否需要显示小数点
	// (通过给数字加上一个大偏移量','作为标志)
	if (Seg_Buf[Seg_Pos] > 20)
		// 显示数字，并点亮小数点
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
	else
		// 显示数字，不点亮小数点
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);

	// 频率测量，每1000ms (1s) 计算一次
	if (++Time_1s == 1000)
	{
		Time_1s = 0; // 计时器清零
		// 读取T0的计数值，即为过去1秒内的脉冲数(Hz)
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0; // 清空T0计数器，开始下一个周期的测量
	}

	// LED调光 (PWM)
	pwm_period = (++pwm_period) % 10; // PWM周期计数器 (0-9)
	if (pwm_period < pwm_compare)
		Led_Disp(ucLed); // 在占空比的有效时间内点亮LED
	else
		Led_Off(); // 在占空比的无效时间内熄灭LED

	// 如果串口正在接收数据，启动超时计时器
	if (Uart_Rx_Flag)
		Uart_Rx_Tick++;
}

/**
 * @brief 串口1中断服务函数
 * 当接收到数据时触发。
 */
void Uart1_Isr(void) interrupt 4
{
	if (RI) // 判断是否是接收中断
	{
		Uart_Rx_Flag = 1; // 设置接收标志
		Uart_Rx_Tick = 0; // 重置超时计时器
		// 将接收到的数据存入缓冲区
		Uart_Rx_Buf[Uart_Rx_Index++] = SBUF;
		RI = 0; // 手动清除接收中断请求位
		// 防止缓冲区溢出
		if (Uart_Rx_Index > 10)
		{
			Uart_Rx_Index = 0;
			memset(Uart_Rx_Buf, 0, 10); // 溢出则清空缓冲区
		}
	}
}

// 简单任务调度器结构体定义
typedef struct
{
	void (*task_func)(void);   // 任务函数指针
	unsigned long int rate_ms; // 任务执行周期（毫秒）
	unsigned long int last_ms; // 上次执行的时间戳
} task_t;

// 任务列表
idata task_t Scheduler_Task[] = {
	{Led_Proc, 1, 0},		   // LED任务，每1ms
	{Key_Proc, 10, 0},		   // 按键任务，每10ms
	{Seg_Proc, 20, 0},		   // 数码管任务，每20ms
	{Get_Time, 100, 0},		   // 获取时间任务，每100ms
	{Get_Temperature, 300, 0}, // 获取温度任务，每300ms
	{AD_DA, 150, 0},		   // AD/DA任务，每150ms
	{Get_Distance, 120, 0},	   // 获取距离任务，每120ms
	{Uart_Proc, 10, 0},		   // 串口处理任务，每10ms
};

idata unsigned char task_num; // 任务数量

/**
 * @brief 调度器初始化
 * 计算任务列表中的任务数量。
 */
void Scheduler_Init()
{
	task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}

/**
 * @brief 调度器运行函数
 * 循环检查每个任务是否到达执行时间。
 */
void Scheduler_Run()
{
	unsigned char i;
	for (i = 0; i < task_num; i++)
	{
		unsigned long int now_time = uwTick; // 获取当前时间
		// 判断是否到达任务执行时间 (当前时间 >= 上次执行时间 + 周期)
		if (now_time >= (Scheduler_Task[i].rate_ms + Scheduler_Task[i].last_ms))
		{
			Scheduler_Task[i].last_ms = now_time; // 更新上次执行时间
			Scheduler_Task[i].task_func();		  // 执行任务函数
		}
	}
}

/**
 * @brief 主函数
 */
void main()
{
	System_Init();	// 系统底层初始化
	Set_Rtc(ucRtc); // 初始化DS1302时钟时间

	// EEPROM读写测试
	EEPROM_Read(EEPROM_Data_R, 0, 8);  // 读取
	EEPROM_Write(EEPROM_Data_W, 0, 8); // 写入
	EEPROM_Read(EEPROM_Data_R, 0, 8);  // 再次读取以验证

	// 各模块初始化
	Timer0_Init();	  // 定时器0（频率计数）初始化
	Scheduler_Init(); // 任务调度器初始化
	Uart1_Init();	  // 串口1初始化
	Timer1_Init();	  // 定时器1（系统心跳）初始化

	// 主循环
	while (1)
	{
		Scheduler_Run(); // 循环执行任务调度器
	}
}