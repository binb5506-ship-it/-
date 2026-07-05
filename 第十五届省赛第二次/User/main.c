/*
 * =========================================================
 * 第十五届蓝桥杯 单片机设计与开发项目 省赛（大学组）
 * 完整修正版 main.c
 *
 * 错误修正总清单（共16处）：
 *
 * [致命编译错误——导致所有测试失败]
 *  Bug-0  DA_Write → Da_Write（库函数名大小写错误，编译失败，
 *                             hex为空，所有50项测试全部失败，
 *                             仅继电器初始断开状态因硬件默认而通过）
 *
 * [逻辑错误]
 *  Bug-1  switch(Seg_Show_Mode==0) → switch(Seg_Show_Mode)
 *  Bug-2  DA_Out加减边界判断：unsigned char溢出和下溢问题
 *  Bug-3  Temperature_Jiaozhun必须是signed char（原unsigned char）
 *  Bug-4  S13长短按逻辑完全错误（应在Key_Up时判断时长）
 *  Bug-5  L2/L3/L4计时基准变量在Led_Proc内每ms重置（永远计不到2s/3s）
 *  Bug-6  L1应与DA_Mode==0绑定（原误与Seg_Show_Mode==0绑定）
 *  Bug-7  完全缺失L8（解锁指示灯）逻辑
 *  Bug-8  Temperature_Flag用异或翻转（应直接置1）
 *  Bug-9  Temperature_Old保存顺序错误（应先存Old再读新值）
 *  Bug-10 校准值从未加入温度计算（T = T_DS18B20 + 校准值）
 *  Bug-11 Da_Write()无返回值，不能用其返回值
 *  Bug-12 DAC线性插值公式有精度损失（改为×153/30）
 *  Bug-13 校准值应"退出界面时生效"（原代码立即生效）
 *  Bug-14 Temperature_Proc完全未加入调度器（温度永远不被采集）
 *  Bug-15 继电器控制条件多余Temperature_Flag==0
 * =========================================================
 */
 
#include "init.h"
#include "onewire.h"
#include "iic.h"
#include "intrins.h"
#include "key.h"
#include "led.h"
#include "seg.h"
 
/* =========================================================
 * 全局变量定义
 * ========================================================= */
 
/* --- 按键三变量 --- */
idata unsigned char Key_Val  = 0;
idata unsigned char Key_Down = 0;
idata unsigned char Key_Up   = 0;
idata unsigned char Key_Old  = 0;
 
/* --- 数码管扫描位索引（0~7），由Timer1 ISR驱动 --- */
idata unsigned char Seg_Pos = 0;
 
/*
 * 数码管缓冲区
 * 0~9=数字, 10=熄灭, 11='C', 12='A', 13='P', 14='-', >20=带小数点
 */
pdata unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
 
/* LED状态缓冲区，ucled[0]=L1 ... ucled[7]=L8，0=灭 1=亮 */
pdata unsigned char ucled[8] = {0,0,0,0,0,0,0,0};
 
/* 系统毫秒计时器，由Timer1 ISR每1ms自增 */
idata unsigned long int uwTick = 0;
 
/* 界面模式：0=温度界面  1=DAC输出控制界面  2=校准值界面 */
idata unsigned char Seg_Show_Mode = 0;
 
/* 经校准后的最终温度（整数摄氏度）*/
idata unsigned char Temperature     = 0;
/* 上次采样温度，用于比较变化 */
idata unsigned char Temperature_Old = 0;
 
/*
 * Bug-3修正：校准值必须是有符号类型，范围 -9~+9
 * Temperature_Jiaozhun      : 已生效的校准值（退出校准界面时更新）
 * Temperature_Jiaozhun_Edit : 校准界面中正在编辑的临时值
 */
signed char idata Temperature_Jiaozhun      = 0;
signed char idata Temperature_Jiaozhun_Edit = 0;
 
/*
 * Bug-8修正：只置1，由Led_Proc在3s后清零，不用XOR翻转
 * Temperature_Flag=1 → L4开始闪烁计时
 */
idata bit           Temperature_Flag = 0;
idata unsigned long int L4_Start     = 0;  /* L4闪烁开始时刻 */
 
/*
 * Bug-5修正：触发时刻由Temperature_Proc设置，Led_Proc不得重置
 */
idata bit           L2_Active = 0;         /* L2是否处于激活状态 */
idata bit           L3_Active = 0;
idata unsigned long int L2_Start = 0;      /* L2触发时刻 */
idata unsigned long int L3_Start = 0;
 
/* DAC数字量0~255，默认100 */
idata unsigned char DA_Out  = 100;
/* Bug-6修正：控制L1和DAC逻辑，0=温度控制, 1=手动控制 */
idata bit           DA_Mode = 0;
 
/* 0=解锁（L8亮，按键可控继电器）  1=锁定（L8灭，按键无效）*/
idata bit Jidian_Flag = 0;
 
/* Bug-4修正：S13按下时刻，用于松开时判断时长 */
idata unsigned long int S13_Down_Tick = 0;
 
 
/* =========================================================
 * Key_Proc：按键处理，每10ms调用（满足≤0.1s响应）
 * ========================================================= */
void Key_Proc(void)
{
    Key_Val  = Key_Read();
    Key_Down = Key_Val  & (Key_Val  ^ Key_Old);
    Key_Up   = ~Key_Val & (Key_Val  ^ Key_Old);
    Key_Old  = Key_Val;
 
    switch(Key_Down)
    {
        /*
         * S12：界面切换
         * Bug-13修正：离开校准值界面时将编辑值写入生效值
         *             进入校准值界面时将生效值同步给编辑值
         */
        case 12:
            if(Seg_Show_Mode == 2)
                Temperature_Jiaozhun = Temperature_Jiaozhun_Edit; /* 退出时生效 */
            Seg_Show_Mode = (Seg_Show_Mode + 1) % 3;
            if(Seg_Show_Mode == 2)
                Temperature_Jiaozhun_Edit = Temperature_Jiaozhun; /* 进入时同步 */
        break;
 
        /*
         * S16："加"按键
         * Bug-2修正：DA_Out>=250时封顶255（原判断unsigned>255永远假）
         * Bug-15修正：去掉Temperature_Flag==0条件
         */
        case 16:
            if(Seg_Show_Mode == 1)
            {
                DA_Out = (DA_Out >= 250) ? 255 : (DA_Out + 5);
            }
            else if(Seg_Show_Mode == 2)
            {
                if(Temperature_Jiaozhun_Edit < 9)
                    Temperature_Jiaozhun_Edit++;
            }
            else if(Seg_Show_Mode == 0 && Jidian_Flag == 0)
            {
                Relay(1);  /* 解锁状态下打开继电器 */
            }
        break;
 
        /*
         * S17："减"按键
         * Bug-2修正：DA_Out<5时封底0（原判断unsigned<0永远假）
         */
        case 17:
            if(Seg_Show_Mode == 1)
            {
                DA_Out = (DA_Out < 5) ? 0 : (DA_Out - 5);
            }
            else if(Seg_Show_Mode == 2)
            {
                if(Temperature_Jiaozhun_Edit > -9)
                    Temperature_Jiaozhun_Edit--;
            }
            else if(Seg_Show_Mode == 0 && Jidian_Flag == 0)
            {
                Relay(0);  /* 解锁状态下关闭继电器 */
            }
        break;
 
        /*
         * S13按下：仅记录时刻，不立即执行功能
         * Bug-4修正：功能在Key_Up时根据时长判断
         */
        case 13:
            S13_Down_Tick = uwTick;
        break;
    }
 
    /*
     * S13松开：根据按住时长判断长/短按
     * Bug-4修正：必须在Key_Up事件处判断，不能在Key_Down处判断
     * 短按(<1500ms)：切换DAC控制模式（温度控制?手动控制）
     * 长按(≥1500ms)：切换继电器锁定状态（锁定?解锁）
     */
    if(Key_Up == 13)
    {
        if(uwTick - S13_Down_Tick >= 1500)
            Jidian_Flag ^= 1;  /* 长按：切换锁定状态 */
        else
            DA_Mode ^= 1;      /* 短按：切换DAC控制模式 */
    }
}
 
 
/* =========================================================
 * Seg_Proc：更新数码管缓冲区，每20ms调用
 * ========================================================= */
void Seg_Proc(void)
{
    unsigned char i, h, t, o;
    signed char   cal;
 
    for(i = 0; i < 8; i++)
        Seg_Buf[i] = 10;  /* 先全部熄灭 */
 
    /*
     * Bug-1修正：switch(Seg_Show_Mode)，不是switch(Seg_Show_Mode==0)
     * 原写法比较结果只有0/1，三个界面无法全部区分
     */
    switch(Seg_Show_Mode)
    {
        /* 温度界面：C [5位熄灭] [十位] [个位] */
        case 0:
            Seg_Buf[0] = 11;  /* 'C' */
            Seg_Buf[6] = (Temperature / 10 == 0) ? 10 : (Temperature / 10);
            Seg_Buf[7] = Temperature % 10;
        break;
 
        /* DAC输出控制界面：A [4位熄灭] [百位] [十位] [个位]（高位0熄灭）*/
        case 1:
            Seg_Buf[0] = 12;  /* 'A' */
            h = DA_Out / 100;
            t = DA_Out / 10 % 10;
            o = DA_Out % 10;
            Seg_Buf[5] = (h == 0) ? 10 : h;
            Seg_Buf[6] = (h == 0 && t == 0) ? 10 : t;
            Seg_Buf[7] = o;
        break;
 
        /* 校准值界面：P [5/6位熄灭] [-号] [绝对值] */
        case 2:
            Seg_Buf[0] = 13;  /* 'P' */
            cal = Temperature_Jiaozhun_Edit;
            if(cal >= 0)
            {
                Seg_Buf[7] = (unsigned char)cal;
            }
            else
            {
                Seg_Buf[6] = 14;                    /* '-' */
                Seg_Buf[7] = (unsigned char)(-cal);
            }
        break;
    }
}
 
 
/* =========================================================
 * Led_Proc：刷新LED指示灯，每1ms调用
 * ========================================================= */
void Led_Proc(void)
{
    unsigned char j;
    for(j = 0; j < 8; j++) ucled[j] = 0;
 
    /* L1：温度控制模式下点亮（Bug-6修正：与DA_Mode绑定）*/
    ucled[0] = (DA_Mode == 0) ? 1 : 0;
 
    /* L2：温度上升时触发，持续2s（Bug-5修正：L2_Start不在此处重置）*/
    if(L2_Active)
    {
        if(uwTick - L2_Start <= 2000)
            ucled[1] = 1;
        else
            L2_Active = 0;  /* 2s到期自动熄灭 */
    }
 
    /* L3：温度下降时触发，持续2s */
    if(L3_Active)
    {
        if(uwTick - L3_Start <= 2000)
            ucled[2] = 1;
        else
            L3_Active = 0;
    }
 
    /*
     * L4：温度突变报警，0.2s间隔闪烁，持续3s后熄灭
     * Bug-5修正：L4_Start由Temperature_Proc设置，不在此重置
     * Bug-8修正：Temperature_Flag在此处3s后清零（不是XOR翻转）
     * 闪烁：(uwTick/200)%2，每200ms取反一次
     */
    if(Temperature_Flag)
    {
        if(uwTick - L4_Start <= 3000)
            ucled[3] = (unsigned char)((uwTick / 200) % 2);
        else
        {
            Temperature_Flag = 0;  /* 3s到期，清除报警标志 */
            ucled[3] = 0;
        }
    }
 
    /* L8：解锁状态下点亮（Bug-7修正：原代码完全缺失此逻辑）*/
    ucled[7] = (Jidian_Flag == 0) ? 1 : 0;
 
    Led_Disp(ucled);
}
 
 
/* =========================================================
 * Temperature_Proc：温度采集与处理，每500ms调用
 * Bug-14修正：原代码完全未加入调度器，温度永远不被采集
 * ========================================================= */
void Temperature_Proc(void)
{
    unsigned char raw;
    signed int    cal_result;
    unsigned char diff;
    unsigned char da_val;
 
    /* Bug-9修正：先保存旧值，再读新值（原代码顺序错误）*/
    Temperature_Old = Temperature;
 
    /* Bug-10修正：读原始温度并加入校准值 */
    raw = rd_temperature();
    cal_result = (signed int)raw + (signed int)Temperature_Jiaozhun;
    if(cal_result < 0)  cal_result = 0;
    if(cal_result > 99) cal_result = 99;
    Temperature = (unsigned char)cal_result;
 
    /* 检测温度上升/下降，触发L2/L3 */
    if(Temperature > Temperature_Old)
    {
        L2_Active = 1;
        L2_Start  = uwTick;
    }
    else if(Temperature < Temperature_Old)
    {
        L3_Active = 1;
        L3_Start  = uwTick;
    }
 
    /* 检测突变（差值>1℃），触发L4报警（Bug-8修正：直接置1）*/
    diff = (Temperature > Temperature_Old) ?
           (Temperature - Temperature_Old) :
           (Temperature_Old - Temperature);
    if(diff > 1)
    {
        Temperature_Flag = 1;  /* 直接置1，不用XOR */
        L4_Start = uwTick;
    }
 
    /*
     * 计算并输出DAC
     * Bug-0修正：Da_Write（不是DA_Write，大小写不同导致编译失败！）
     * Bug-11修正：Da_Write()无返回值，不能用其返回值
     * Bug-12修正：精确插值公式 102 + (T-10)*153/30
     *   推导：2V对应102，5V对应255，差值153；温度区间30℃
     *   用unsigned int中间变量防止乘法溢出（最大30*153=4590<65535）
     */
    if(DA_Mode == 0)
    {
        if(Temperature <= 10)
            da_val = 102;
        else if(Temperature >= 40)
            da_val = 255;
        else
            da_val = (unsigned char)(102 +
                     (unsigned int)(Temperature - 10) * 153 / 30);
 
        DA_Out = da_val;
        Da_Write(da_val);  /* Bug-0：正确函数名 Da_Write */
    }
    else
    {
        Da_Write(DA_Out);  /* 手动模式：直接输出当前数字量 */
    }
}
 
 
/* =========================================================
 * Timer1初始化：产生1ms定时中断
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
 * Timer1中断服务程序：每1ms执行
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
    { Led_Proc,          1,   0 },  /* 1ms：LED刷新，保证L4闪烁精度 */
    { Key_Proc,         10,   0 },  /* 10ms：按键扫描 */
    { Seg_Proc,         20,   0 },  /* 20ms：数码管内容更新 */
    { Temperature_Proc, 500,  0 },  /* 500ms：温度采集（Bug-14修正：原完全缺失）*/
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
 * 上电初始状态（题目第四节）：
 *   温度界面        Seg_Show_Mode=0 ?
 *   继电器解锁      Jidian_Flag=0   ? (L8亮)
 *   DAC温度控制     DA_Mode=0       ? (L1亮)
 *   DAC数字量100    DA_Out=100      ?
 *   温度校准值0     Jiaozhun=0      ?
 * ========================================================= */
void main(void)
{
    System_Init();
     Timer1_Init();  /* 最后开中断，避免打乱初始化 */
    Scheduler_Init();
 
 
    while(1)
    {
        Scheduler_Run();

    }
}
 