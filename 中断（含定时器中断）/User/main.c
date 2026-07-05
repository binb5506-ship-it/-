#include <STC15F2K60S2.H>
#include "LED.h"
#include "Init.h"
#include  "timer.h"
unsigned long int ms_Tick; //滴答定时器
unsigned char ucled;//led专用变量
unsigned char count;
unsigned char count_com;
unsigned char sign=1;



void main()
{
	

	per_init();
	Timer1_Init();
	





	while(1)
	{
		//if(ms_Tick==500)//每500ms进入一次，0.5s进入一次
		//{
//			//流水灯
//			if(count<8)
//			{
//				ucled=0x01<<count;//每次左移一位
//				count++;
//			
//			
//			}
//			else
//				count=0;
//			
//			ms_Tick=0;
//		
//		
//		}
	
	//呼吸灯
			if(ms_Tick==50)//每500ms进入一次，0.5s进入一次
			{
				if(sign==1)
				{
					count_com++;
				}
			
				else if(sign==0)
				{
					count_com--;
				}
				
				if(count_com==10)
				{
					sign=0;
				
				}
				else if(count_com==0)
				{
					sign=1;
				}
			
			ms_Tick=0;
			
			}
	}

}	
void tim_isr() interrupt 3
{
	//每1ms进入一次
	ms_Tick++;
	count++;
	if(count==10) count=0;
	if(count<count_com)
		ucled=0xff;
	else
		ucled=0x00;

	led_disp(ucled);
	
}
