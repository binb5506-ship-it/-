#include "init.h" 
#include "led.h"
#include "key.h" 
#include "iic.h"
#include "intrins.h"
#include "ultrasound.h"
#include "stdio.h"
#include "string.h"
#include "seg.h" 

/* =========================================================
 * 全局变量定义
 * ========================================================= */
idata unsigned char  Key_Val = 0, Key_Down = 0, Key_Up = 0, Key_Old = 0;
idata unsigned char  Seg_Pos = 0;
pdata unsigned char  Seg_Buf[8] = {10,10,10,10,10,10,10,10};
pdata unsigned char  ucled[8]   = {0,0,0,0,0,0,0,0};
idata unsigned long  int uwTick = 0;

/* 显示模式：0=电压界面  1=测距界面  2=参数界面 */
idata unsigned char Seg_Show_Mode = 0;

/* 参数选择：0=上限  1=下限 */
idata bit Type = 0;

/*
 * 参数以×10整数存储，例：4.5V存为45，0.5V存为5
 * Type_Shang/Xia  : 参数界面正在编辑的工作值（未生效）
 * Active_Shang/Xia: 已生效的参数（退出参数界面时从工作值更新）
 */
idata unsigned char Type_Shang   = 45;  /* 初始上限 4.5V */
idata unsigned char Type_Xia     = 5;   /* 初始下限 0.5V */
idata unsigned char Active_Shang = 45;
idata unsigned char Active_Xia   = 5;

/* 测距相关 */
idata bit           Ceju_Flag = 0;          /* 0=未启动  1=启动 */
idata unsigned int  Distance  = 0;          /* 距离，单位cm */

/* AD电压，单位0.01V（例：245表示2.45V），范围0~500 */
idata unsigned int  AD_Voltage = 0;

/* L8闪烁 */
idata unsigned long int led_cnt = 0;
idata bit               L8_flag = 0;


/* =========================================================
 * 按键处理
 * ========================================================= */
void Key_Proc(void)
{
    Key_Val  = Key_Read();
    Key_Down = Key_Val  & (Key_Val  ^ Key_Old);
    Key_Up   = ~Key_Val & (Key_Val  ^ Key_Old);
    Key_Old  = Key_Val;

    /* S4：切换界面 */
    if(Key_Down == 4)
    {
        unsigned char old_mode = Seg_Show_Mode;
        Seg_Show_Mode = (Seg_Show_Mode + 1) % 3;

        /* 从参数界面退出时，工作值生效 */
        if(old_mode == 2)
        {
            Active_Shang = Type_Shang;
            Active_Xia   = Type_Xia;
        }

        /* 进入参数界面时，默认选上限，工作值初始化为当前生效值 */
        if(Seg_Show_Mode == 2)
        {
            Type         = 0;
            Type_Shang   = Active_Shang;
            Type_Xia     = Active_Xia;
        }
    }

    /* S5：切换上限/下限（仅参数界面有效） */
    if(Key_Down == 5 && Seg_Show_Mode == 2)
    {
        Type ^= 1;
    }

    /* S6、S7：加减参数（仅参数界面有效） */
    if(Seg_Show_Mode == 2)
    {
        if(Key_Down == 6)  /* 加0.5V */
        {
            if(Type == 0)
            {
                Type_Shang += 5;
                if(Type_Shang > 50) Type_Shang = 5;   /* 5.0V -> 0.5V 循环 */
            }
            else
            {
                Type_Xia += 5;
                if(Type_Xia > 50) Type_Xia = 5;
            }
        }

        if(Key_Down == 7)  /* 减0.5V */
        {
            if(Type == 0)
            {
                /* unsigned char下溢检测：减5后若大于200说明发生了下溢 */
                Type_Shang -= 5;
                if(Type_Shang < 5 || Type_Shang > 200) Type_Shang = 50;  /* 0.5V -> 5.0V 循环 */
            }
            else
            {
                Type_Xia -= 5;
                if(Type_Xia < 5 || Type_Xia > 200) Type_Xia = 50;
            }
        }
    }
}


/* =========================================================
 * 数码管显示处理
 * ========================================================= */
void Seg_Proc(void)
{
    unsigned char h, t, u;

    if(Seg_Show_Mode == 0)  /* 电压界面：U _ _ _ _ X.XX */
    {
        Seg_Buf[0] = 11;  /* 'U' */
        Seg_Buf[1] = 10;  /* 熄灭 */
        Seg_Buf[2] = 10;
        Seg_Buf[3] = 10;
        Seg_Buf[4] = 10;
        /* +','(44) 表示该位带小数点，中断里判断>20则减','后显示并开小数点 */
        Seg_Buf[5] = AD_Voltage / 100 % 10 + ',';  /* 个位（整数部分）带小数点 */
        Seg_Buf[6] = AD_Voltage / 10  % 10;        /* 十分位 */
        Seg_Buf[7] = AD_Voltage       % 10;        /* 百分位 */
    }
    else if(Seg_Show_Mode == 1)  /* 测距界面：L _ _ _ _ XXX 或 AAA */
    {
        Seg_Buf[0] = 13;  /* 'L' */
        Seg_Buf[1] = 10;
        Seg_Buf[2] = 10;
        Seg_Buf[3] = 10;
        Seg_Buf[4] = 10;

        if(Ceju_Flag == 0)  /* 未启动：显示 AAA */
        {
            Seg_Buf[5] = 14;  /* 'A' */
            Seg_Buf[6] = 14;
            Seg_Buf[7] = 14;
        }
        else  /* 连续测量：显示距离，高位消零 */
        {
            h = Distance / 100 % 10;
            t = Distance / 10  % 10;
            u = Distance       % 10;

            Seg_Buf[5] = (h == 0)              ? 10 : h;  /* 百位，0则熄灭 */
            Seg_Buf[6] = (h == 0 && t == 0)   ? 10 : t;  /* 十位，前两位都0则熄灭 */
            Seg_Buf[7] = u;                                /* 个位始终显示 */
        }
    }
    else if(Seg_Show_Mode == 2)  /* 参数界面：P _ _ X.X _ X.X */
    {
        Seg_Buf[0] = 12;  /* 'P' */
        Seg_Buf[1] = 10;
        Seg_Buf[2] = 10;
        /* 显示正在编辑的工作值（未生效），格式X.X */
        Seg_Buf[3] = Type_Shang / 10 + ',';  /* 上限整数位带小数点 */
        Seg_Buf[4] = Type_Shang % 10;        /* 上限小数位 */
        Seg_Buf[5] = 10;
        Seg_Buf[6] = Type_Xia / 10 + ',';   /* 下限整数位带小数点 */
        Seg_Buf[7] = Type_Xia % 10;         /* 下限小数位（无小数点） */
    }
}


/* =========================================================
 * LED指示灯处理
 * ========================================================= */
void Led_Proc(void)
{
    unsigned char i;
    for(i = 0; i < 8; i++)
        ucled[i] = 0;

    /* 界面指示灯 L1/L2/L3 */
    if     (Seg_Show_Mode == 0) ucled[0] = 1;  /* L1：电压界面 */
    else if(Seg_Show_Mode == 1) ucled[1] = 1;  /* L2：测距界面 */
    else if(Seg_Show_Mode == 2) ucled[2] = 1;  /* L3：参数界面 */

    /* L8：测距启动时0.1s间隔闪烁，停止时熄灭 */
    if(Ceju_Flag == 1)
    {
        if(uwTick - led_cnt >= 100)  /* 100ms */
        {
            led_cnt = uwTick;
            L8_flag = !L8_flag;
        }
    }
    else
    {
        L8_flag = 0;  /* 停止测距时立即熄灭 */
    }

    if(L8_flag == 1)
        ucled[7] = 1;

    Led_Disp(ucled);
}


/* =========================================================
 * AD采样 + Ceju_Flag更新 + DA输出
 * ========================================================= */
void Da_Proc(void)
{
    unsigned char i;
    unsigned long sum = 0;
    unsigned int new_voltage;
    unsigned long da_val;

    // 先读一次丢掉（PCF8591特性，第一次是上次的值）
    Ad_Read(0x03);
    
    // 连续读4次取平均
    for(i = 0; i < 8; i++)
    {
        sum += Ad_Read(0x03);
    }
    AD_Voltage = (unsigned int)(sum * 500UL / 8UL / 255UL);

    // 判断是否满足测距启动条件
    if(   AD_Voltage > (unsigned int)Active_Xia   * 10
       && AD_Voltage < (unsigned int)Active_Shang * 10)
    {
        Ceju_Flag = 1;
    }
    else
    {
        Ceju_Flag = 0;
    }

    // DA输出
    if(Ceju_Flag == 0)
    {
        Da_Write(0);
    }
    else
    {
        if(Distance <= 20)
        {
            da_val = 51;
        }
        else if(Distance >= 80)
        {
            da_val = 255;
        }
        else
        {
            da_val = 51 + (unsigned long)(Distance - 20) * 17UL / 5UL;
        }
        Da_Write((unsigned char)da_val);
    }
}


/* =========================================================
 * 超声波测距
 * ========================================================= */
void Ultra_Ceju(void)
{
    if(Ceju_Flag == 1)
    {
        Distance = Ut_Wave_Data();  /* 返回值单位：cm */
    }
}


/* =========================================================
 * 定时器1初始化（1ms中断，12T模式，12MHz）
 * ========================================================= */
void Timer1_Init(void)
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
 * Timer1中断服务：每1ms执行，负责数码管扫描
 * ========================================================= */
void Timer1_Isr(void) interrupt 3
{
    uwTick++;

    Seg_Pos = (++Seg_Pos) % 8;

    if(Seg_Buf[Seg_Pos] > 20)
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);  /* 带小数点 */
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);         /* 不带小数点 */
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
    {Led_Proc,   1,   0},
    {Key_Proc,   10,  0},
    {Seg_Proc,   20,  0},
    {Ultra_Ceju, 80,  0},
    {Da_Proc,    60,  0},
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
    Scheduler_Init();
    Timer1_Init();

    while(1)
    {
        Scheduler_Run();
    }
}