#include "init.h"
#include "key.h"
#include "led.h"
#include "seg.h"
#include "ds1302.h"
#include "intrins.h"
#include "stdio.h"
#include "string.h"
#include "onewire.h"


idata unsigned char Key_Val = 0, Key_Old = 0, Key_Up = 0, Key_Down = 0;


pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata unsigned char Seg_Pos = 0;


pdata unsigned char ucled[8] = {0, 0, 0, 0, 0, 0, 0, 0};


	idata unsigned long int uwTick = 0;


pdata unsigned char ucRtc[3] = {23, 59, 50};  // ? ????23:59:50
idata unsigned char Seg_Show_Mode = 0;  // 0-?? 1-?? 2-????
idata bit Seg_Light_Flag = 0;
idata unsigned char Time_Set = 0;  // 0-???? 1-???? 2-????
idata unsigned char ucRtc_Index = 0;  // 0-? 1-? 2-?
idata bit Alarm_Flag = 0;
idata unsigned int Time_1000Ms = 0;
pdata unsigned char Alarm_Time[3] = {0, 0, 0};  // ? ??????00:00:00

// ????????
idata unsigned long int Alarm_Start_Time = 0;
idata unsigned long int Led_Time = 0;

void Key_Proc()
{
    Key_Val = Key_Read();
    Key_Down = Key_Val & (Key_Val ^ Key_Old);
    Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
    Key_Old = Key_Val;
    
    // ? ???????,??????
    if(Seg_Show_Mode == 2)
    {
        if(Key_Down)
        {
            Alarm_Flag = 0;
            Seg_Show_Mode = 0;
            ucled[0] = 0;
        }
        return;
    }
    
    // ? S7 - ????
    if(Key_Down == 7)
    {
        if(Time_Set == 0)
        {
            Time_Set = 1;  // ??????
            ucRtc_Index = 0;
        }
        else if(Time_Set == 1)
        {
            ucRtc_Index++;
            if(ucRtc_Index > 2)
            {
                Time_Set = 0;  // ??????
                ucRtc_Index = 0;
            }
        }
    }
    
    // ? S6 - ????
    if(Key_Down == 6)
    {
        if(Time_Set == 0)
        {
            Time_Set = 2;  // ??????
            ucRtc_Index = 0;
        }
        else if(Time_Set == 2)
        {
            ucRtc_Index++;
            if(ucRtc_Index > 2)
            {
                Time_Set = 0;  // ??????
                ucRtc_Index = 0;
            }
        }
    }
    
    // ? S5 - ?
    if(Key_Down == 5)
    {
        if(Time_Set == 1)  // ????
        {
            if(ucRtc_Index == 0)
            {
                ucRtc[0]++;
                if(ucRtc[0] >= 24) ucRtc[0] = 0;
            }
            else if(ucRtc_Index == 1)
            {
                ucRtc[1]++;
                if(ucRtc[1] >= 60) ucRtc[1] = 0;
            }
            else if(ucRtc_Index == 2)
            {
                ucRtc[2]++;
                if(ucRtc[2] >= 60) ucRtc[2] = 0;
            }
        }
        else if(Time_Set == 2)  // ????
        {
            if(ucRtc_Index == 0)
            {
                Alarm_Time[0]++;
                if(Alarm_Time[0] >= 24) Alarm_Time[0] = 0;
            }
            else if(ucRtc_Index == 1)
            {
                Alarm_Time[1]++;
                if(Alarm_Time[1] >= 60) Alarm_Time[1] = 0;
            }
            else if(ucRtc_Index == 2)
            {
                Alarm_Time[2]++;
                if(Alarm_Time[2] >= 60) Alarm_Time[2] = 0;
            }
        }
    }
    
    // ? S4 - ? / ????
    if(Key_Val == 4 && Time_Set == 0)
    {
        Seg_Show_Mode = 1;  // ????
    }
    else if(Key_Up == 4 && Time_Set == 0)
    {
        Seg_Show_Mode = 0;  // ??????
    }
    else if(Key_Down == 4)
    {
        if(Time_Set == 1)  // ????
        {
            if(ucRtc_Index == 0)
            {
                if(ucRtc[0] == 0) ucRtc[0] = 23;
                else ucRtc[0]--;
            }
            else if(ucRtc_Index == 1)
            {
                if(ucRtc[1] == 0) ucRtc[1] = 59;
                else ucRtc[1]--;
            }
            else if(ucRtc_Index == 2)
            {
                if(ucRtc[2] == 0) ucRtc[2] = 59;
                else ucRtc[2]--;
            }
        }
        else if(Time_Set == 2)  // ????
        {
            if(ucRtc_Index == 0)
            {
                if(Alarm_Time[0] == 0) Alarm_Time[0] = 23;
                else Alarm_Time[0]--;
            }
            else if(ucRtc_Index == 1)
            {
                if(Alarm_Time[1] == 0) Alarm_Time[1] = 59;
                else Alarm_Time[1]--;
            }
            else if(ucRtc_Index == 2)
            {
                if(Alarm_Time[2] == 0) Alarm_Time[2] = 59;
                else Alarm_Time[2]--;
            }
        }
    }
}

void Seg_Proc()
{
    unsigned char i;
    
    // ? ????
    if(Seg_Show_Mode == 0 && Time_Set == 0)
    {
        Seg_Buf[0] = ucRtc[0] / 10;
        Seg_Buf[1] = ucRtc[0] % 10;
        Seg_Buf[2] = 11;  // ??? "-"
        Seg_Buf[3] = ucRtc[1] / 10;
        Seg_Buf[4] = ucRtc[1] % 10;
        Seg_Buf[5] = 11;  // ??? "-"
        Seg_Buf[6] = ucRtc[2] / 10;
        Seg_Buf[7] = ucRtc[2] % 10;
    }
    // ? ????(???)
    else if(Time_Set == 1)
    {
        Seg_Buf[0] = ucRtc[0] / 10;
        Seg_Buf[1] = ucRtc[0] % 10;
        Seg_Buf[2] = 11;
        Seg_Buf[3] = ucRtc[1] / 10;
        Seg_Buf[4] = ucRtc[1] % 10;
        Seg_Buf[5] = 11;
        Seg_Buf[6] = ucRtc[2] / 10;
        Seg_Buf[7] = ucRtc[2] % 10;
        
        // ?????????
        if(Seg_Light_Flag == 0)
        {
            if(ucRtc_Index == 0)
            {
                Seg_Buf[0] = 10;
                Seg_Buf[1] = 10;
            }
            else if(ucRtc_Index == 1)
            {
                Seg_Buf[3] = 10;
                Seg_Buf[4] = 10;
            }
            else if(ucRtc_Index == 2)
            {
                Seg_Buf[6] = 10;
                Seg_Buf[7] = 10;
            }
        }
    }
    // ? ????(???)
    else if(Time_Set == 2)
    {
        Seg_Buf[0] = Alarm_Time[0] / 10;
        Seg_Buf[1] = Alarm_Time[0] % 10;
        Seg_Buf[2] = 11;
        Seg_Buf[3] = Alarm_Time[1] / 10;
        Seg_Buf[4] = Alarm_Time[1] % 10;
        Seg_Buf[5] = 11;
        Seg_Buf[6] = Alarm_Time[2] / 10;
        Seg_Buf[7] = Alarm_Time[2] % 10;
        
        // ?????????
        if(Seg_Light_Flag == 0)
        {
            if(ucRtc_Index == 0)
            {
                Seg_Buf[0] = 10;
                Seg_Buf[1] = 10;
            }
            else if(ucRtc_Index == 1)
            {
                Seg_Buf[3] = 10;
                Seg_Buf[4] = 10;
            }
            else if(ucRtc_Index == 2)
            {
                Seg_Buf[6] = 10;
                Seg_Buf[7] = 10;
            }
        }
    }
    // ? ????
    else if(Seg_Show_Mode == 1)
    {
        idata float temp_f;
        idata unsigned char temp_int;
        
        temp_f = rd_temperature();
        temp_int = (unsigned char)temp_f;
        
        Seg_Buf[0] = 10;  // ??
        Seg_Buf[1] = 10;
        Seg_Buf[2] = 10;
        Seg_Buf[3] = 10;
        Seg_Buf[4] = 10;
        Seg_Buf[5] = temp_int / 10;
        Seg_Buf[6] = temp_int % 10;
        Seg_Buf[7] = 12;  // C
    }
    // ? ????(????)
    else if(Seg_Show_Mode == 2)
    {
        Seg_Buf[0] = ucRtc[0] / 10;
        Seg_Buf[1] = ucRtc[0] % 10;
        Seg_Buf[2] = 11;
        Seg_Buf[3] = ucRtc[1] / 10;
        Seg_Buf[4] = ucRtc[1] % 10;
        Seg_Buf[5] = 11;
        Seg_Buf[6] = ucRtc[2] / 10;
        Seg_Buf[7] = ucRtc[2] % 10;
    }
}

void Led_Proc()
{
    // ? ????:LED??0.2?,??5?
    if(Alarm_Flag)
    {
        // ??????5?
        if(uwTick - Alarm_Start_Time >= 5000)
        {
            Alarm_Flag = 0;
            Seg_Show_Mode = 0;
            ucled[0] = 0;
        }
        else
        {
            // 0.2???
            if(uwTick - Led_Time >= 200)
            {
                Led_Time = uwTick;
                ucled[0] ^= 0x01;
            }
        }
    }
    else
    {
        ucled[0] = 0;
    }
    
    Led_Disp(ucled);
}

void ucRtc_Proc()
{
    // ? ????
    if(Alarm_Flag == 0 && ucRtc[0] == Alarm_Time[0] && 
       ucRtc[1] == Alarm_Time[1] && ucRtc[2] == Alarm_Time[2])
    {
        Alarm_Flag = 1;
        Seg_Show_Mode = 2;
        Alarm_Start_Time = uwTick;
        Led_Time = uwTick;
    }
    
    // ? ????(????????)
    if(Time_Set == 0)
    {
        ucRtc[2]++;
        if(ucRtc[2] >= 60)
        {
            ucRtc[2] = 0;
            ucRtc[1]++;
        }
        if(ucRtc[1] >= 60)
        {
            ucRtc[1] = 0;
            ucRtc[0]++;
        }
        if(ucRtc[0] >= 24)
        {
            ucRtc[0] = 0;
        }
    }
}

void Timer1_Init(void)
{
    AUXR &= 0xBF;
    TMOD &= 0x0F;
    TL1 = 0x18;
    TH1 = 0xFC;
    TF1 = 0;
    TR1 = 1;
    ET1 = 1;
    EA = 1;
}

void Timer1_Isr(void) interrupt 3
{
    uwTick++;
    Seg_Pos = (++Seg_Pos) % 8;
    Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
    
    if(++Time_1000Ms >= 1000)
    {
        Time_1000Ms = 0;
        Seg_Light_Flag ^= 1;
    }
}

typedef struct
{
    void (*task_func)(void);
    unsigned long int rate_ms;
    unsigned long int last_ms;
} task_t;

idata task_t Scheduler_Task[] =
{
    {Led_Proc, 1, 0},
    {Key_Proc, 10, 0},
    {Seg_Proc, 20, 0},
    {ucRtc_Proc, 1000, 0},
};

idata unsigned char task_num;

void Scheduler_Init()
{
    task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}

void Scheduler_Run()
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

void main()
{
    System_Init();  // ? ?????????
    Scheduler_Init();
    Timer1_Init();
    
    while(1)
    {
        Scheduler_Run();
    }
}

