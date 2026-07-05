#include "led.h"

/* 当前要写入硬件的 LED 状态字节（bit0=LED1 … bit7=LED8） */
idata unsigned char temp_1     = 0x00;
/* 上一次写入硬件的 LED 状态，用于"状态未变则不刷新"优化 */
idata unsigned char temp_1_old = 0xff;

/* =========================================================
 * LED 显示函数
 *   ucled : 8 元素数组指针，每个元素 0=灭、1=亮
 *           ucled[0] 对应 LED1，ucled[7] 对应 LED8
 * ========================================================= */
void Led_Disp(unsigned char *ucled)
{
    unsigned char temp;

    /* 将 8 个 0/1 值合并成一个字节 */
    temp_1 = 0x00;
    temp_1 = (ucled[0] << 0) | (ucled[1] << 1) | (ucled[2] << 2) | (ucled[3] << 3)
           | (ucled[4] << 4) | (ucled[5] << 5) | (ucled[6] << 6) | (ucled[7] << 7);

    /* 状态未变化则跳过硬件操作，减少总线干扰 */
    if(temp_1 != temp_1_old)
    {
        P0   = ~temp_1;             /* LED 低电平点亮，取反后写 P0 */
        temp = P2 & 0x1f | 0x80;   /* P2[7:5]=100，选通 LED 锁存器 */
        P2   = temp;
        temp = P2 & 0x1f;           /* P2[7:5]=000，关闭选通（完成锁存） */
        P2   = temp;

        temp_1_old = temp_1;        /* 更新缓存状态 */
    }
}

/* =========================================================
 * 关闭所有 LED
 *   同时重置 temp_1_old，确保下次 Led_Disp 能正常刷新
 * ========================================================= */
void led_off(void)
{
    unsigned char temp;

    P0   = 0xff;                /* 所有 LED 熄灭 */
    temp = P2 & 0x1f | 0x80;   /* 选通 LED 锁存器 */
    P2   = temp;
    temp = P2 & 0x1f;           /* 关闭选通 */
    P2   = temp;

    temp_1_old = 0x00;          /* 复位缓存，下次必然触发刷新 */
}
