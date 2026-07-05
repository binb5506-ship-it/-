#include "init.h"
#include "led.h"
#include "Seg.h"

/* ---- 全局变量 ---- */
pdata unsigned char ucled[8]  = {0,0,0,0,0,0,0,0};  /* LED 状态数组，0=灭 1=亮 */
idata unsigned char Seg_Pos   = 0;                    /* 当前扫描的数码管位号(0~7) */
pdata unsigned char Seg_Buf[] = {10,10,10,10,10,10,10,10}; /* 数码管显示缓冲，10=熄灭 */
unsigned long int   uwTick    = 0;                    /* 系统毫秒计数，由 Timer1 ISR 累加 */


/* =========================================================
 * LED 处理函数（在调度器里按需调用）
 * ========================================================= */
void Led_Proc()
{
    Led_Disp(ucled);
}


/* =========================================================
 * Timer1 初始化 — 12T 模式，1ms 中断 @12.000MHz
 * ========================================================= */
void Timer1_Init(void)
{
    AUXR &= 0xBF;   /* 清 bit6：选 12T 模式（每 12 个时钟计一次） */
    TMOD &= 0x0F;   /* 清高 4 位：Timer1 工作在模式 0（16 位自动重装） */
    TL1   = 0x18;   /* 定时初值低字节 */
    TH1   = 0xFC;   /* 定时初值高字节，(65536-0xFC18)*12/12MHz = 1ms */
    TF1   = 0;      /* 清中断标志 */
    TR1   = 1;      /* 启动 Timer1 */
    ET1   = 1;      /* 允许 Timer1 中断 */
    EA    = 1;      /* 开总中断 */
}


/* =========================================================
 * Timer1 中断服务程序 — 每 1ms 执行一次
 * ========================================================= */
void Timer1_Isr(void) interrupt 3
{
    uwTick++;                           /* 系统时钟 +1ms */
    Seg_Pos = (Seg_Pos + 1) % 8;       /* 轮流扫描 8 位数码管 */

    if(Seg_Buf[Seg_Pos] > 20)
    {
        /* 缓冲值 >20 表示"带小数点"，实际段码索引 = 值 - ',' (44) */
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
    }
    else
    {
        /* 缓冲值 0~10：直接作为段码索引（0~9 为数字，10 为熄灭） */
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
    }
}


/* =========================================================
 * 调度器任务结构体
 *   task_func : 任务函数指针
 *   rate_ms   : 任务执行周期（ms）
 *   last_ms   : 上次执行时的 uwTick 值
 * ========================================================= */
typedef struct
{
    void (*task_func)(void);    /* 函数指针放第一位 */
    unsigned long int rate_ms;  /* 周期，用 long 防止溢出 */
    unsigned long int last_ms;  /* 上次运行时间，用 long 防止溢出 */
} task_t;                       /* 注意：是 task_t，不是 time_t */

idata task_t Scheduler_Task[] =
{
    /* { 函数名, 周期ms, 0 } — 根据题目在此填写各任务 */
};

idata unsigned char task_num;   /* 任务总数，由 Scheduler_Init 计算 */


/* =========================================================
 * 调度器初始化 — 自动计算任务数量
 * ========================================================= */
void Scheduler_Init(void)
{
    /* sizeof(数组) / sizeof(单个元素) = 元素个数 */
    task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}


/* =========================================================
 * 调度器运行 — 在 main 的 while(1) 中反复调用
 * ========================================================= */
void Scheduler_Run(void)
{
    unsigned char     i;
    unsigned long int now;

    for(i = 0; i < task_num; i++)
    {
        now = uwTick;   /* 读取当前时间（避免中断途中 uwTick 变化）*/

        if(now >= Scheduler_Task[i].last_ms + Scheduler_Task[i].rate_ms)
        {
            Scheduler_Task[i].last_ms = now;   /* 更新上次执行时间 */
            Scheduler_Task[i].task_func();     /* 调用任务函数（必须加括号！）*/
        }
    }
}


/* =========================================================
 * 主函数
 * ========================================================= */
void main(void)
{
    System_Init();      /* 硬件初始化：关 LED、关继电器 */
    Timer1_Init();      /* 启动 1ms 定时中断 */
    Scheduler_Init();   /* 计算任务数量 */

    while(1)
        Scheduler_Run();    /* 轮询调度器 */
}
