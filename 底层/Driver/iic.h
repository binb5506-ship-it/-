#include <STC15F2K60S2.H>
unsigned char Ad_read(unsigned char addr);
void Da_Write(unsigned char dat);
void EEprom_write(unsigned char *str,unsigned char addr  ,unsigned num);
void EEprom_read(unsigned char *str,unsigned char addr  ,unsigned num);