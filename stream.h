#ifndef STREAM_H
#define STREAM_H

#include "ti83.h"

u32 Stream_Read(Stream_t* stream, u8* buffer, u32 count);
void Stream_Seek(Stream_t* stream, s32 offset, s32 origin);

#endif
