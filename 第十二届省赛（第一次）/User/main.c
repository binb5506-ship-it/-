#include "init.h" 
#include "key.h"
#include "led.h"
#include "seg.h"
#include "onewire.h"
#include "iic.h"
#include "intrins.h"

idata unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;
idata unsigned char Seg_Pos=0;
pdata unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
pdata unsigned char ucled[8]={0,0,0,0,0,0,0,0};
idata unsigned long int uwTick=0;
idata unsigned char Seg_Show_Mode=0;
idata unsigned long int Temp_100x=0; 
idata unsigned long int DA_Out_100x=0;
idata unsigned char wendu=25;       // 生效的温度参数（DAC使用这个）
idata unsigned char wendu_temp=25;  // 参数界面中临时调整用
idata bit Mode=0;

// ===================== 温度采集（独立任务，1秒一次）=====================
void Temp_Proc()
{
    Temp_100x = (unsigned long)(rd_temperature() * 100);
}

// ===================== 按键处理 =====================
void Key_Proc()
{
    Key_Val = Key_Read();
    Key_Down = Key_Val & (Key_Val ^ Key_Old);
    Key_Up   = ~Key_Val & (Key_Val ^ Key_Old);
    Key_Old  = Key_Val;

    switch(Key_Down)
    {
        case 4:
            // 离开参数界面时，临时参数才生效
            if(Seg_Show_Mode == 1)
                wendu = wendu_temp;
            
            Seg_Show_Mode = (++Seg_Show_Mode) % 3;
            
            // 进入参数界面时，同步当前生效参数到临时变量
            if(Seg_Show_Mode == 1)
                wendu_temp = wendu;
        break;
        
        case 5:
            Mode ^= 1;
        break;
    }

    // S8/S9 仅在参数界面有效，修改临时变量
    if(Seg_Show_Mode == 1)
    {
        switch(Key_Down)
        {
            case 8: // S8 = 减
                if(wendu_temp > 0) wendu_temp--;
            break;
            case 9: // S9 = 加
                if(wendu_temp < 99) wendu_temp++;
            break;
        }
    }
}

// ===================== 数码管内容 =====================
void Seg_Proc()
{
    if(Seg_Show_Mode == 0)
    {
        // 温度显示界面，标识C
        Seg_Buf[0] = 11; // C
        Seg_Buf[1] = 10;
        Seg_Buf[2] = 10;
        Seg_Buf[3] = 10;
        // 十位：为0则消隐
        Seg_Buf[4] = (Temp_100x/1000%10 == 0) ? 10 : Temp_100x/1000%10;
        // 个位+小数点：永远不消隐（0.xx时也要显示0.）
        Seg_Buf[5] = Temp_100x/100%10 + ',';
        Seg_Buf[6] = Temp_100x/10%10;
        Seg_Buf[7] = Temp_100x%10;
    }
    else if(Seg_Show_Mode == 1)
    {
        // 参数设置界面，标识P，显示临时参数
        Seg_Buf[0] = 12; // P
        Seg_Buf[1] = 10;
        Seg_Buf[2] = 10;
        Seg_Buf[3] = 10;
        Seg_Buf[4] = 10;
        Seg_Buf[5] = 10;
        Seg_Buf[6] = (wendu_temp/10%10 == 0) ? 10 : wendu_temp/10%10;
        Seg_Buf[7] = wendu_temp%10;
    }
    else if(Seg_Show_Mode == 2)
    {
        // DAC输出界面，标识A
        Seg_Buf[0] = 13; // A
        Seg_Buf[1] = 10;
        Seg_Buf[2] = 10;
        Seg_Buf[3] = 10;
        Seg_Buf[4] = 10;
        // 个位+小数点：固定显示（0.xx也要显示0.）
        Seg_Buf[5] = DA_Out_100x/100%10 + ',';
        Seg_Buf[6] = DA_Out_100x/10%10;
        Seg_Buf[7] = DA_Out_100x%10;
    }
}

// ===================== LED =====================
void Led_Proc()
{
    idata unsigned char i;
    for(i=0;i<8;i++) ucled[i]=0;

    if(Mode == 0)       ucled[0] = 1; // L1：模式1亮
    if(Seg_Show_Mode==0) ucled[1] = 1; // L2：温度界面
    if(Seg_Show_Mode==1) ucled[2] = 1; // L3：参数界面
    if(Seg_Show_Mode==2) ucled[3] = 1; // L4：DAC界面

    Led_Disp(ucled);
}

// ===================== DAC输出 =====================
void Da_Proc()
{
    unsigned char da_val = 0;
    unsigned int temp_int = (unsigned int)(Temp_100x / 100);

    if(Mode == 0)
    {
        // 模式1：温度<参数→0V，否则→5V
        da_val = (temp_int < wendu) ? 0 : 255;
    }
    else
    {
        // 模式2：20℃以下1V，40℃以上4V，中间线性
        if(temp_int <= 20)
            da_val = 51;
        else if(temp_int >= 40)
            da_val = 204;
        else
            da_val = (unsigned char)(51 + (unsigned int)(153UL * (temp_int - 20) / 20));
    }

    Da_Write(da_val);
    DA_Out_100x = (unsigned long)da_val * 500 / 255;
}

// ===================== 定时器1 1ms中断 =====================
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

void Timer1_Isr(void) interrupt 3
{
    uwTick++;
    Seg_Pos = (++Seg_Pos) % 8;
    if(Seg_Buf[Seg_Pos] > 20)
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos]-',', 1);
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
}

// ===================== 调度器 =====================
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
    {Da_Proc,    50,   0},
    {Temp_Proc,  1000, 0}, // 温度1秒读一次，独立出来
};

idata unsigned char task_num;

void Scheduler_Init()
{
    task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}

void Scheduler_Run()
{
    unsigned char i;
    for(i=0;i<task_num;i++)
    {
        unsigned long int now_Time = uwTick;
        if(now_Time >= (Scheduler_Task[i].rate_ms + Scheduler_Task[i].last_ms))
        {
            Scheduler_Task[i].last_ms = now_Time;
            Scheduler_Task[i].task_func();
        }
    }
}

void main()
{
    System_Init();
    Scheduler_Init();
    Timer1_Init();
    while(1)
    {
        Scheduler_Run();
    }
}