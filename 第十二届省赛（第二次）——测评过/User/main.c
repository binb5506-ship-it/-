/*
 * 第十二届蓝桥杯单片机省赛（第二次）
 * 完整修正版 main.c
 */
 
#include "init.h"
#include "key.h"
#include "led.h"
#include "seg.h"
#include "iic.h"
#include "intrins.h"
#include "stdio.h"
#include "string.h"
 
/* =========================================================
 * 全局变量定义
 * ========================================================= */
 
idata unsigned char  Key_Val = 0, Key_Down = 0, Key_Up = 0, Key_Old = 0;
idata unsigned char  Seg_Pos = 0;
pdata unsigned char  Seg_Buf[8] = {10,10,10,10,10,10,10,10};
pdata unsigned char  ucled[8]   = {0,0,0,0,0,0,0,0};
idata unsigned long  int uwTick = 0;
 
/* 显示界面：0=频率  1=周期  2=电压 */
idata unsigned char  Seg_Show_Mode = 0;
 
/* 电压通道选择：0=通道1(光敏RD1)  1=通道3(电位器Rb2) */
idata bit            Mode = 0;
 
/* LED功能开关：0=开启  1=禁用（全灭） */
idata bit            Ucled_Flag = 0;
 
/* 频率（Hz）和周期（μs） */
idata unsigned long  int Freq   = 0;
idata unsigned long  int Time_a = 0;
 
idata unsigned long  int Old_Freq = 0;
 
/*
 * [修正] AD电压变量全部改为 unsigned int
 * 原来用 unsigned char，最大只能存255。
 * 5.00V对应的存储值是500，超过了255会溢出，
 * 导致电压大于约2.55V时显示全部错误。
 */
idata unsigned int   AD_Ch1_100x = 0;   /* 通道1：光敏电阻 RD1 */
idata unsigned int   AD_Ch3_100x = 0;   /* 通道3：电位器 Rb2  */
idata unsigned int   AD_In_100x  = 0;   /* 当前界面显示用     */
idata unsigned int   Old_huancun = 0;   /* 缓存的通道3电压值  */
 
idata unsigned long  int Key7_Press_Time = 0;
 
 
/* =========================================================
 * Timer0 初始化：计数器模式（用于频率测量）
 * ========================================================= */
void Counter0_Init(void)
{
    TMOD &= 0xF0;
    TMOD |= 0x05;   /* T0：模式1，计数模式 */
    TL0 = 0;
    TH0 = 0;
    TR0 = 1;
}
 
 
/* =========================================================
 * 频率测量任务：每1000ms执行一次
 * ========================================================= */
void Freq_Proc(void)
{
    TR0 = 0;
    Freq = (unsigned long int)TH0 * 256 + TL0;
    TL0 = 0;
    TH0 = 0;
    TR0 = 1;
 
    if(Freq != 0)
        Time_a = 1000000UL / Freq;
    else
        Time_a = 0;
}
 
 
/* =========================================================
 * 按键处理
 * ========================================================= */
void Key_Proc(void)
{
    Key_Val  = Key_Read();
    Key_Down = Key_Val  & (Key_Val  ^ Key_Old);
    Key_Up   = ~Key_Val & (Key_Val  ^ Key_Old);
    Key_Old  = Key_Val;
 
    switch(Key_Down)
    {
        case 4:
            Seg_Show_Mode = (++Seg_Show_Mode) % 3;
            if(Seg_Show_Mode == 2)
                Mode = 0;
        break;
 
        case 5:
            if(Seg_Show_Mode == 2)
                Mode ^= 1;
        break;
 
        case 6:
            Old_huancun = AD_Ch3_100x;
        break;
 
        case 7:
            Old_Freq = Freq;
            Key7_Press_Time = uwTick;
        break;
    }
 
    if(Key_Up == 7)
    {
        if(uwTick - Key7_Press_Time >= 1000)
        {
            Ucled_Flag ^= 1;
        }
    }
}
 
 
/* =========================================================
 * 数码管内容计算
 * ========================================================= */
void Seg_Proc(void)
{
    if(Seg_Show_Mode == 0)
    {
        /* 频率界面：F + 7位频率值 */
        Seg_Buf[0] = 11; /* F (seg_dula[11]=0x8c) */
        Seg_Buf[1] = (Freq/1000000%10 == 0)                         ? 10 : Freq/1000000%10;
        Seg_Buf[2] = (Freq/100000%10  == 0 && Seg_Buf[1] == 10)     ? 10 : Freq/100000%10;
        Seg_Buf[3] = (Freq/10000%10   == 0 && Seg_Buf[2] == 10)     ? 10 : Freq/10000%10;
        Seg_Buf[4] = (Freq/1000%10    == 0 && Seg_Buf[3] == 10)     ? 10 : Freq/1000%10;
        Seg_Buf[5] = (Freq/100%10     == 0 && Seg_Buf[4] == 10)     ? 10 : Freq/100%10;
        Seg_Buf[6] = (Freq/10%10      == 0 && Seg_Buf[5] == 10)     ? 10 : Freq/10%10;
        /*
         * [修正] 个位（最后一位）永远不消隐
         * 原代码：当Freq=0时，所有位都满足消隐条件，导致7个数字位全灭，
         * 只剩下"F"一个字，初始上电时数码管几乎全黑。
         * 个位必须始终显示，哪怕值为0。
         */
        Seg_Buf[7] = Freq % 10;
    }
    else if(Seg_Show_Mode == 1)
    {
        /* 周期界面：N + 7位周期值（μs） */
        Seg_Buf[0] = 12; /* N (seg_dula[12]=0xc8) */
        Seg_Buf[1] = (Time_a/1000000%10 == 0)                          ? 10 : Time_a/1000000%10;
        Seg_Buf[2] = (Time_a/100000%10  == 0 && Seg_Buf[1] == 10)      ? 10 : Time_a/100000%10;
        Seg_Buf[3] = (Time_a/10000%10   == 0 && Seg_Buf[2] == 10)      ? 10 : Time_a/10000%10;
        Seg_Buf[4] = (Time_a/1000%10    == 0 && Seg_Buf[3] == 10)      ? 10 : Time_a/1000%10;
        Seg_Buf[5] = (Time_a/100%10     == 0 && Seg_Buf[4] == 10)      ? 10 : Time_a/100%10;
        Seg_Buf[6] = (Time_a/10%10      == 0 && Seg_Buf[5] == 10)      ? 10 : Time_a/10%10;
        /* 同上，个位永远不消隐 */
        Seg_Buf[7] = Time_a % 10;
    }
    else if(Seg_Show_Mode == 2)
    {
        /*
         * 电压界面：U - [通道号] [灭] [灭] [整数.][小数高][小数低]
         * 例：通道1，3.25V → U - 1  空  空  3. 2 5
         *
         * seg_dula 表：
         *   13 = U (0xc1)
         *   14 = - (0xbf)
         */
        Seg_Buf[0] = 13;               /* U */
        Seg_Buf[1] = 14;               /* - */
        Seg_Buf[2] = Mode ? 3 : 1;     /* 通道号 */
        Seg_Buf[3] = 10;               /* 熄灭 */
        Seg_Buf[4] = 10;               /* 熄灭 */
        /*
         * 整数位需要带小数点：
         * 在 Timer1_Isr 中，判断 Seg_Buf > 20 则带小数点显示，
         * 所以整数位加上 ',' (ASCII=44) 作为标志。
         * 例：AD_In_100x=325 → 整数位=3，存 3+44=47
         */
        Seg_Buf[5] = AD_In_100x / 100 % 10 + ','; /* 整数位（带小数点） */
        Seg_Buf[6] = AD_In_100x / 10  % 10;        /* 小数第一位 */
        Seg_Buf[7] = AD_In_100x       % 10;        /* 小数第二位 */
    }
}
 
 
/* =========================================================
 * LED指示灯处理
 * ========================================================= */
void Led_Proc(void)
{
    unsigned char i;
 
    for(i = 0; i < 8; i++) ucled[i] = 0;   /* 先全灭 */
 
    if(Ucled_Flag == 0)   /* LED功能开启状态 */
    {
        ucled[0] = (AD_Ch3_100x > Old_huancun);    /* L1 */
        ucled[1] = (Freq > Old_Freq);              /* L2 */
 
        if(Seg_Show_Mode == 0)       ucled[2] = 1; /* L3：频率界面 */
        else if(Seg_Show_Mode == 1)  ucled[3] = 1; /* L4：周期界面 */
        else if(Seg_Show_Mode == 2)  ucled[4] = 1; /* L5：电压界面 */
    }
    /* Ucled_Flag==1 时数组已全零，直接写入即可 */
 
    /*
     * 注意：Led_Disp() 不在这里调用，而是在 Timer1_Isr 里调用。
     * 原因：Led_Disp 和 Seg_Disp 都要操作 P2 口（138译码器），
     * 如果在主循环调用 Led_Disp，会和每1ms触发的定时器中断里的
     * Seg_Disp 互相抢占 P2，导致数码管显示乱码。
     * 放进同一个中断里顺序执行，彻底消除冲突。
     */
}
 
 
/* =========================================================
 * ADC采集任务：每50ms执行一次
 * ========================================================= */
void AD_Proc(void)
{
    /*
     * [修正] 强制转换为 unsigned int 再做乘法，
     * 防止 Ad_Read() 返回的 unsigned char(最大255)
     * 在乘以500之前就因为8位运算溢出截断。
     * 255 * 500 = 127500，超过了 unsigned char 的范围。
     */
    Ad_Read(0x01);
    AD_Ch1_100x = (unsigned int)Ad_Read(0x01) * 500 / 255;
    Ad_Read(0x03);
    AD_Ch3_100x = (unsigned int)Ad_Read(0x03) * 500 / 255;
 
    if(Mode == 0)
        AD_In_100x = AD_Ch1_100x;
    else
        AD_In_100x = AD_Ch3_100x;
}
 
 
/* =========================================================
 * Timer1 初始化：1ms定时中断
 * ========================================================= */
void Timer1_Init(void)   /* 1毫秒@12.000MHz */
{
    AUXR &= 0xBF;
    TMOD &= 0x0F;
    TL1 = 0x18;
    TH1 = 0xFC;
    TF1 = 0;
    TR1 = 1;
    ET1 = 1;
    EA  = 1;
}
 
 
/* =========================================================
 * Timer1 中断服务：每1ms执行
 * ========================================================= */
void Timer1_Isr(void) interrupt 3
{
    uwTick++;
 
    Seg_Pos = (++Seg_Pos) % 8;
 
    /*
     * Seg_Buf 值 > 20 表示该位需要带小数点（整数位加了 ',' 偏移44）
     * 减去 ',' 还原真实数字索引，然后带小数点显示
     */
    if(Seg_Buf[Seg_Pos] > 20)
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
 
    /*
     * 每扫完8位数码管（8ms）刷新一次LED。
     * 放在中断里和 Seg_Disp 顺序执行，避免主循环调用时
     * 与中断争用 P2 口（138译码器）造成显示乱码。
     */
    if(Seg_Pos == 7)
        Led_Disp(ucled);
}
 
 
/* =========================================================
 * 任务调度器
 * ========================================================= */
typedef struct
{
    void (*task_func)(void);
    unsigned long int rate_ms;
    unsigned long int last_ms;
} task_t;
 
idata task_t Scheduler_Task[] =
{
    {Led_Proc,   1,    0},
    {Key_Proc,   10,   0},
    {Seg_Proc,   20,   0},
    {AD_Proc,    50,   0},
    {Freq_Proc,  1000, 0},
};
 
idata unsigned char task_num;
 
void Scheduler_Init(void)
{
    task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}
 
void Scheduler_Run(void)
{
    unsigned char i;
    for(i = 0; i < task_num; i++)
    {
        unsigned long int now_Time = uwTick;
        if(now_Time >= (Scheduler_Task[i].rate_ms + Scheduler_Task[i].last_ms))
        {
            Scheduler_Task[i].last_ms = now_Time;
            Scheduler_Task[i].task_func();
        }
    }
}
 
 
/* =========================================================
 * 主函数
 * ========================================================= */
void main(void)
{
    System_Init();
    Counter0_Init();
    Scheduler_Init();
    Timer1_Init();
 
    while(1)
    {
        Scheduler_Run();
    }
}
 