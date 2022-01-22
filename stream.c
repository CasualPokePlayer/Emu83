/*
MIT License

Copyright (c) 2022 CasualPokePlayer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "ti83.h"

u32 Stream_Read(Stream_t* stream, u8* buffer, u32 count) {
	u32 streamRm = stream->Length - stream->Index;
	count = count <= streamRm ? count : streamRm;
	memcpy(buffer, stream->Data + stream->Index, count);
	stream->Index += count;
	return count;
}

void Stream_Seek(Stream_t* stream, s32 offset, s32 origin) {
	switch (origin) {
		case SEEK_SET:
		{
			if (offset < 0) {
				stream->Index = 0;
			} else if ((u32)offset > stream->Length) {
				stream->Index = stream->Length;
			} else {
				stream->Index = offset;
			}
			break;
		}
		case SEEK_CUR:
		{
			if ((s32)(stream->Index + offset) < 0) {
				stream->Index = 0;
			} else if (stream->Index + offset > stream->Length) {
				stream->Index = stream->Length;
			} else {
				stream->Index += offset;
			}
			break;
		}
		case SEEK_END:
		{
			if (offset > 0) {
				stream->Index = stream->Length;
			} else if ((u32)(-offset) > stream->Length) {
				stream->Index = 0;
			} else {
				stream->Index += offset;
			}
			break;
		}
	}
}