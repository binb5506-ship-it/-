#include <STC15F2K60S2.H>
#include "LED.h"
#include "Init.h"
#include  "timer.h"
#include "seg.h"
#include "stdio.h"//包含sprintf
unsigned long int ms_Tick; //滴答定时器
unsigned char ucled;//led专用变量
unsigned char count;
unsigned char count_com;
unsigned char sign=1;
unsigned char seg_buf[8]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char pos=0;
unsigned char seg_string[10];
unsigned long disp_data=5201314;


void main()
{
	

	per_init();
	
	sprintf(seg_string,"%7lu   ",disp_data);




	seg_tran(seg_string,seg_buf);
	Timer1_Init();
	while(1)
	{
		
	}	
	
	
}

	
	
	void tim_isr() interrupt 3
{
	//每1ms进入一次
	ms_Tick++;
	
	
	//数码管显示
	seg_disp(seg_buf,pos);
	pos++;if(pos==8) pos=0;
		//led显式调用
	led_disp(ucled);
	
}
