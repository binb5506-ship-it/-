#include <STC15F2K60S2.H>
unsigned char Ad_Read(unsigned char addr);
void Da_Write(unsigned char dat);
void EEPROM_Write(unsigned char*str,unsigned char addr,unsigned num) ;  /* 原来多写了一个R：EERROM → EEPROM */
void EEPROM_Read(unsigned char*str,unsigned char addr,unsigned num);