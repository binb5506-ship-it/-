#include "Seg.h"
pdata unsigned char Seg_dula[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,
															 0xff};

void Seg_Disp(unsigned char wela,unsigned char dula,bit point)
{
	unsigned char temp;
		P0=0xff;
	temp=P2&0x1f|0xe0;
	P2=temp;
	temp=P2&0x1f;
	P2=temp;
	P0=0x01 << wela;
	temp=P2&0x1f|0xc0;
	P2=temp;
	temp=P2&0x1f;
	P2=temp;
	P0=Seg_dula[dula];
	if(point)
	{
		P0&=0X7f;
	
	}
	temp=P2&0x1f|0xe0;
	P2=temp;
	temp=P2&0x1f;
	P2=temp;




}