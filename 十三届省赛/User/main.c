//头文件区域
#include "stdio.h"
#include "Init.h"
#include "led.h"
#include "key.h"
#include "timer.h"
#include "seg.h"
#include "ds18b20.h"
#include "ds1302.h"

//函数声明区
void task_proc();
void seg_proc();
void led_proc();
void key_proc();

//led专用变量
unsigned char ucled;
//数码管专用变量
unsigned char seg_buf[8]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char seg_string[20]={0}; 
unsigned char pos=0;
//按键专用变量
unsigned char key_down,key_old,key_val;
//调用时间专用变量
unsigned char cnt_seg,cnt_key,cnt_led;
unsigned int cnt_T;
unsigned char cnt_ucRtc;
unsigned int cnt_led1;
unsigned char cnt_led3;
//用户自定义比变量
unsigned  char screen;
unsigned char T_val=23;//温度参数
float Temp;
unsigned char ucRtc[3]={0,0,0};
bit mode;
bit screen_time_flag;
bit led1_flag;
bit relay_flag;





void  main()
{
     cls_init();
     Timer1Init();
    set_ucRtc(ucRtc);
     rd_tempreture();
     Delay750ms();


    while(1)
    {
     task_proc();
    
    }



}

void task_proc()
{
    if(cnt_seg==50)
    {
        seg_proc();
        cnt_seg=0;
        
    
    }
    if(cnt_key==20)
    {
        key_proc();
        cnt_key=0;
    
    }
    
    if(cnt_led==10)
    {
        led_proc();
        cnt_led=0;
    }
    
    if(cnt_T==100)
    {
       Temp= rd_tempreture();
		cnt_T=0;
    }
    
    if(cnt_ucRtc==200)
    {
        read_ucRtc(ucRtc);
		//整点报时功能
		if(ucRtc[1]==0&&ucRtc[2]==0)
		{
			led1_flag=1;
				cnt_led1=0;//从零开始计时
		}//led1点亮
		
		
        cnt_ucRtc=0;
    }
	if(cnt_led1==5000)
		{
			led1_flag=0;//led1熄灭
		}
		
		
		//继电器控制逻辑
		if(mode==0)
		{
			if(Temp>T_val)
			{
				relay(1);
				relay_flag=1;
			}
			else
			{
			relay(0);
				relay_flag=0;
			}
			
		}
		else if(mode==1)
		{
			if(led1_flag==1)
			{	relay(1);
				relay_flag=1;
			}
			else if(led1_flag==0)
			{	relay(0); 
				relay_flag=0;
			}
		}
		
}

void seg_proc()
{
    
    switch(screen)
    {
        case 0://温度
            sprintf(seg_string,"U1   %4.1f",Temp);
        break;
    
        case 1://时间
            if(screen_time_flag==0)//未按下
                sprintf(seg_string,"U2 %02u-%02u",(unsigned int)ucRtc[0],(unsigned int)ucRtc[1]);
            else if(screen_time_flag==1)
                sprintf(seg_string,"U2 %02u-%02u",(unsigned int)ucRtc[1],(unsigned int)ucRtc[2]);
        break;
        
        case 2://参数设置
            sprintf(seg_string,"U3    %02u",(unsigned int)T_val);
        break;
    }
    
    seg_tran(seg_string,seg_buf);
}
void key_proc()
{
    key_val=key_read();
    key_down=key_val&(key_val^key_old);
    key_old=key_val;
    
    switch(key_down)
    {
        case 12:
            if(++screen==3) screen=0;
        break;
        case  13:
            mode ^=1;//0-温度或者1-时间
        break;
        case 16:
            if(screen==2)
            {
                T_val++;
                if(T_val==100)
                    T_val=99;
            }
        break;
            
        case 17:
            if(screen==2)
            {
                T_val--;
                if(T_val==9)
                    T_val=10;
            }
            else if(screen==1)
            {
            }
            break;
            
            
        
    
    
    }

        if(key_old==17)
        {screen_time_flag=1;}
        else
        {screen_time_flag=0;}
        
}


void led_proc()
{
    if(led1_flag=1)
		ucled |=0x01;
	else if(led1_flag=0)
		ucled&=~0x01;
	
	if(mode==0)
		ucled |=0x02;
	else if(mode==1)
			ucled&=~0x02;
	if(relay_flag==1)
	{
		if(cnt_led3==100)
		{
			ucled^=0x04;
			cnt_led3=0;
		}
	}
	else if(relay_flag==0)
	{
		ucled&=~0x04;
	
	
	}
}

void tim_isr() interrupt 3
{

    if(cnt_seg<50) cnt_seg++;
    if(cnt_key<20) cnt_key++;
    if(cnt_led<10) cnt_led++;
    if(cnt_T<100)  cnt_T++;
    if(cnt_ucRtc<200) cnt_ucRtc++;
    seg_disp(seg_buf,pos);
    if(++pos==8) pos=0;
	if(cnt_led1<5000)  cnt_led1++;
	if(cnt_led3<100)  cnt_led3++;
    
    led_disp(ucled);
}