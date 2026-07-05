#include "init.h"      // 系统初始化相关
#include "key.h"       // 按键读取函数 Key_Read()
#include "led.h"       // LED显示函数 Led_Disp()
#include "onewire.h"   // DS18B20单总线驱动，提供 rd_temperature()
#include "seg.h"       // 数码管驱动，提供 Seg_Disp()
#include "iic.h"       // I2C驱动，PCF8591用到，提供 Ad_Read()
#include "intrins.h"   // Keil内部函数，提供 _nop_()
#include "ds1302.h"    // DS1302驱动，提供 Read_RTC()
#include "string.h"
#include "stdio.h"


/* =====================================================
 * 按键相关变量
 * 三变量检测法：
 *   Key_Val  = 当前按键值
 *   Key_Down = 按下沿（刚按下的瞬间为1，其余为0）
 *   Key_Up   = 松开沿（刚松开的瞬间为1，其余为0）
 *   Key_Old  = 上次按键值（用于和当前值做异或，检测变化）
 * ===================================================== */
idata unsigned char Key_Val = 0, Key_Down = 0, Key_Up = 0, Key_Old = 0;


/* =====================================================
 * 数码管相关变量
 *   Seg_Pos  = 当前扫描到第几位（0~7），由Timer1 ISR每1ms轮换
 *   Seg_Buf  = 8位数码管的显示缓冲区，每个元素代表该位显示什么
 *              0~9    = 数字0~9
 *              10     = 熄灭
 *              11     = 间隔符"-"
 *              12     = 字符C（温度标识）
 *              13     = 字符H（湿度标识）
 *              14     = 字符F（时间回显标识）
 *              15     = 字符P（参数界面标识）
 *              16     = 字符E（温湿度界面标识）
 *              17     = 字符A（湿度无效时显示AA）
 *              >20    = 该位数字带小数点（实际值减去','的ASCII码）
 *   ucled    = 8个LED的状态缓冲区，1=亮，0=灭
 * ===================================================== */
idata unsigned char  Seg_Pos = 0;
pdata unsigned char  Seg_Buf[8] = {10,10,10,10,10,10,10,10}; // 初始全部熄灭
pdata unsigned char  ucled[8]   = {0,0,0,0,0,0,0,0};         // 初始全部熄灭


/* =====================================================
 * 系统计时
 *   uwTick = 系统运行毫秒数，由Timer1 ISR每1ms加1
 *   用途：任务调度、长按计时、3秒冷却计时、L4闪烁
 * ===================================================== */
idata unsigned long int uwTick = 0;


/* =====================================================
 * DS1302时间数据
 *   ucRtc[0] = 小时
 *   ucRtc[1] = 分钟
 *   ucRtc[2] = 秒
 * ===================================================== */
pdata unsigned char ucRtc[3] = {13, 3, 5};


/* =====================================================
 * 界面状态
 *   Seg_Show_Mode = 当前主界面
 *                   0 = 时间界面
 *                   1 = 回显界面
 *                   2 = 参数界面
 *   Type          = 回显子界面
 *                   0 = 温度回显
 *                   1 = 湿度回显
 *                   2 = 时间回显
 *   Seg_Mode      = 是否处于温湿度界面
 *                   0 = 正常（时间/回显/参数）
 *                   1 = 触发后的温湿度界面（优先级最高）
 * ===================================================== */
idata unsigned char Seg_Show_Mode = 0;
idata unsigned char Type          = 0;
idata bit           Seg_Mode      = 0;


/* =====================================================
 * 传感器数据
 *   Temper = DS18B20读取的当前温度（整数，单位℃）
 *   Shidu  = NE555频率换算出的当前湿度（整数，单位%）
 *            特殊值：0 = 无效数据（频率不在200~2000Hz范围）
 *   Freq   = Timer0计数器1秒内的脉冲数，即NE555频率
 * ===================================================== */
idata unsigned char Temper = 0;
idata unsigned char Shidu  = 0;
idata unsigned int  Freq   = 0;


/* =====================================================
 * 温度参数
 *   Wendu = 报警温度阈值，S8/S9在参数界面下调节
 *           默认值30℃，采集温度超过此值L4闪烁
 * ===================================================== */
idata unsigned char Wendu = 30;


/* =====================================================
 * 统计数据
 *   Times            = 累计触发次数（湿度有效时才计入）
 *   Temper_Max        = 所有有效采集中的最大温度
 *   Shidu_Max         = 所有有效采集中的最大湿度
 *   Temper_Sum        = 温度累加和（用于计算平均值）
 *   Shidu_Sum         = 湿度累加和
 *   Float_Temper_10x  = 平均温度×10（保留1位小数）
 *                       例：平均23.2℃ → 存232
 *   Float_Shidu_10x   = 平均湿度×10
 *   Old_ucRtc[0]      = 最近一次触发时的小时
 *   Old_ucRtc[1]      = 最近一次触发时的分钟
 * ===================================================== */
idata unsigned char  Times            = 0;
idata unsigned char  Temper_Max       = 0;
idata unsigned char  Shidu_Max        = 0;
idata unsigned int   Temper_Sum       = 0;
idata unsigned int   Shidu_Sum        = 0;
idata unsigned int   Float_Temper_10x = 0;
idata unsigned int   Float_Shidu_10x  = 0;
pdata unsigned char  Old_ucRtc[2]     = {0, 0};


/* =====================================================
 * L6比较用变量
 *   Collected_Temper = 本次采集的温度（触发时更新）
 *   Collected_Shidu  = 本次采集的湿度
 *   Last_Temper      = 上次采集的温度（用于和本次比较）
 *   Last_Shidu       = 上次采集的湿度
 *
 * 逻辑：每次触发时
 *   先把Collected的值存到Last
 *   再把本次采集值存到Collected
 *   这样Last和Collected就是相邻两次的数据
 * ===================================================== */
idata unsigned char Collected_Temper = 0;
idata unsigned char Collected_Shidu  = 0;
idata unsigned char Last_Temper      = 0;
idata unsigned char Last_Shidu       = 0;


/* =====================================================
 * L5标志
 *   Shidu_Invalid_Flag = 上次采集的湿度是否无效
 *                        1 = 无效，L5点亮
 *                        0 = 有效，L5熄灭
 * ===================================================== */
idata bit Shidu_Invalid_Flag = 0;


/* =====================================================
 * 光照检测相关
 *   Last_Light   = 上次检测到的光照状态（1=亮，0=暗）
 *                  初值1，认为开机时是亮的
 *   Trigger_Time = 触发时刻的uwTick值
 *                  用于计算3秒冷却：uwTick - Trigger_Time >= 3000
 * ===================================================== */
idata unsigned char     Last_Light    = 1;
idata unsigned long int Trigger_Time  = 0;


/* =====================================================
 * S9长按计时
 *   S9_Press_Time = S9按下时的uwTick值
 *   松开时用 uwTick - S9_Press_Time 判断是否超过2秒
 * ===================================================== */
idata unsigned long int S9_Press_Time = 0;


/* =====================================================
 * Timer0初始化：计数器模式
 *   TMOD |= 0x05 → T0工作在模式1计数器模式
 *   外部脉冲从P34引脚输入，每来一个脉冲TH0/TL0加1
 *   Freq_Proc每1秒读一次计数值，就得到了频率
 * ===================================================== */
void Timer0_Init(void)
{
    AUXR &= 0x7F;   // T0使用12T模式（12个时钟周期计一次）
    TMOD &= 0xF0;   // 清除T0的模式位
    TMOD |= 0x05;   // T0设为模式1（16位计数器）+计数器模式
    TL0 = 0;        // 计数初值清零
    TH0 = 0;
    TR0 = 1;        // 启动T0开始计数
}


/* =====================================================
 * Timer1初始化：1ms定时器
 *   每1ms产生一次中断，在ISR里完成：
 *   1. uwTick++（系统计时）
 *   2. 数码管扫描（每1ms切换一位）
 * ===================================================== */
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


/* =====================================================
 * 清除所有统计数据
 *   S9长按2秒后松开时调用
 *   把所有统计相关变量全部归零
 * ===================================================== */
void Clear_Data(void)
{
    Times            = 0;
    Temper_Max       = 0;
    Shidu_Max        = 0;
    Temper_Sum       = 0;
    Shidu_Sum        = 0;
    Float_Temper_10x = 0;
    Float_Shidu_10x  = 0;
    Old_ucRtc[0]     = 0;
    Old_ucRtc[1]     = 0;
    Collected_Temper = 0;
    Collected_Shidu  = 0;
    Last_Temper      = 0;
    Last_Shidu       = 0;
    Shidu_Invalid_Flag = 0;
}


/* =====================================================
 * 按键处理：每10ms执行一次
 * ===================================================== */
void Key_Proc(void)
{
    // 三变量检测法
    Key_Val  = Key_Read();
    Key_Down = Key_Val  & (Key_Val  ^ Key_Old); // 按下沿：当前为1，上次为0
    Key_Up   = ~Key_Val & (Key_Val  ^ Key_Old); // 松开沿：当前为0，上次为1
    Key_Old  = Key_Val;                          // 保存本次状态供下次比较

    // 温湿度界面期间所有按键无效
    // 注意：return写在读按键之后，保证Key_Old正常更新
    // 否则回到正常界面时Key_Old是旧值，会误触发
    if(Seg_Mode == 1) return;

    // S9按下时记录时刻，用于后面判断长按时长
    if(Key_Down == 9) S9_Press_Time = uwTick;

    switch(Key_Down)
    {
        case 4: // S4：主界面循环切换（时间→回显→参数→时间）
            Seg_Show_Mode = (++Seg_Show_Mode) % 3;
            // 切换到回显界面时，重置子界面为温度回显
            // 注意：必须在switch里判断，不能在Seg_Proc里判断
            // 因为要的是"切换那一刻"重置，而不是"在回显界面期间一直"重置
            if(Seg_Show_Mode == 1) Type = 0;
            break;

        case 5: // S5：回显子界面切换（仅回显界面有效）
            if(Seg_Show_Mode == 1)
                Type = (++Type) % 3; // 0→1→2→0循环
            break;

        case 8: // S8：温度参数+1（仅参数界面有效）
            if(Seg_Show_Mode == 2)
                Wendu++;
            break;

        case 9: // S9短按：温度参数-1（仅参数界面有效）
            if(Seg_Show_Mode == 2)
                Wendu--;
            break;
    }

    // S9长按检测：松开时判断按住了多久
    // 条件：在时间回显子界面 && 按住时长>=2秒
    if(Key_Up == 9 && Seg_Show_Mode == 1 && Type == 2)
    {
        if(uwTick - S9_Press_Time >= 2000)
            Clear_Data();
    }
}


/* =====================================================
 * 数码管显示：每20ms执行一次
 * 只负责填写Seg_Buf，实际显示由Timer1 ISR完成
 * ===================================================== */
void Seg_Proc(void)
{
    // 温湿度界面优先级最高，直接覆盖其他界面
    if(Seg_Mode == 1)
    {
        Seg_Buf[0] = 16;  // E（温湿度界面标识）
        Seg_Buf[1] = 10;  // 熄灭
        Seg_Buf[2] = 10;  // 熄灭
        Seg_Buf[3] = Temper / 10 % 10; // 温度十位
        Seg_Buf[4] = Temper % 10;       // 温度个位
        Seg_Buf[5] = 11;                // 间隔符"-"

        if(Shidu == 0) // 湿度无效：显示"AA"
        {
            Seg_Buf[6] = 17; // A
            Seg_Buf[7] = 17; // A
        }
        else
        {
            Seg_Buf[6] = Shidu / 10 % 10; // 湿度十位
            Seg_Buf[7] = Shidu % 10;       // 湿度个位
        }
        return; // 直接返回，不再执行下面的逻辑
    }

    if(Seg_Show_Mode == 0) // 时间界面
    {
        // 格式：时-分-秒，每个数据固定占2位，不足补0
        // 例：13时3分5秒 → 1 3 - 0 3 - 0 5
        Seg_Buf[0] = ucRtc[0] / 10;      // 时十位
        Seg_Buf[1] = ucRtc[0] % 10;      // 时个位
        Seg_Buf[2] = 11;                  // "-"
        Seg_Buf[3] = ucRtc[1] / 10;      // 分十位
        Seg_Buf[4] = ucRtc[1] % 10;      // 分个位
        Seg_Buf[5] = 11;                  // "-"
        Seg_Buf[6] = ucRtc[2] / 10;      // 秒十位
        Seg_Buf[7] = ucRtc[2] % 10;      // 秒个位
    }
    else if(Seg_Show_Mode == 1) // 回显界面
    {
        if(Type == 0) // 温度回显
        {
            Seg_Buf[0] = 12;  // C（温度标识）
            Seg_Buf[1] = 10;  // 熄灭

            if(Times == 0) // 从未触发过，除标识符外全部熄灭
            {
                Seg_Buf[2]=10; Seg_Buf[3]=10; Seg_Buf[4]=10;
                Seg_Buf[5]=10; Seg_Buf[6]=10; Seg_Buf[7]=10;
            }
            else
            {
                // 格式：C 熄灭 最大温度(2位) - 平均温度(3位含小数点)
                // 例：C 熄灭 28 - 23.2
                Seg_Buf[2] = Temper_Max / 10 % 10; // 最大温度十位
                Seg_Buf[3] = Temper_Max % 10;       // 最大温度个位
                Seg_Buf[4] = 11;                    // "-"

                // 平均温度×10存在Float_Temper_10x里
                // 例：平均23.2℃ → Float_Temper_10x=232
                // 232/100=2（十位），232/10%10=3（个位，带小数点），232%10=2（小数位）
                Seg_Buf[5] = Float_Temper_10x / 100 % 10;       // 十位
                Seg_Buf[6] = Float_Temper_10x / 10 % 10 + ',';  // 个位+小数点（加','使值>20，ISR会显示小数点）
                Seg_Buf[7] = Float_Temper_10x % 10;             // 小数位
            }
        }
        else if(Type == 1) // 湿度回显
        {
            Seg_Buf[0] = 13;  // H（湿度标识）
            Seg_Buf[1] = 10;

            if(Times == 0)
            {
                Seg_Buf[2]=10; Seg_Buf[3]=10; Seg_Buf[4]=10;
                Seg_Buf[5]=10; Seg_Buf[6]=10; Seg_Buf[7]=10;
            }
            else
            {
                Seg_Buf[2] = Shidu_Max / 10 % 10;
                Seg_Buf[3] = Shidu_Max % 10;
                Seg_Buf[4] = 11;
                Seg_Buf[5] = Float_Shidu_10x / 100 % 10;
                Seg_Buf[6] = Float_Shidu_10x / 10 % 10 + ',';
                Seg_Buf[7] = Float_Shidu_10x % 10;
            }
        }
        else if(Type == 2) // 时间回显
        {
            Seg_Buf[0] = 14;               // F（时间回显标识）
            Seg_Buf[1] = Times / 10 % 10;  // 触发次数十位（不足2位补0）
            Seg_Buf[2] = Times % 10;       // 触发次数个位

            if(Times == 0) // 从未触发，时间部分熄灭
            {
                Seg_Buf[3]=10; Seg_Buf[4]=10; Seg_Buf[5]=10;
                Seg_Buf[6]=10; Seg_Buf[7]=10;
            }
            else
            {
                // 格式：F 触发次数(2位) 时(2位) - 分(2位)
                Seg_Buf[3] = Old_ucRtc[0] / 10 % 10; // 触发时的小时十位
                Seg_Buf[4] = Old_ucRtc[0] % 10;       // 触发时的小时个位
                Seg_Buf[5] = 11;                       // "-"
                Seg_Buf[6] = Old_ucRtc[1] / 10 % 10;  // 触发时的分钟十位
                Seg_Buf[7] = Old_ucRtc[1] % 10;        // 触发时的分钟个位
            }
        }
    }
    else if(Seg_Show_Mode == 2) // 参数界面
    {
        // 格式：P 熄灭×5 温度参数(2位)
        Seg_Buf[0] = 15;  // P
        Seg_Buf[1]=10; Seg_Buf[2]=10; Seg_Buf[3]=10;
        Seg_Buf[4]=10; Seg_Buf[5]=10;
        Seg_Buf[6] = Wendu / 10 % 10; // 温度参数十位
        Seg_Buf[7] = Wendu % 10;       // 温度参数个位
    }
}


/* =====================================================
 * LED指示灯：每1ms执行一次
 * 每次先全部清零，再根据条件点亮，避免状态残留
 * ===================================================== */
void Led_Proc(void)
{
    unsigned char i;
    for(i = 0; i < 8; i++) ucled[i] = 0; // 全部清零

    // L1：时间界面下点亮（不在温湿度界面 且 在时间界面）
    if(Seg_Mode == 0 && Seg_Show_Mode == 0) ucled[0] = 1;

    // L2：回显界面下点亮
    if(Seg_Mode == 0 && Seg_Show_Mode == 1) ucled[1] = 1;

    // L3：温湿度界面下点亮
    if(Seg_Mode == 1) ucled[2] = 1;

    // L4：采集温度>温度参数时，0.1秒闪烁
    // uwTick每1ms加1，除以100就是每100ms进位
    // 对2取余在0和1之间循环，实现0.1秒切换亮灭
    if(Temper > Wendu)
        ucled[3] = (uwTick / 100) % 2;

    // L5：上次采集湿度无效时点亮，直到下次有效采集才熄灭
    if(Shidu_Invalid_Flag) ucled[4] = 1;

    // L6：本次采集的温度和湿度均高于上次时点亮
    // Times>=2保证至少有两次采集才做比较
    if(Times >= 2 && Collected_Temper > Last_Temper && Collected_Shidu > Last_Shidu)
        ucled[5] = 1;

    Led_Disp(ucled); // 把ucled数组输出到硬件
}


/* =====================================================
 * 频率→湿度换算：每1000ms执行一次
 * 原理：Timer0计数器模式，1秒内P34收到多少脉冲就是多少Hz
 * ===================================================== */
void Freq_Proc(void)
{
    // 停止计数，读出1秒内的脉冲数
    TR0 = 0;
    Freq = (unsigned int)TH0 * 256 + TL0;
    TL0 = 0; // 清零，准备下一秒计数
    TH0 = 0;
    TR0 = 1; // 重新开始计数

    // 频率不在200~2000Hz范围内，认为是无效数据
    if(Freq < 200 || Freq > 2000)
    {
        Shidu = 0; // 0代表无效
    }
    else
    {
        // 线性换算公式：
        // 200Hz→10%，2000Hz→90%
        // Shidu = 2*(Freq-200)/45 + 10
        // 用2UL避免乘法溢出（unsigned int最大65535）
        Shidu = (unsigned char)(2UL * (Freq - 200) / 45 + 10);
    }
}


/* =====================================================
 * DS18B20温度读取：每500ms执行一次
 * rd_temperature()由onewire.h提供，返回浮点温度
 * 强制转换为整数，舍弃小数部分
 * ===================================================== */
void DS18B20_Proc(void)
{
    Temper = (unsigned char)rd_temperature();
}


/* =====================================================
 * DS1302时间读取：每500ms执行一次
 * Read_RTC()由ds1302.h提供，填充ucRtc数组
 * ===================================================== */
void RTC_Proc(void)
{
    Read_Rtc(ucRtc); // ucRtc[0]=时，[1]=分，[2]=秒
}


/* =====================================================
 * 光照检测 & 采集触发：每200ms执行一次
 * 200ms执行一次满足题目"亮暗感知时间≤0.5秒"的要求
 * ===================================================== */
void Light_Proc(void)
{
    // 读PCF8591光敏通道（通道1）
    unsigned char adc_val   = Ad_Read(0x01);
    // 阈值128：大于128认为是亮，小于等于128认为是暗
    // 实际阈值需根据硬件环境调整
    unsigned char cur_light = (adc_val > 128) ? 1 : 0;

    // 检测亮→暗的下降沿
    // 条件：上次是亮（Last_Light==1）
    //       当前是暗（cur_light==0）
    //       不在温湿度界面冷却期（Seg_Mode==0）
    if(Last_Light == 1 && cur_light == 0 && Seg_Mode == 0)
    {
        // 记录本次湿度是否无效（用于L5）
        Shidu_Invalid_Flag = (Shidu == 0) ? 1 : 0;

        if(Shidu != 0) // 湿度有效才统计，无效时不计入回显数据
        {
            Times++; // 触发次数+1

            // 保存触发时刻（时、分）
            Old_ucRtc[0] = ucRtc[0];
            Old_ucRtc[1] = ucRtc[1];

            // 更新L6比较基准
            // 先把本次存到Last，再把当前传感器值存到Collected
            Last_Temper      = Collected_Temper;
            Last_Shidu       = Collected_Shidu;
            Collected_Temper = Temper;
            Collected_Shidu  = Shidu;

            // 更新最大值
            if(Temper > Temper_Max) Temper_Max = Temper;
            if(Shidu  > Shidu_Max)  Shidu_Max  = Shidu;

            // 更新累加和，重新计算平均值
            // ×10是为了保留1位小数：平均23.2℃ → 存232
            // 10UL防止乘法溢出
            Temper_Sum += Temper;
            Shidu_Sum  += Shidu;
            Float_Temper_10x = (unsigned int)(Temper_Sum * 10UL / Times);
            Float_Shidu_10x  = (unsigned int)(Shidu_Sum  * 10UL / Times);
        }

        // 无论湿度是否有效，都切换到温湿度界面
        Seg_Mode     = 1;
        Trigger_Time = uwTick; // 记录触发时刻，用于3秒计时
    }

    // 更新上次光照状态，供下次比较
    Last_Light = cur_light;

    // 3秒冷却期结束，返回原界面
    // uwTick - Trigger_Time >= 3000 → 距触发已过3秒
    if(Seg_Mode == 1 && (uwTick - Trigger_Time >= 3000))
        Seg_Mode = 0;
}


/* =====================================================
 * 任务调度器
 * 原理：每个任务有自己的执行周期rate_ms和上次执行时刻last_ms
 *       Scheduler_Run每次循环检查所有任务
 *       如果当前时间-上次执行时间 >= 执行周期，就执行该任务
 *       执行完后更新last_ms
 * 优点：各任务独立计时，互不影响，类似RTOS的思路
 * ===================================================== */
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
    {DS18B20_Proc,  500,  0}, // 温度每500ms读一次
    {RTC_Proc,      500,  0}, // 时间每500ms读一次
    {Light_Proc,    200,  0}, // 光照每200ms检测一次（满足≤0.5秒感知）
    {Freq_Proc,     1000, 0}  // 频率每1秒统计一次
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