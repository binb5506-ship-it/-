#include "iic.h"
#include "intrins.h"

#define DELAY_TIME 5    /* I2C 延时参数，控制通信速率 */

/* ---- 引脚定义 ----
 * 变量名必须全小写，与下方所有函数中的 sda/scl 保持一致
 * 原来写的 SDA/SCL 大写或 SBIT 都会导致编译报错或引用未定义变量 */
sbit scl = P2^0;    /* SCL 时钟线 → P2.0 */
sbit sda = P2^1;    /* SDA 数据线 → P2.1 */


/* ---- 内部延时（static 限本文件可见）---- */
static void I2C_Delay(unsigned char n)
{
    do
    {
        _nop_();
    }
    while(n--);
}


/* =========================================================
 * 起始信号：SCL 高电平期间，SDA 由高变低
 * ========================================================= */
void I2CStart(void)
{
    sda = 1;
    scl = 1;
    I2C_Delay(DELAY_TIME);
    sda = 0;    /* SDA 下降沿 = 起始信号 */
    I2C_Delay(DELAY_TIME);
    scl = 0;    /* 钳住总线，准备传输 */
}


/* =========================================================
 * 停止信号：SCL 高电平期间，SDA 由低变高
 * ========================================================= */
void I2CStop(void)
{
    sda = 0;
    scl = 1;
    I2C_Delay(DELAY_TIME);
    sda = 1;    /* SDA 上升沿 = 停止信号 */
    I2C_Delay(DELAY_TIME);
}


/* =========================================================
 * 发送一个字节（MSB 先发）
 * ========================================================= */
void I2CSendByte(unsigned char byt)
{
    unsigned char i;

    for(i = 0; i < 8; i++)
    {
        scl = 0;
        I2C_Delay(DELAY_TIME);
        sda = (byt & 0x80) ? 1 : 0;    /* 取最高位放到 SDA */
        I2C_Delay(DELAY_TIME);
        scl = 1;                         /* 上升沿从机采样 */
        byt <<= 1;                       /* 左移，准备下一位 */
        I2C_Delay(DELAY_TIME);
    }
    scl = 0;    /* 8 位发完，拉低准备接收 ACK */
}


/* =========================================================
 * 接收一个字节（MSB 先收）
 * ========================================================= */
unsigned char I2CReceiveByte(void)
{
    unsigned char da;
    unsigned char i;

    for(i = 0; i < 8; i++)
    {
        scl = 1;
        I2C_Delay(DELAY_TIME);
        da <<= 1;
        if(sda) da |= 0x01;     /* 读取当前位 */
        scl = 0;
        I2C_Delay(DELAY_TIME);
    }
    return da;
}


/* =========================================================
 * 等待从机 ACK（返回 0=ACK，1=NACK）
 * ========================================================= */
unsigned char I2CWaitAck(void)
{
    unsigned char ackbit;

    scl = 1;
    I2C_Delay(DELAY_TIME);
    ackbit = sda;   /* 读取 ACK 位 */
    scl = 0;
    I2C_Delay(DELAY_TIME);

    return ackbit;
}


/* =========================================================
 * 主机发送 ACK/NACK（0=ACK 继续，1=NACK 结束）
 * ========================================================= */
void I2CSendAck(unsigned char ackbit)
{
    scl = 0;
    sda = ackbit;
    I2C_Delay(DELAY_TIME);
    scl = 1;
    I2C_Delay(DELAY_TIME);
    scl = 0;
    sda = 1;    /* 释放 SDA */
    I2C_Delay(DELAY_TIME);
}


/* =========================================================
 * PCF8591 ADC 读取
 *   addr : 控制字节，选择通道（如 0x03 = 通道3 滑动变阻器）
 * ========================================================= */
unsigned char Ad_Read(unsigned char addr)
{
    unsigned char temp;

    I2CStart();
    I2CSendByte(0x90);          /* 设备地址 + 写 */
    I2CWaitAck();
    I2CSendByte(addr);          /* 发送控制字节，选通道 */
    I2CWaitAck();

    I2CStart();                 /* 重复起始，切换读模式 */
    I2CSendByte(0x91);          /* 设备地址 + 读 */
    I2CWaitAck();
    temp = I2CReceiveByte();    /* 读取 AD 结果 */
    I2CSendAck(1);              /* 发 NACK，通知从机结束 */
    I2CStop();                  /* 必须加括号才是调用！原来 I2CStop 无括号只是取地址 */

    return temp;
}


/* =========================================================
 * PCF8591 DAC 输出
 *   dat : 要转换的数字量（0~255 对应 0~5V）
 * ========================================================= */
void Da_Write(unsigned char dat)
{
    I2CStart();
    I2CSendByte(0x90);      /* 设备地址 + 写 */
    I2CWaitAck();
    I2CSendByte(0x41);      /* 控制字节：使能 DA 输出 */
    I2CWaitAck();
    I2CSendByte(dat);       /* 发送数字量，参数是 dat 不是 addr！*/
    I2CWaitAck();
    I2CStop();              /* 加括号调用 */
}


/* =========================================================
 * AT24C02 EEPROM 写入多字节
 *   str  : 数据源指针
 *   addr : EEPROM 内部起始地址
 *   num  : 写入字节数
 * ========================================================= */
void EEPROM_Write(unsigned char *str, unsigned char addr, unsigned num)
{
    I2CStart();
    I2CSendByte(0xa0);      /* 设备地址 + 写 */
    I2CWaitAck();
    I2CSendByte(addr);      /* 发送内部写入地址 */
    I2CWaitAck();

    while(num--)
    {
        I2CSendByte(*str++);
        I2CWaitAck();
        I2C_Delay(200);     /* 等待 EEPROM 内部写周期（函数名是 I2C_Delay，不是 IICDelay）*/
    }
    I2CStop();

    /* 写完后额外等待，确保最后一字节写入完成 */
    I2C_Delay(250);
    I2C_Delay(250);
    I2C_Delay(250);
}


/* =========================================================
 * AT24C02 EEPROM 读取多字节
 *   str  : 数据目标指针
 *   addr : EEPROM 内部起始地址
 *   num  : 读取字节数
 * ========================================================= */
void EEPROM_Read(unsigned char *str, unsigned char addr, unsigned num)
{
    /* 第一步：虚写，设置 EEPROM 内部地址指针 */
    I2CStart();
    I2CSendByte(0xa0);
    I2CWaitAck();
    I2CSendByte(addr);
    I2CWaitAck();

    /* 第二步：重复起始，切换读模式 */
    I2CStart();
    I2CSendByte(0xa1);      /* 设备地址 + 读 */
    I2CWaitAck();

    while(num--)
    {
        *str++ = I2CReceiveByte();  /* 接收用 I2CReceiveByte，不是 I2CSendByte！*/
        if(num)
            I2CSendAck(0);  /* 不是最后字节，发 ACK，继续读 */
        else
            I2CSendAck(1);  /* 最后字节，发 NACK，通知从机结束 */
    }
    I2CStop();  /* I2CStop 在循环外，只调用一次！原来写在循环内是错的 */
}
