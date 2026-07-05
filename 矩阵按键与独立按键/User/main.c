#include <STC15F2K60S2.H>
#include "LED.h"
#include "Init.h"
#include "Intrins.h"
#include "key.h"
unsigned char ucled=0;
unsigned char Key_Num,Key_old,Key_Down;



void main()
{
    	
	per_init();//外设初始化，关闭所有LED灯
	

	while(1)
	{
		//消抖
		Key_Num=Key_Read();
		Key_Down=Key_Num&(Key_Num^Key_old);
		Key_old=Key_Num;
		
		if(Key_Down==7)
		{
			ucled^=0x01;
			
		
		}
		else if(Key_Down==8)
		{
			ucled^=0x02;
		}
		else if(Key_Down==12)
		{
			ucled^=0x04;
		}
		
		
		
		led_disp(ucled);
	}
	
	

}
