#include "init.h" // 包含了该模块相关的头文件
#include "key.h" // 包含了按键相关的头文件
#include "led.h" // 包含LED及其他外设相关的头文件
#include "seg.h" // 包含了数码管显示相关的头文件
#include "iic.h"       // 可能包含了一些I2C相关的宏定义或函数声明
#include "intrins.h"   // 包含了Keil C51编译器提供的内部函数，如_nop_()
#include "ds1302.h"	 // 包含了DS1302相关寄存器地址和命令的宏定义
#include "intrins.h" // 包含了Keil C51编译器提供的内部函数，如_nop_()



idata unsigned char Key_Val = 0, Key_Down = 0, Key_Up = 0, Key_Old = 0;
idata unsigned char  Seg_Pos = 0;
pdata unsigned char  Seg_Buf[8] = {10,10,10,10,10,10,10,10}; // 初始全部熄灭
pdata unsigned char  ucled[8]   = {0,0,0,0,0,0,0,0};         // 初始全部熄灭
idata unsigned long int uwTick = 0;
pdata unsigned char ucRtc[3] = {13, 3, 5};
idata unsigned long Freq=0;
idata unsigned long Freq_Chaoxian=2000;
idata  long Freq_Jiaozhun=0;
idata unsigned long Freq_Max=0;
pdata unsigned char Max_ucRtc[3] = {13, 3, 5};
idata unsigned char Seg_Show_Mode=0;
idata bit Type=0;
idata bit Type_Hui=0;
idata bit Flag=0;
idata long Rd_Freq=0;
unsigned char hour = 13, min =3, sec = 5;  // 当前时间

unsigned char Max_h, Max_m, Max_s; // 最大频率发生时间






void Key_Proc(void)
{
    // 三变量检测法
    Key_Val  = Key_Read();
    Key_Down = Key_Val  & (Key_Val  ^ Key_Old); // 按下沿：当前为1，上次为0
    Key_Up   = ~Key_Val & (Key_Val  ^ Key_Old); // 松开沿：当前为0，上次为1
    Key_Old  = Key_Val;                          // 保存本次状态供下次比较

	
		switch (Key_Down)
		{
			case 4:
					Seg_Show_Mode=(++	Seg_Show_Mode)%4;
					if(	Seg_Show_Mode==1)
						Type=0;
					if(Seg_Show_Mode==3)
					{
						Type_Hui=0;
					}
			break;
			case  5:
					if(Seg_Show_Mode==1)
					{Type^=1;}
					if(Seg_Show_Mode==3)
					{
						Type_Hui ^=1;
					
					}
			break;
			case 8:
					if(Type==0)
						Freq_Chaoxian=(Freq_Chaoxian>9000)?1000:Freq_Chaoxian+1000;
					if(Type==1)
						Freq_Jiaozhun=(Freq_Jiaozhun>900)?(-900):Freq_Jiaozhun+100;
			break;
			case 9:
					if(Type==0)
						Freq_Chaoxian=(Freq_Chaoxian<1000)?9000:Freq_Chaoxian-1000;
					if(Type==1)
						Freq_Jiaozhun=(Freq_Jiaozhun<-900)?(900):Freq_Jiaozhun-100;
			break;
		
		
		
		}
	
	
    
}



void Seg_Proc(void)
{
	if(Seg_Show_Mode==0)
	{
		Seg_Buf[0]=11;
		Seg_Buf[1]=10;
		Seg_Buf[2]=10;
		Seg_Buf[3]=(Freq/10000%10==0)?10:Freq/10000%10;
		Seg_Buf[4]=(Freq/1000%10==0&&Seg_Buf[3]==10)?10:Freq/1000%10;
		Seg_Buf[5]=(Freq/100%10==0&&Seg_Buf[4]==10)?10:Freq/100%10;
		Seg_Buf[6]=(Freq/10%10==0&&Seg_Buf[5]==10)?10:Freq/10%10;
		Seg_Buf[7]=Freq%10;
	
	}
	else if(Seg_Show_Mode==1)
	{
		if(Type==0)
		{
				Seg_Buf[0]=12;
				Seg_Buf[1]=2;
				Seg_Buf[2]=10;
				Seg_Buf[3]=10;
				Seg_Buf[4]=Freq_Chaoxian/1000%10;
		Seg_Buf[5]=Freq_Chaoxian/100%10;
				Seg_Buf[6]=Freq_Chaoxian/10%10;
				Seg_Buf[7]=Freq_Chaoxian%10;
		}
		else if (Type==1)
		{
			Seg_Buf[0]=12;
			Seg_Buf[1]=3;
				Seg_Buf[2]=10;
				Seg_Buf[3]=10;
			if(Freq_Jiaozhun>0)
			{
				Seg_Buf[4]=10;
				Seg_Buf[5]=Freq_Jiaozhun/100%10;
				Seg_Buf[6]=Freq_Jiaozhun/10%10;
				Seg_Buf[7]=Freq_Jiaozhun%10;
			}
			else if(Freq_Jiaozhun<0)
			{
				Seg_Buf[4]=13;
				Seg_Buf[5]=(-Freq_Jiaozhun)/100%10;
				Seg_Buf[6]=(-Freq_Jiaozhun)/10%10;
				Seg_Buf[7]=(-Freq_Jiaozhun%10);
			
			
			}
			else if(Freq_Jiaozhun==0)
			{
					Seg_Buf[4]=10;
				Seg_Buf[5]=10;
				Seg_Buf[6]=10;
				Seg_Buf[7]=0;
			
			}
		
		
		
		
		}
	
	
	
	}
	else if(Seg_Show_Mode==2)
	{
		Seg_Buf[0]= (ucRtc[0]/10%10==0)?0:ucRtc[0]/10%10;
	Seg_Buf[1]=ucRtc[0]%10;
			Seg_Buf[2]=13;
		Seg_Buf[3]=(ucRtc[1]/10%10==0)?0:ucRtc[1]/10%10;
		Seg_Buf[4]=ucRtc[1]%10;
		Seg_Buf[5]=13;
			Seg_Buf[6]=	(ucRtc[2]/10%10==0)?0:ucRtc[2]/10%10;
			Seg_Buf[7]=ucRtc[2]%10;
	}
	else if(Seg_Show_Mode==3)
	{
		if(Type_Hui==0)
		{
			Seg_Buf[0]=14;
			Seg_Buf[1]=11;
			Seg_Buf[2]=10;
				Seg_Buf[3]=(Freq_Max/10000%10==0)?10:Freq_Max/10000%10;
			Seg_Buf[4]=Freq_Max/1000%10;
			Seg_Buf[5]=Freq_Max/100%10;
		Seg_Buf[6]=Freq_Max/10%10;
		Seg_Buf[7]=Freq_Max%10;
		
		}
		else if(Type_Hui==1)
		{
			Seg_Buf[0]=14;
				Seg_Buf[1]=15;
			Seg_Buf[2]=Max_ucRtc[0]/10%10;
			Seg_Buf[3]=Max_ucRtc[0]%10;
			Seg_Buf[4]=Max_ucRtc[1]/10%10;
			Seg_Buf[5]=Max_ucRtc[1]%10;
			Seg_Buf[6]=Max_ucRtc[2]/10%10;
			Seg_Buf[7]=Max_ucRtc[2]%10;
			
		}
	
	
	
	
	
	
	}
	
	if(Flag==1)
	{
		Seg_Buf[0]=11;
		Seg_Buf[1]=10;
			Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=10;
		Seg_Buf[6]=16;
		Seg_Buf[7]=16;
		
	
	}






}

void Led_Proc(void)
{
	idata unsigned char i;
	for(i=0;i<8;i++)
	{
		ucled[i]=0;
	}
	if(Seg_Show_Mode==0)
	{
	ucled[0]=(uwTick/200)%2;
	}
	if(Freq>Freq_Chaoxian)
	{
	ucled[1]=(uwTick/200)%2;
	}
	if(Freq<0)
{
	ucled[1]=1;

}	

	




 Led_Disp(ucled);
}


void Freq_Proc(void)
{
	// 停止计数，读出1秒内的脉冲数
    TR0 = 0;
    Rd_Freq = (unsigned int)TH0 * 256 + TL0;
    TL0 = 0; // 清零，准备下一秒计数
    TH0 = 0;
    TR0 = 1; // 重新开始计数
		Freq=Rd_Freq+Freq_Jiaozhun;
	if(Freq<0)
	{
		Flag=1;
	
	}
	else
	{
		Flag=0;
	
	}
	if(Freq > Freq_Max)
        {
           Freq_Max = Freq;

           Max_ucRtc[0] = ucRtc[0];
           Max_ucRtc[1] = ucRtc[1];
						Max_ucRtc[2] = ucRtc[2];
				}
	
	




}

void DA_Proc()
{
	if(Rd_Freq<500)
	{
		Da_Write(1*51);
	
	}
	else if(Rd_Freq>Freq_Chaoxian)
	{
	
	Da_Write(5*51);
	}

	else if(Rd_Freq>500&&Rd_Freq<Freq_Chaoxian)
	{
			Da_Write(((Rd_Freq-500)*4/(Freq_Chaoxian-500)+1)*51);
	
	}
if(Freq<0)
{
Da_Write(0*51);
}


}




void RTC_Proc(void)
{
    Read_Rtc(ucRtc); // ucRtc[0]=时，[1]=分，[2]=秒
}

void Timer0_Init(void)
{
    AUXR &= 0x7F;   // T0使用12T模式（12个时钟周期计一次）
    TMOD &= 0xF0;   // 清除T0的模式位
    TMOD |= 0x05;   // T0设为模式1（16位计数器）+计数器模式
    TL0 = 0;        // 计数初值清零
    TH0 = 0;
    TR0 = 1;        // 启动T0开始计数
}


void Timer1_Init(void)
{
    AUXR &= 0xBF;   // T1使用12T模式
    TMOD &= 0x0F;   // 清除T1的模式位（T1模式0，16位自动重装）
    TL1 = 0x18;     // 定时初值，产生1ms中断
    TH1 = 0xFC;
    TF1 = 0;        // 清除中断标志
    TR1 = 1;        // 启动T1
    ET1 = 1;        // 允许T1中断
    EA  = 1;        // 开总中断
}


/* =====================================================
 * Timer1中断服务函数：每1ms执行一次
 * ===================================================== */
void Timer1_Isr(void) interrupt 3
{
    uwTick++; // 系统计时+1ms

    // 数码管扫描：每1ms切换到下一位
    // (++Seg_Pos) % 8 → 0,1,2,3,4,5,6,7,0,1,2...循环
    Seg_Pos = (++Seg_Pos) % 8;

    // 判断是否需要显示小数点
    // Seg_Buf里存的值>20，说明是"数字+小数点"
    // 实际数字 = Seg_Buf[Seg_Pos] - ','（','的ASCII是44）
    if(Seg_Buf[Seg_Pos] > 20)
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1); // 第三个参数1=带小数点
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);        // 第三个参数0=不带小数点
}



typedef struct
{
    void (*task_func)(void); // 函数指针，指向要执行的任务函数
    unsigned long int rate_ms;  // 执行周期（毫秒）
    unsigned long int last_ms;  // 上次执行时刻
} task_t;




idata task_t Scheduler_Task[] =
{
//  {函数名,       周期ms,  上次执行时刻}
    {Led_Proc,      1,    0}, // LED每1ms刷新，保证L4闪烁精度
    {Key_Proc,      10,   0}, // 按键每10ms扫描，满足0.2秒响应要求
    {Seg_Proc,      20,   0}, // 数码管内容每20ms更新
		 {Freq_Proc,     1000, 0} ,
		 {RTC_Proc,      500,  0}, // 时间每500ms读一次
     {DA_Proc,    80   ,0},

};



idata unsigned char task_num; // 任务总数，由Scheduler_Init计算


void Scheduler_Init(void)
{
    // 用数组总字节数除以单个元素字节数，自动计算任务数量
    // 好处：增减任务时不需要手动修改task_num
    task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}

void Scheduler_Run(void)
{
    unsigned char i;
    for(i = 0; i < task_num; i++)
    {
        // 读一次uwTick存到局部变量，避免判断过程中uwTick被ISR修改
        unsigned long int now_Time = uwTick;

        // 判断是否到了该任务的执行时间
        if(now_Time >= (Scheduler_Task[i].rate_ms + Scheduler_Task[i].last_ms))
        {
            Scheduler_Task[i].last_ms = now_Time; // 更新上次执行时刻
            Scheduler_Task[i].task_func();         // 执行任务函数
        }
    }
}


/* =====================================================
 * 主函数
 * ===================================================== */
void main(void)
{
    System_Init();    // 硬件初始化（IO口、外设等）
    Timer0_Init();    // 初始化T0计数器（频率测量）
    Scheduler_Init(); // 计算任务数量
    Timer1_Init();    // 初始化T1定时器并开启中断（最后开，避免中断打乱初始化）

    while(1)
    {
        Scheduler_Run(); // 循环检查并执行到期的任务
    }
}
