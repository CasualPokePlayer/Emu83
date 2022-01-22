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