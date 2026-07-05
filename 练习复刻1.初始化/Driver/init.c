#include "init.h"

void system_init(void)
{
	unsigned char temp;
	
	P0=0xff;
	temp=(P2&0x1f)|0x80;
	P2=temp;
	P2&=0x1f;
	
	
	
	P0=0x00;
	temp=(P2&0x1f)|0xa0;
	P2=temp;
	P2&=0x1f;

	
	
	
//	 // 数码管消隐(段选)
//    P0 = 0x00;  // 共阴极数码管,0熄灭
//    temp = (P2 & 0x1f) | 0xc0;
//    P2 = temp;
//    P2 &= 0x1f;
//    
//    // 数码管消隐(位选)
//    P0 = 0x00;
//    temp = (P2 & 0x1f) | 0xe0;
//    P2 = temp;
//    P2 &= 0x1f;
}