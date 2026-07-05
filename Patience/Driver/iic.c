#include "iic.h"
#include "intrins.h"
sbit sda=P2^1;
sbit scl=P2^0;

#define DELAY_TIME  5   // 定义一个延时常量，用于控制I2C信号的速率

/**
 * @brief I2C总线延时函数
 * @param n 延时参数，数值越大延时越长
 * @note  这是一个软件延时，通过执行空操作指令_nop_()来产生。
 * I2C的通信速率与此延时有关。
 */
static void I2C_Delay(unsigned char n)
{
    do
    {
        _nop_();_nop_();_nop_();_nop_();_nop_();
        _nop_();_nop_();_nop_();_nop_();_nop_();
        _nop_();_nop_();_nop_();_nop_();_nop_();
    }
    while(n--);
}

/**
 * @brief I2C总线起始信号
 * @note  时序: SCL为高电平时，SDA由高电平跳变为低电平。
 */
void I2CStart(void)
{
    sda = 1; // 1. 确保SDA为高电平
    scl = 1; // 2. 确保SCL为高电平
    I2C_Delay(DELAY_TIME);
    sda = 0; // 3. SCL高电平期间，SDA拉低，产生起始信号
    I2C_Delay(DELAY_TIME);
    scl = 0; // 4. 钳住总线，准备发送数据
}

/**
 * @brief I2C总线停止信号
 * @note  时序: SCL为高电平时，SDA由低电平跳变为高电平。
 */
void I2CStop(void)
{
    sda = 0; // 1. 确保SDA为低电平
    scl = 1; // 2. 拉高SCL
    I2C_Delay(DELAY_TIME);
    sda = 1; // 3. SCL高电平期间，SDA拉高，产生停止信号
    I2C_Delay(DELAY_TIME);
}

/**
 * @brief 通过I2C总线发送一个字节
 * @param byt 要发送的字节数据
 * @note  数据从最高位（MSB）开始发送。
 */
void I2CSendByte(unsigned char byt)
{
    unsigned char i;

    for(i=0; i<8; i++){ // 一个字节8位，循环8次
        scl = 0; // 1. 拉低SCL，准备在SDA上放置数据位
        I2C_Delay(DELAY_TIME);
        if(byt & 0x80){ // 2. 判断当前字节的最高位是1还是0
            sda = 1;    //    如果是1，则SDA置1
        }
        else{
            sda = 0;    //    如果是0，则SDA置0
        }
        I2C_Delay(DELAY_TIME);
        scl = 1; // 3. 拉高SCL，从机在此上升沿读取SDA上的数据位
        byt <<= 1; // 4. 将字节左移一位，准备发送下一位
        I2C_Delay(DELAY_TIME);
    }

    scl = 0;   // 5. 8位发送完毕后，拉低SCL，准备接收ACK信号
}

/**
 * @brief 通过I2C总线接收一个字节
 * @return unsigned char 返回接收到的字节数据
 * @note  数据从最高位（MSB）开始接收。
 */
unsigned char I2CReceiveByte(void)
{
    unsigned char da; // 用于存储接收到的数据
    unsigned char i;
    for(i=0; i<8; i++){
        scl = 1; // 1. 拉高SCL，通知从机可以把数据放到SDA上了
        I2C_Delay(DELAY_TIME);
        da <<= 1; // 2. 将接收变量左移一位，为新来的数据位腾出空间(LSB)
        if(sda)   // 3. 读取SDA线上的电平
            da |= 0x01; // 如果SDA为1，则将接收变量的最低位置1
        scl = 0; // 4. 拉低SCL，完成一位数据的接收
        I2C_Delay(DELAY_TIME);
    }
    return da; // 5. 返回完整的8位数据
}

/**
 * @brief 等待从机的应答(ACK)信号
 * @return unsigned char 返回ACK位。0表示有效应答(ACK)，1表示无应答(NACK)。
 */
unsigned char I2CWaitAck(void)
{
    unsigned char ackbit;

    scl = 1; // 1. 主机拉高SCL，并释放SDA，准备接收从机的应答
    I2C_Delay(DELAY_TIME);
    ackbit = sda; // 2. 读取SDA线上的电平，即ACK位
    scl = 0; // 3. 主机拉低SCL
    I2C_Delay(DELAY_TIME);

    return ackbit; // 返回应答位
}

/**
 * @brief 主机向从机发送应答(ACK)或非应答(NACK)信号
 * @param ackbit 要发送的应答位。0表示ACK，1表示NACK。
 */
void I2CSendAck(unsigned char ackbit)
{
    scl = 0;    // 1. 拉低SCL
    sda = ackbit; // 2. 在SDA上放置ACK(0)或NACK(1)位
    I2C_Delay(DELAY_TIME);
    scl = 1;    // 3. 拉高SCL，从机在上升沿读取此信号
    I2C_Delay(DELAY_TIME);
    scl = 0;    // 4. 拉低SCL
    sda = 1;    // 5. 主机释放SDA线，通常使其恢复高电平
    I2C_Delay(DELAY_TIME);
}




unsigned char Ad_Read(unsigned char addr)
{
	unsigned char temp=0;
	I2CStart();
	I2CSendByte(0x90);
	I2CWaitAck();
	I2CSendByte(addr);
	I2CWaitAck();
	I2CStart();
	I2CSendByte(0x91);
	I2CWaitAck();
	temp=I2CReceiveByte();
	I2CSendAck(1);
	I2CStop();
	
	return temp;






}

void Da_Write(unsigned dat)
{
	I2CStart();
	I2CSendByte(0x90);
	I2CWaitAck();
	I2CSendByte(0x43);
	I2CWaitAck();
	I2CSendByte(dat);
	I2CWaitAck();
	I2CStop();

}

void EEPROM_Write(unsigned char *str,unsigned char addr,unsigned num)
{
	I2CStart();
	I2CSendByte(0xa0);
	I2CWaitAck();
	I2CSendByte(addr);
	I2CWaitAck();
	while(num--)
	{
		I2CSendByte(*str++);
		I2CWaitAck();
		I2C_Delay(250);
		
	
	
	}
	
	I2CStop();

	I2C_Delay(250);I2C_Delay(250);I2C_Delay(250);I2C_Delay(250);I2C_Delay(250);
	I2C_Delay(250);I2C_Delay(250);I2C_Delay(250);I2C_Delay(250);I2C_Delay(250);









}


void EEPROM_Read(unsigned char *str,unsigned char addr,unsigned num)
{
	I2CStart();
	I2CSendByte(0xa0);
	I2CWaitAck();
	I2CSendByte(addr);
	I2CWaitAck();
	I2CStart();
	I2CSendByte(0xa1);
	I2CWaitAck();
	while(num--)
	{
		*str++=I2CReceiveByte();
		if(num)
		{
			I2CSendAck(0);
		
		}
		else
		{
				I2CSendAck(1);
		
		}
	
	
	}
	
I2CStop();








}
