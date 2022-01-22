#ifndef MEMORY_H
#define MEMORY_H

#include "ti83.h"

u8 ReadMem(TI83_t* TI83, u16 addr);
void WriteMem(TI83_t* TI83, u16 addr, u8 val);

u8 ReadPort(TI83_t* TI83, u8 port);
void WritePort(TI83_t* TI83, u8 port, u8 val, u64 cycleCount);

#endif
