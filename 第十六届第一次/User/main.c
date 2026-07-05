/* =========================================================
 * main.c  —  第十六届蓝桥杯单片机设计与开发 省赛（大学组）
 *            完整修正版
 * ========================================================= */
#include "init.h"
#include "iic.h"
#include "intrins.h"
#include "key.h"
#include "led.h"
#include "onewire.h"
#include "seg.h"
#include "ultrasound.h"
 
/* ── 按键三变量 ── */
idata unsigned char Key_Val  = 0;
idata unsigned char Key_Down = 0;
idata unsigned char Key_Up   = 0;
idata unsigned char Key_Old  = 0;
 
/* ── 数码管 ── */
idata unsigned char Seg_Pos = 0;
/*
 * 数码管缓冲区编码约定（需与 Seg_Disp 实现一致）：
 *   0~9  = 数字
 *   10   = 熄灭
 *   11   = 'C'
 *   12   = 'N'
 *   13   = 'L'
 *   14   = 'P'
 *   >20  = 带小数点（值减去 ',' 后传入）
 */
pdata unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
 
/* ── LED ── */
pdata unsigned char ucled[8]   = {0,0,0,0,0,0,0,0};
 
/* ── 系统毫秒计时器（Timer1 ISR 每 1ms 自增） ── */
idata unsigned long int uwTick = 0;
 
/* ── 光强 ── */
idata unsigned char Dengji = 1;          /* 光强等级 1~4 */
 
/* ── 温度 ── */
idata unsigned char Wendu     = 0;       /* 当前温度 */
idata unsigned char Wendu_Can = 30;      /* 温度参数 PC，初始 30℃ */
idata bit           Wendu_Flag = 0;      /* 高温标志 */
 
/* ── 距离 / 接近 ── */
idata unsigned char Distance     = 0;   /* 本次采集距离 */
idata unsigned char Old_Distance = 0;   /* 上次采集距离（用于 ΔL） */
idata unsigned char Distance_Can = 30;  /* 距离参数 PL，初始 30cm */
idata bit           Distance_Flag = 0;  /* 接近标志 */
 
/* ── 运动状态 ── */
idata unsigned char Yundong            = 1;  /* 1=静止 2=徘徊 3=跑动 */
idata bit           Yundong_Lock       = 0;  /* 3s 锁定标志 */
idata unsigned long Yundong_Lock_Start = 0;  /* 锁定起始时刻 */
 
/* ── 继电器 ── */
idata unsigned int Times      = 0;  /* 吸合次数计数 */
idata bit          Relay_Last = 0;  /* 上次继电器状态（用于上升沿检测） */
 
/* ── 界面 ── */
idata unsigned char Seg_Show_Mode = 0;  /* 0=环境 1=运动 2=参数 3=统计 */
idata bit           Mode          = 0;  /* 0=温度参数子界面 1=距离参数子界面 */
 
/* ── 参数界面进入时锁定的 LED/继电器状态 ── */
idata unsigned char Lock_Dengji        = 1;
idata unsigned char Lock_Yundong       = 1;
idata bit           Lock_Distance_Flag = 0;
idata bit           Lock_Relay_State   = 0;
 
/* ── S8+S9 组合键持续计时 ── */
idata unsigned long S89_Start  = 0;
idata bit           S89_Active = 0;
 
 
/* =========================================================
 * 按键处理（10ms 调用）
 * ========================================================= */
void Key_Proc(void)
{
    Key_Val  = Key_Read();
    Key_Down = Key_Val  & (Key_Val  ^ Key_Old);
    Key_Up   = ~Key_Val & (Key_Val  ^ Key_Old);
    Key_Old  = Key_Val;
 
    switch(Key_Down)
    {
        /* S4：循环切换四个主界面 */
        case 4:
            Seg_Show_Mode = (++Seg_Show_Mode) % 4;
            if(Seg_Show_Mode == 2)
            {
                /* 进入参数界面：锁定当前 LED/继电器状态；
                 * 【修正 #1】同时将 Mode 重置为 0（默认温度参数子界面） */
                Lock_Dengji        = Dengji;
                Lock_Yundong       = Yundong;
                Lock_Distance_Flag = Distance_Flag;
                Lock_Relay_State   = (Wendu_Flag && Distance_Flag) ? 1 : 0;
                Mode = 0;
            }
            break;
 
        /* S5：仅在参数界面切换子界面
         * 【修正 #2】原代码在任意界面均可触发，应限制在 Seg_Show_Mode==2 */
        case 5:
            if(Seg_Show_Mode == 2)
                Mode ^= 1;
            break;
 
        /* S8："加"按键
         * 【修正 #3】边界由 >79 改为 >=80，上限修正为 80℃/80cm */
        case 8:
            if(Seg_Show_Mode == 2 && Mode == 0)
                Wendu_Can = (Wendu_Can >= 80) ? 80 : Wendu_Can + 1;
            else if(Seg_Show_Mode == 2 && Mode == 1)
                Distance_Can = (Distance_Can >= 80) ? 80 : Distance_Can + 5;
            break;
 
        /* S9："减"按键
         * 【修正 #4】边界由 <21 改为 <=20，下限修正为 20℃/20cm */
        case 9:
            if(Seg_Show_Mode == 2 && Mode == 0)
                Wendu_Can = (Wendu_Can <= 20) ? 20 : Wendu_Can - 1;
            else if(Seg_Show_Mode == 2 && Mode == 1)
                Distance_Can = (Distance_Can <= 20) ? 20 : Distance_Can - 5;
            break;
    }
 
    /* ── S8+S9 组合键：统计界面下同时按住 ≥2s 清零 ──
     *
     * Key_Read() 在 S8(P3.3) 与 S9(P3.2) 同时按下时直接返回 89，
     * 因此只需检测 Key_Val == 89，无需任何位运算。
     *
     * 持续计时逻辑：
     *   - Key_Val 变为 89 的第一个 10ms 周期：记录起始时刻，置 S89_Active
     *   - 后续每次调用 Key_Val 仍为 89：检查是否已满 2000ms
     *   - Key_Val 不再为 89（任一键松开）：复位计时器
     *
     * 注意：Key_Down 同样可能等于 89（首次下沿），但 switch 中无 case 89，
     *       不会误触发 S8/S9 的单键逻辑，天然安全。               */
    if(Seg_Show_Mode == 3 && Key_Val == 89)
    {
        if(!S89_Active)
        {
            S89_Active = 1;
            S89_Start  = uwTick;
        }
        else if(uwTick - S89_Start >= 2000)
        {
            Times      = 0;
            S89_Active = 0;   /* 清零后复位，避免反复清零 */
        }
    }
    else
    {
        S89_Active = 0;       /* 任一键松开或不在统计界面，重置计时 */
    }
}
 
 
/* =========================================================
 * 数码管内容刷新（20ms 调用）
 * ========================================================= */
void Seg_Proc(void)
{
    unsigned char i;
    for(i = 0; i < 8; i++) Seg_Buf[i] = 10;
 
    switch(Seg_Show_Mode)
    {
        /* ── 界面 0：环境状态 ── C[温度]___N[等级] ── */
        case 0:
            Seg_Buf[0] = 11;  /* 'C' */
            /* 十位为 0 时熄灭（题目图示高位不显示前导零） */
            Seg_Buf[1] = (Wendu / 10 % 10 == 0) ? 10 : Wendu / 10 % 10;
            Seg_Buf[2] = Wendu % 10;
            /* 位 3~5 熄灭 */
            Seg_Buf[6] = 12;  /* 'N' */
            /* 【修正 #6】原代码显示 Old_Dengji，应实时显示 Dengji */
            Seg_Buf[7] = Dengji;
            break;
 
        /* ── 界面 1：运动检测 ── L[状态]___[距离 3 位] ── */
        case 1:
            Seg_Buf[0] = 13;  /* 'L' */
            /* 【修正 #7】原代码显示 Yundong_Old，应实时显示 Yundong */
            Seg_Buf[1] = Yundong;
            /* 距离固定 3 位，不足 3 位高位补 0 */
            Seg_Buf[5] = Distance / 100 % 10;
            Seg_Buf[6] = Distance / 10  % 10;
            Seg_Buf[7] = Distance % 10;
            break;
 
        /* ── 界面 2：参数设置 ── */
        case 2:
            if(Mode == 0)
            {
                Seg_Buf[0] = 14;  /* 'P' */
                Seg_Buf[1] = 11;  /* 'C' */
                Seg_Buf[6] = Wendu_Can / 10 % 10;
                Seg_Buf[7] = Wendu_Can % 10;
            }
            else
            {
                Seg_Buf[0] = 14;  /* 'P' */
                Seg_Buf[1] = 13;  /* 'L' */
                Seg_Buf[6] = Distance_Can / 10 % 10;
                Seg_Buf[7] = Distance_Can % 10;
            }
            break;
 
        /* ── 界面 3：统计数据 ── NC___[次数 4 位] ── */
        case 3:
            Seg_Buf[0] = 12;  /* 'N' */
            Seg_Buf[1] = 11;  /* 'C' */
            /* 【修正 #8】原代码显示 Old_Times，应直接显示实时 Times */
            Seg_Buf[4] = (Times / 1000 % 10 == 0) ? 10 : Times / 1000 % 10;
            Seg_Buf[5] = (Times / 100  % 10 == 0) ? 10 : Times / 100  % 10;
            Seg_Buf[6] = (Times / 10   % 10 == 0) ? 10 : Times / 10   % 10;
            Seg_Buf[7] = Times % 10;
            break;
    }
}
 
 
/* =========================================================
 * LED 刷新（1ms 调用，保证 L8 闪烁精度）
 * ========================================================= */
void Led_Proc(void)
{
    unsigned char j;
    for(j = 0; j < 8; j++) ucled[j] = 0;
 
    if(Seg_Show_Mode == 2)
    {
        /* 参数界面：使用进入时锁定的状态，不允许变化
         * 【修正 #9】原代码在参数界面仍实时更新 LED */
 
        /* L1~L4：接近 + 光强等级（锁定值） */
        if(Lock_Distance_Flag)
        {
            if(Lock_Dengji == 1)
            { ucled[0]=1; }
            else if(Lock_Dengji == 2)
            { ucled[0]=1; ucled[1]=1; }
            else if(Lock_Dengji == 3)
            { ucled[0]=1; ucled[1]=1; ucled[2]=1; }
            else if(Lock_Dengji == 4)
            { ucled[0]=1; ucled[1]=1; ucled[2]=1; ucled[3]=1; }
        }
        /* L8：运动状态（锁定值） */
        if(Lock_Yundong == 2)
            ucled[7] = 1;
        else if(Lock_Yundong == 3)
            ucled[7] = (uwTick / 100) % 2;
    }
    else
    {
        /* 正常界面：实时状态 */
 
        /* L1~L4：仅在"接近"判定时点亮
         * 【修正 #10】原代码未检查 Distance_Flag，无论是否接近均点亮 */
        if(Distance_Flag)
        {
            if(Dengji == 1)
            { ucled[0]=1; }
            else if(Dengji == 2)
            { ucled[0]=1; ucled[1]=1; }
            else if(Dengji == 3)
            { ucled[0]=1; ucled[1]=1; ucled[2]=1; }
            else if(Dengji == 4)
            { ucled[0]=1; ucled[1]=1; ucled[2]=1; ucled[3]=1; }
        }
        /* L8：运动状态 */
        if(Yundong == 2)
            ucled[7] = 1;
        else if(Yundong == 3)
            ucled[7] = (uwTick / 100) % 2;
        /* Yundong==1：ucled[7]=0（已在开头清零） */
    }
 
    Led_Disp(ucled);
}
 
 
/* =========================================================
 * 继电器控制 + 吸合计数（1ms 调用）
 * ========================================================= */
void Relay_Proc(void)
{
    bit relay_now;
 
    if(Seg_Show_Mode == 2)
    {
        /* 参数界面：继电器状态锁定
         * 【修正 #11】原代码继电器逻辑在 Led_Proc 中，参数界面无法锁定继电器 */
        Relay(Lock_Relay_State);
        return;
    }
 
    /* 同时满足"接近"和"高温"时吸合 */
    relay_now = (Wendu_Flag && Distance_Flag) ? 1 : 0;
 
    /* 上升沿检测：每次从断开→吸合计一次
     * 【修正 #12】原代码 Times 从未自增，继电器吸合次数始终为 0 */
    if(relay_now && !Relay_Last)
        Times++;
 
    Relay_Last = relay_now;
 
    /* 【修正 #13】原代码只调用 Relay(1)，从未调用 Relay(0)，继电器无法断开 */
    Relay(relay_now);
}
 
 
/* =========================================================
 * 光强采集（50ms 调用）
 * 【修正 #14】原代码未加入任务调度器，光强数据从不更新
 * ========================================================= */
void Get_Da(void)
{
    unsigned char adc = Ad_Read(0x03);
    /* 直接与 ADC 原始值比较（5V/255≈0.0196V/bit）
     * 3.0V→153, 2.0V→102, 0.5V≈26
     * 【修正 #15】原代码使用 /51*10 转换，0.5V 阈值因整数截断丢失精度 */
    if(adc >= 153)       Dengji = 1;  /* N1: ≥3.0V */
    else if(adc >= 102)  Dengji = 2;  /* N2: 2.0~3.0V */
    else if(adc >= 26)   Dengji = 3;  /* N3: 0.5~2.0V */
    else                 Dengji = 4;  /* N4: <0.5V  */
}
 
 
/* =========================================================
 * 温度采集（80ms 调用）
 * ========================================================= */
void Get_Wendu(void)
{
    Wendu = rd_temperature();
    /* 【修正 #16】原代码只置位 Wendu_Flag，从不清零；
     *             温度下降后"高温"标志永远保持 1 */
    Wendu_Flag = (Wendu > Wendu_Can) ? 1 : 0;
}
 
 
/* =========================================================
 * 距离采集 + 运动判定（1000ms 调用）
 * ========================================================= */
void Get_Distance(void)
{
    unsigned char new_Yundong;
    unsigned char delta;
 
    /* 保存上次距离，采集新距离
     * 【修正 #17】原代码在同一函数内连续读取两次，不符合"间隔 1 秒"要求 */
    Old_Distance = Distance;
    Distance     = Ut_Wave_Data();
 
    /* 接近判定：距离 < 参数
     * 【修正 #18】原代码写作 Distance > Distance_Can，方向相反；
     *             同时只置位从不清零 */
    Distance_Flag = (Distance < Distance_Can) ? 1 : 0;
 
    /* 计算距离差绝对值 */
    delta = (Distance >= Old_Distance) ?
            (Distance - Old_Distance) :
            (Old_Distance - Distance);
 
    /* 判定新运动状态 */
    if(delta < 5)       new_Yundong = 1;  /* 静止 L1 */
    else if(delta < 10) new_Yundong = 2;  /* 徘徊 L2 */
    else                new_Yundong = 3;  /* 跑动 L3 */
 
    /* 运动状态更新与 3s 锁定
     * 【修正 #19】原代码无锁定机制，状态每秒都可变化 */
    if(!Yundong_Lock)
    {
        if(new_Yundong != Yundong)
        {
            Yundong            = new_Yundong;
            Yundong_Lock       = 1;
            Yundong_Lock_Start = uwTick;
        }
    }
    else
    {
        /* 锁定期间继续采集距离，但不更新运动状态 */
        if(uwTick - Yundong_Lock_Start >= 3000)
            Yundong_Lock = 0;
    }
}
 
 
/* =========================================================
 * Timer1 初始化：1ms 定时
 * ========================================================= */
void Timer1_Init(void)
{
    AUXR &= 0xBF;
    TMOD &= 0x0F;
    TL1   = 0x18;
    TH1   = 0xFC;
    TF1   = 0;
    TR1   = 1;
    ET1   = 1;
    EA    = 1;
}
 
 
/* =========================================================
 * Timer1 ISR：每 1ms 执行
 * ========================================================= */
void Timer1_Isr(void) interrupt 3
{
    uwTick++;
    Seg_Pos = (++Seg_Pos) % 8;
 
    if(Seg_Buf[Seg_Pos] > 20)
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
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
    { Led_Proc,       1,    0 },   /* 1ms：LED 刷新（L8 闪烁精度） */
    { Relay_Proc,     1,    0 },   /* 1ms：继电器 + 计数（响应时间 ≤0.1s） */
    { Key_Proc,       10,   0 },   /* 10ms：按键扫描 */
    { Seg_Proc,       20,   0 },   /* 20ms：数码管内容更新 */
    { Get_Da,         50,   0 },   /* 50ms：光强采集 */  /* 【修正 #14】*/
    { Get_Wendu,      80,   0 },   /* 80ms：温度采集 */
    { Get_Distance,   1000, 0 },   /* 1000ms：距离采集 */
};
 
idata unsigned char task_num;
 
void Scheduler_Init(void)
{
    task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}
 
void Scheduler_Run(void)
{
    unsigned char     i;
    unsigned long int now;
 
    for(i = 0; i < task_num; i++)
    {
        now = uwTick;
        if(now >= Scheduler_Task[i].last_ms + Scheduler_Task[i].rate_ms)
        {
            Scheduler_Task[i].last_ms = now;
            Scheduler_Task[i].task_func();
        }
    }
}
 
 
/* =========================================================
 * 主函数
 * 上电初始状态：
 *   环境状态界面（Seg_Show_Mode=0）
 *   温度参数 30℃（Wendu_Can=30）
 *   距离参数 30cm（Distance_Can=30）
 *   未涉及 LED 熄灭，蜂鸣器静音（由 System_Init 保证）
 * ========================================================= */
void main(void)
{
    System_Init();
    Timer1_Init();
    Scheduler_Init();
 
    while(1)
        Scheduler_Run();
}