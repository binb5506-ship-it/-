/*==============================================================
 * 彩灯控制器 - CT107D单片机竞赛板
 * 功能: 4种LED流转模式 + 电位器亮度控制 + 按键设置 + E2PROM存储
 *==============================================================*/
#include "init.h"
#include "key.h"
#include "seg.h"
#include "led.h"
#include "iic.h"
#include "intrins.h"
#include "stdio.h"
#include "string.h"

/*--------------------------------------------------------------
 * 按键专用变量
 *--------------------------------------------------------------*/
idata unsigned char Key_Val, Key_Old, Key_Up, Key_Down;

/*--------------------------------------------------------------
 * 数码管显示缓冲区
 * 编码规则:
 *   0~9  => 显示数字0-9
 *   10   => 熄灭 (blank)
 *   11   => 显示"-"
 *--------------------------------------------------------------*/
pdata unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
idata unsigned char Seg_Pos    = 0;

/*--------------------------------------------------------------
 * LED显示数组: ucled[0]=L1(最左), ucled[7]=L8(最右)
 *              0=灭, 1=亮
 * ucled_off : 全灭参考数组，用于停止/熄灭时传给Led_Disp
 *--------------------------------------------------------------*/
pdata unsigned char ucled[8]     = {0,0,0,0,0,0,0,0};
pdata unsigned char ucled_off[8] = {0,0,0,0,0,0,0,0};

/*--------------------------------------------------------------
 * 系统滴答计时器 (Timer1每1ms中断自增)
 *--------------------------------------------------------------*/
idata unsigned long int uwTick;

/*--------------------------------------------------------------
 * LED运行控制
 *--------------------------------------------------------------*/
idata unsigned char     Led_Start  = 0;    /* LED启停: 0=停止, 1=运行 */
idata unsigned char     Run_Mode   = 1;    /* 当前运行模式: 1~4 */
/* [修改点1] Break_Time改为数组，每个模式独立存储间隔(ms)
 * 上电后从E2PROM加载，默认400ms                          */
idata unsigned int      Break_Time[4] = {400, 400, 400, 400};
idata unsigned char     Led_Step   = 0;    /* 当前模式内步骤索引(0起) */
idata unsigned long int Led_Last   = 0;    /* 上次步骤切换时刻 */

/*--------------------------------------------------------------
 * 亮度控制 (通过PCF8591读取RB2电位器)
 * [修改点2] 新增亮度相关变量
 *--------------------------------------------------------------*/
idata unsigned char     Brightness  = 4;  /* 亮度等级 1(暗)~4(亮) */
idata unsigned char     Pwm_Cnt     = 0;  /* PWM计数 0~3，每1ms+1 */
idata unsigned char     Show_Bright = 0;  /* 1=数码管显示亮度等级 */
idata unsigned long int Adc_Last    = 0;  /* ADC上次采样时刻 */

/*--------------------------------------------------------------
 * 设置界面状态
 * [修改点3] 新增设置状态变量
 *
 * Set_State = 0 : 正常运行
 * Set_State = 1 : 设置中，当前选中"运行模式编号"
 * Set_State = 2 : 设置中，当前选中"流转间隔"
 *--------------------------------------------------------------*/
idata unsigned char     Set_State  = 0;
idata unsigned char     Set_Mode   = 1;   /* 设置界面正在编辑的模式号(1~4) */
idata unsigned char     Blink_On   = 1;   /* 闪烁: 1=亮段, 0=灭段 */
idata unsigned long int Blink_Last = 0;   /* 闪烁计时基准 */

/*==============================================================
 * [修改点4] 新增 AT24C02 E2PROM 读写函数
 * I2C地址: 写=0xA0, 读=0xA1
 * 存储规划: 地址0~3 存模式1~4的间隔索引(0~8 => 400~1200ms)
 *==============================================================*/

/**
 * @brief 向AT24C02指定地址写入一个字节
 * @param addr  内部存储地址 0~255
 * @param dat   写入数据
 */
void AT24C02_Write(unsigned char addr, unsigned char dat)
{
    unsigned long int t;          /* [C89要求] 变量声明放在函数顶部 */
    IIC_Start();
    IIC_SendByte(0xA0);           /* 器件写地址 */
    IIC_WaitAck();
    IIC_SendByte(addr);           /* 字节地址 */
    IIC_WaitAck();
    IIC_SendByte(dat);            /* 写入数据 */
    IIC_WaitAck();
    IIC_Stop();
    /* 等待AT24C02内部写周期完成，最长约5ms */
    t = uwTick;
    while(uwTick - t < 5);
}

/**
 * @brief 从AT24C02指定地址读取一个字节
 * @param addr  内部存储地址
 * @return 读取的数据
 */
unsigned char AT24C02_Read(unsigned char addr)
{
    unsigned char dat;
    /* 写模式定位地址 */
    IIC_Start();
    IIC_SendByte(0xA0);
    IIC_WaitAck();
    IIC_SendByte(addr);
    IIC_WaitAck();
    /* 重复起始条件，切换为读模式 */
    IIC_Start();
    IIC_SendByte(0xA1);           /* 器件读地址 */
    IIC_WaitAck();
    dat = IIC_RecByte();
    IIC_SendAck(1);               /* NACK: 表示不再继续读 */
    IIC_Stop();
    return dat;
}

/**
 * @brief 上电时从E2PROM加载各模式流转间隔
 */
void E2PROM_Load(void)
{
    unsigned char i, val;
    for(i = 0; i < 4; i++)
    {
        val = AT24C02_Read(i);
        if(val <= 8)                              /* 有效范围: 0~8 */
            Break_Time[i] = 400 + (unsigned int)val * 100;
        else
            Break_Time[i] = 400;                  /* 无效数据用默认值 */
    }
}

/**
 * @brief 将各模式流转间隔保存到E2PROM（S6第三次按下时调用）
 */
void E2PROM_Save(void)
{
    unsigned char i;
    for(i = 0; i < 4; i++)
        AT24C02_Write(i, (unsigned char)((Break_Time[i] - 400) / 100));
}

/*==============================================================
 * [修改点5] 新增 PCF8591 ADC 读取函数
 * I2C地址: 写=0x90, 读=0x91
 * RB2电位器接 AIN0 (通道0)
 *==============================================================*/

/**
 * @brief 读取PCF8591指定通道的ADC值
 * @param ch  通道号 0~3
 * @return    0~255
 */
unsigned char PCF8591_Read(unsigned char ch)
{
    unsigned char dat;
    /* 发送控制字：选通道，启动本次转换 */
    IIC_Start();
    IIC_SendByte(0x90);
    IIC_WaitAck();
    IIC_SendByte(0x40 | (ch & 0x03));   /* 单端输入模式 + 通道选择 */
    IIC_WaitAck();
    IIC_Stop();
    /* 读取上一次转换结果（PCF8591流水线机制） */
    IIC_Start();
    IIC_SendByte(0x91);
    IIC_WaitAck();
    dat = IIC_RecByte();
    IIC_SendAck(1);                      /* NACK */
    IIC_Stop();
    return dat;
}

/*==============================================================
 * [修改点6] 新增 LED 模式图案数据（存放于代码区 code）
 *
 *  ucled[0]=L1(左) ~ ucled[7]=L8(右), 1=亮, 0=灭
 *  Mode_Steps: 各模式的步骤总数
 *
 *  模式1: L1→L2→...→L8, 8步, 每步单个LED从左到右
 *  模式2: L8→L7→...→L1, 8步, 每步单个LED从右到左
 *  模式3: (L1,L8)→(L2,L7)→(L3,L6)→(L4,L5), 4步对称收缩
 *  模式4: (L4,L5)→(L3,L6)→(L2,L7)→(L1,L8), 4步对称扩展
 *==============================================================*/
code unsigned char Mode_Steps[4] = {8, 8, 4, 4};

/* 模式1图案 */
code unsigned char Pat1[8][8] = {
    {1,0,0,0,0,0,0,0},  /* 步0: L1 */
    {0,1,0,0,0,0,0,0},  /* 步1: L2 */
    {0,0,1,0,0,0,0,0},  /* 步2: L3 */
    {0,0,0,1,0,0,0,0},  /* 步3: L4 */
    {0,0,0,0,1,0,0,0},  /* 步4: L5 */
    {0,0,0,0,0,1,0,0},  /* 步5: L6 */
    {0,0,0,0,0,0,1,0},  /* 步6: L7 */
    {0,0,0,0,0,0,0,1}   /* 步7: L8 */
};
/* 模式2图案 */
code unsigned char Pat2[8][8] = {
    {0,0,0,0,0,0,0,1},  /* 步0: L8 */
    {0,0,0,0,0,0,1,0},  /* 步1: L7 */
    {0,0,0,0,0,1,0,0},  /* 步2: L6 */
    {0,0,0,0,1,0,0,0},  /* 步3: L5 */
    {0,0,0,1,0,0,0,0},  /* 步4: L4 */
    {0,0,1,0,0,0,0,0},  /* 步5: L3 */
    {0,1,0,0,0,0,0,0},  /* 步6: L2 */
    {1,0,0,0,0,0,0,0}   /* 步7: L1 */
};
/* 模式3图案: 对称收缩 */
code unsigned char Pat3[4][8] = {
    {1,0,0,0,0,0,0,1},  /* 状态1: L1,L8 */
    {0,1,0,0,0,0,1,0},  /* 状态2: L2,L7 */
    {0,0,1,0,0,1,0,0},  /* 状态3: L3,L6 */
    {0,0,0,1,1,0,0,0}   /* 状态4: L4,L5 */
};
/* 模式4图案: 对称扩展（模式3的逆序） */
code unsigned char Pat4[4][8] = {
    {0,0,0,1,1,0,0,0},  /* 状态1: L4,L5 */
    {0,0,1,0,0,1,0,0},  /* 状态2: L3,L6 */
    {0,1,0,0,0,0,1,0},  /* 状态3: L2,L7 */
    {1,0,0,0,0,0,0,1}   /* 状态4: L1,L8 */
};

/**
 * @brief 根据 Run_Mode 和 Led_Step 刷新 ucled[] 数组
 */
void Update_LED_Pattern(void)
{
    unsigned char i;
    switch(Run_Mode)
    {
        case 1: for(i=0;i<8;i++) ucled[i]=Pat1[Led_Step][i]; break;
        case 2: for(i=0;i<8;i++) ucled[i]=Pat2[Led_Step][i]; break;
        case 3: for(i=0;i<8;i++) ucled[i]=Pat3[Led_Step][i]; break;
        case 4: for(i=0;i<8;i++) ucled[i]=Pat4[Led_Step][i]; break;
        default: for(i=0;i<8;i++) ucled[i]=0; break;
    }
}

/*==============================================================
 * [修改点7] 修复并完善 Key_Proc
 *==============================================================*/
void Key_Proc()
{
    Key_Val  = Key_Read();
    Key_Down = Key_Val & (Key_Val ^ Key_Old);   /* 按下沿 */
    Key_Up   = ~Key_Val & (Key_Val ^ Key_Old);  /* 释放沿 */
    Key_Old  = Key_Val;

    /*----------------------------------------------------------
     * S7: 启动/停止 LED 流转
     * [修改] 原来 Led_Show_Mode=(++Led_Show_Mode)%2 语义不清且
     *        在设置状态下也会响应，现在加 Set_State==0 保护，
     *        启动时同步重置计时基准
     *----------------------------------------------------------*/
    if(Key_Down == 7)
    {
        if(Set_State == 0)
        {
            Led_Start = !Led_Start;
            if(Led_Start)
                Led_Last = uwTick;  /* 启动时重置步骤计时，防止第一步瞬间跳过 */
        }
    }

    /*----------------------------------------------------------
     * S6: 进入/切换/退出 设置界面
     * [修改] 原代码 case 6 为空，现实现三段式状态机:
     *   ① 正常→设置(选模式编号)
     *   ② 模式编号→流转间隔
     *   ③ 流转间隔→保存并退出
     *----------------------------------------------------------*/
    if(Key_Down == 6)
    {
        if(Set_State == 0)
        {
            Set_State  = 1;         /* 进入设置，默认选中模式编号 */
            Set_Mode   = Run_Mode;  /* 从当前运行模式开始编辑 */
            Led_Start  = 0;         /* 进入设置后停止LED流转 */
            Blink_On   = 1;
            Blink_Last = uwTick;
        }
        else if(Set_State == 1)
        {
            Set_State  = 2;         /* 切换到选中流转间隔 */
            Blink_On   = 1;
            Blink_Last = uwTick;
        }
        else if(Set_State == 2)
        {
            Run_Mode  = Set_Mode;   /* 确认新模式号 */
            E2PROM_Save();          /* 保存到E2PROM */
            Set_State = 0;          /* 退出设置界面 */
            Led_Step  = 0;
        }
    }

    /*----------------------------------------------------------
     * S5: "加" 按键
     * [修改] 原代码 case 5 中 break 位置错误、else if 没有函数体
     *        现分三种情况处理，并加边界判断
     *----------------------------------------------------------*/
    if(Key_Down == 5)
    {
        if(Set_State == 1)                         /* 选中模式编号时: 模式+1 */
        {
            if(Set_Mode < 4)
                Set_Mode++;
        }
        else if(Set_State == 2)                    /* 选中流转间隔时: +100ms */
        {
            if(Break_Time[Set_Mode - 1] < 1200)
                Break_Time[Set_Mode - 1] += 100;
        }
        /* 非设置状态下 S5 无定义功能 */
    }

    /*----------------------------------------------------------
     * S4: "减" 按键 / 非设置状态下显示亮度等级
     * [修改] 原代码 case 4 完全缺失，现实现"减"功能和亮度显示
     *----------------------------------------------------------*/
    if(Key_Down == 4)
    {
        if(Set_State == 1)                         /* 选中模式编号时: 模式-1 */
        {
            if(Set_Mode > 1)
                Set_Mode--;
        }
        else if(Set_State == 2)                    /* 选中流转间隔时: -100ms */
        {
            if(Break_Time[Set_Mode - 1] > 400)
                Break_Time[Set_Mode - 1] -= 100;
        }
        else                                       /* 正常状态: 按下显示亮度 */
        {
            Show_Bright = 1;
        }
    }

    /* S4 释放: 正常状态下关闭亮度显示 */
    if(Key_Up == 4)
    {
        if(Set_State == 0)
            Show_Bright = 0;
    }
}

/*==============================================================
 * [修改点8] 完整实现 Seg_Proc（原函数体为空）
 *==============================================================*/
void Seg_Proc()
{
    unsigned char i;
    unsigned int  interval;

    /* 设置状态下每400ms切换一次闪烁，总周期0.8s */
    if(Set_State != 0)
    {
        if(uwTick - Blink_Last >= 400)
        {
            Blink_Last = uwTick;
            Blink_On   = !Blink_On;
        }
    }

    /*----------------------------------------------------
     * 优先级1: 按住S4时显示亮度等级
     * 格式: [熄灭×6] ["-"] [亮度1~4]
     * 示例亮度2: {10,10,10,10,10,10,11,2}
     *----------------------------------------------------*/
    if(Show_Bright)
    {
        for(i = 0; i < 6; i++) Seg_Buf[i] = 10;
        Seg_Buf[6] = 11;           /* "-" */
        Seg_Buf[7] = Brightness;   /* 1~4 */
        return;
    }

    /*----------------------------------------------------
     * 优先级2: 设置界面
     * 格式:
     *  位0      : 熄灭
     *  位1      : 运行模式编号  (Set_State==1 时以0.8s闪烁)
     *  位2~3    : 熄灭
     *  位4~7    : 流转间隔(ms) (Set_State==2 时以0.8s闪烁)
     *             400ms  => { 10, 4, 0, 0 }
     *             1200ms => {  1, 2, 0, 0 }
     *----------------------------------------------------*/
    if(Set_State != 0)
    {
        Seg_Buf[0] = 10;
        Seg_Buf[2] = 10;
        Seg_Buf[3] = 10;

        /* 模式编号：Set_State==1 时闪烁 */
        Seg_Buf[1] = (Set_State == 1 && !Blink_On) ? 10 : Set_Mode;

        /* 流转间隔：Set_State==2 时闪烁 */
        interval = Break_Time[Set_Mode - 1];
        if(Set_State == 2 && !Blink_On)
        {
            Seg_Buf[4] = Seg_Buf[5] = Seg_Buf[6] = Seg_Buf[7] = 10;
        }
        else
        {
            /* 千位：仅≥1000ms时显示 */
            Seg_Buf[4] = (interval >= 1000) ? (interval / 1000)     : 10;
            Seg_Buf[5] = (interval % 1000) / 100;
            Seg_Buf[6] = (interval % 100)  / 10;
            Seg_Buf[7] =  interval % 10;
        }
        return;
    }

    /*----------------------------------------------------
     * 正常运行状态: 数码管全部熄灭（题目3.2节要求）
     *----------------------------------------------------*/
    for(i = 0; i < 8; i++)
        Seg_Buf[i] = 10;
}

/*==============================================================
 * [修改点9] 完整实现 Led_Proc（原只有停止处理）
 *==============================================================*/
void Led_Proc()
{
    unsigned char i;
    unsigned char adc;  /* [C89] 局部变量声明统一放顶部 */

    /*----------------------------------------------------------
     * 每20ms采样一次RB2电位器(AIN0)，更新亮度等级
     * 0~255 均匀分为4档:
     *   0~63   => 1级(25%亮)
     *   64~127 => 2级(50%亮)
     *   128~191=> 3级(75%亮)
     *   192~255=> 4级(100%亮)
     *----------------------------------------------------------*/
    if(uwTick - Adc_Last >= 20)
    {
        Adc_Last = uwTick;
        adc = PCF8591_Read(0);
        if     (adc < 64)  Brightness = 1;
        else if(adc < 128) Brightness = 2;
        else if(adc < 192) Brightness = 3;
        else               Brightness = 4;
    }

    /*----------------------------------------------------------
     * PWM计数: 每次Led_Proc被调用(每1ms)加1, 0→1→2→3→0→...
     * 当 Pwm_Cnt < Brightness 时亮灯, 否则灭灯
     * 效果: 亮度1=25%, 亮度2=50%, 亮度3=75%, 亮度4=100%
     *----------------------------------------------------------*/
    Pwm_Cnt = (Pwm_Cnt + 1) % 4;

    /* 停止状态或设置界面: 全部LED熄灭 */
    if(!Led_Start || Set_State != 0)
    {
        Led_Disp(ucled_off);
        return;
    }

    /*----------------------------------------------------------
     * 运行状态: 检查是否达到切换步骤的时刻
     * 使用 Run_Mode-1 作为 Break_Time 下标，确保用当前模式间隔
     *----------------------------------------------------------*/
    if(uwTick - Led_Last >= (unsigned long int)Break_Time[Run_Mode - 1])
    {
        Led_Last = uwTick;
        Led_Step++;

        /* 当前模式所有步骤完成 → 切换到下一模式(1→2→3→4→1) */
        if(Led_Step >= Mode_Steps[Run_Mode - 1])
        {
            Led_Step = 0;
            Run_Mode = (Run_Mode % 4) + 1;
        }
    }

    /* 根据当前模式和步骤更新 ucled[] */
    Update_LED_Pattern();

    /* PWM输出: Pwm_Cnt < Brightness 时才实际点亮 */
    if(Pwm_Cnt < Brightness)
        Led_Disp(ucled);
    else
        Led_Disp(ucled_off);
}

/*==============================================================
 * 定时器初始化 (保持不变)
 *==============================================================*/
void Timer0_Init(void)      /* 1ms @12.000MHz, 12T */
{
    AUXR &= 0x7F;
    TMOD &= 0xF0;
    TL0   = 0x18;
    TH0   = 0xFC;
    TF0   = 0;
    TR0   = 1;
    /* Timer0 本程序仅初始化，不启用中断 */
}

void Timer1_Init(void)      /* 1ms @12.000MHz, 12T */
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

/*==============================================================
 * Timer1中断: 1ms系统滴答 + 数码管动态扫描 (保持不变)
 *==============================================================*/
void Timer1_Isr(void) interrupt 3
{
    uwTick++;                           /* 系统滴答 +1ms */

    Seg_Pos = (++Seg_Pos) % 8;         /* 切换扫描位 */

    /* Seg_Buf[x]>20: 减','(44)后显示并带小数点(保留原有逻辑) */
    if(Seg_Buf[Seg_Pos] > 20)
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
}

/*==============================================================
 * 任务调度器
 *==============================================================*/
typedef struct
{
    void (*task_func)(void);
    unsigned long int rate_ms;
    unsigned long int last_ms;
} task_t;

idata task_t Scheduler_Task[] =
{
    {Led_Proc, 1,  0},   /* LED刷新+PWM亮度: 每1ms  */
    {Key_Proc, 10, 0},   /* 按键扫描:         每10ms */
    {Seg_Proc, 20, 0},   /* 数码管逻辑:       每20ms */
};

idata unsigned char task_num;

void Scheduler_Init(void)
{
    task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}

void Scheduler_Run(void)
{
    unsigned char     i;
    unsigned long int now;          /* [修改点10] C89要求声明提到顶部 */

    for(i = 0; i < task_num; i++)
    {
        now = uwTick;
        if(now >= Scheduler_Task[i].rate_ms + Scheduler_Task[i].last_ms)
        {
            Scheduler_Task[i].last_ms = now;
            Scheduler_Task[i].task_func();
        }
    }
}

/*==============================================================
 * 主函数
 *==============================================================*/
void main(void)
{
    System_Init();       /* 系统初始化: 关闭蜂鸣器/继电器等无关外设 */
    Timer0_Init();
    Scheduler_Init();
    Timer1_Init();

    /* [修改点11] 上电后从E2PROM加载各模式流转间隔（原代码缺失）*/
    E2PROM_Load();

    while(1)
    {
        Scheduler_Run();
    }
}