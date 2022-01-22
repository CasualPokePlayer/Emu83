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
#include "z80.h"
#include "memory.h"
#include "events.h"
#include "link.h"
#include "savestate.h"

#define EXPORT __attribute__((visibility("default")))

EXPORT TI83_t* TI83_CreateContext(u8* ROMData, u32 ROMSize) {
	if (ROMSize > 0x40000) {
		return NULL;
	}
	TI83_t* TI83 = calloc(1, sizeof (TI83_t));
	if (!TI83) {
		return NULL;
	}
	memcpy(TI83->ROM, ROMData, ROMSize);
	u32 paddingSize = sizeof (TI83->ROM) - ROMSize;
	if (paddingSize) {
		memset(TI83->ROM + ROMSize, 0xFF, paddingSize);
	}
	memset(TI83->RAM, 0xFF, sizeof (TI83->RAM));
	TI83->ReadPtrs[0] = TI83->ROM;
	TI83->WritePtrs[0] = TI83->DisabledWritePage;
	TI83->ReadPtrs[1] = TI83->ROM - 0x4000;
	TI83->WritePtrs[1] = TI83->DisabledWritePage - 0x4000;
	TI83->ReadPtrs[2] = TI83->WritePtrs[2] = TI83->ReadPtrs[3] = TI83->WritePtrs[3] = TI83->RAM - 0x8000;
	TI83->IM = 1;
	TI83->CurrentLinkData.Data = malloc(0x1000);
	if (!TI83->CurrentLinkData.Data) {
		free(TI83);
		return NULL;
	}
	TI83->CurrentLinkData.Length = 0x1000;
	TI83->LinkStatus = LINK_INACTIVE;
	TI83->LinkActionId = ACTION_DO_NOTHING;
	TI83->TimerPeriod = 0x2B67;
	memset(TI83->EventSchedule, 0xFF, sizeof (TI83->EventSchedule));
	TI83->NextEventId = NUM_EVENTS;
	TI83->NextEventTime = EVENT_TIME_NEVER;
	return TI83;
}

EXPORT void TI83_DestroyContext(TI83_t* TI83) {
	for (u32 i = 0; i < 256; i++) {
		free(TI83->LinkFiles[i].Data);
	}
	free(TI83->CurrentLinkData.Data);
	free(TI83);
}

EXPORT bool TI83_LoadLinkFile(TI83_t* TI83, u8* linkFile, u32 len) {
	if (TI83->LinkFilesAreLoaded || TI83->CurrentLinkFile == 0xFF) {
		return false;
	}
	u8* buffer = malloc(len);
	if (!buffer) {
		return false;
	}
	memcpy(buffer, linkFile, len);
	TI83->LinkFiles[TI83->CurrentLinkFile].Data = buffer;
	TI83->LinkFiles[TI83->CurrentLinkFile].Length = len;
	TI83->LinkFiles[TI83->CurrentLinkFile].Index = 0;
	++TI83->CurrentLinkFile;
	return true;
}

EXPORT void TI83_SetLinkFilesAreLoaded(TI83_t* TI83) {
	TI83->LinkFilesAreLoaded = true;
	TI83->CurrentLinkFile = 0;
}

EXPORT bool TI83_GetLinkActive(TI83_t* TI83) {
	return TI83->LinkStatus != LINK_INACTIVE;
}

EXPORT bool TI83_Advance(TI83_t* TI83, bool onPressed, bool sendNextLinkFile, u32* videoBuffer, u32 backgroundColor, u32 foreColor) {
	TI83->Lagged = true;
	TI83->OnPressed = onPressed;
	if (onPressed && TI83->OnIntEn && !TI83->OnIntPending) {
		TI83->OnIntPending = true;
		if (TI83->IFF) {
			ScheduleEvent(TI83, INTERRUPT, EVENT_TIME_NOW);
		}
	}
	if (sendNextLinkFile) {
		SendNextLinkFile(TI83);
	}
	RunFrame(TI83);
	if (videoBuffer) {
		for (u32 i = 0; i < (96 * 64); i++) {
			u8 bit = TI83->VRAM[i >> 3] & (0x80 >> (i & 7));
			videoBuffer[i] = bit ? foreColor : backgroundColor;
		}
	}
	return TI83->Lagged;
}

EXPORT u64 TI83_GetStateSize(void) {
	return StateSize();
}

EXPORT bool TI83_SaveState(TI83_t* TI83, void* buf) {
	return SaveState(TI83, buf);
}

EXPORT bool TI83_LoadState(TI83_t* TI83, void* buf) {
	return LoadState(TI83, buf);
}

EXPORT void TI83_GetRegs(TI83_t* TI83, u32* buf) {
	buf[0] = TI83->MainRegs.AF;
	buf[1] = TI83->MainRegs.BC;
	buf[2] = TI83->MainRegs.DE;
	buf[3] = TI83->MainRegs.HL;
	buf[4] = TI83->AltRegs.AF;
	buf[5] = TI83->AltRegs.BC;
	buf[6] = TI83->AltRegs.DE;
	buf[7] = TI83->AltRegs.HL;
	buf[8] = TI83->IX;
	buf[9] = TI83->IY;
	buf[10] = TI83->PC;
	buf[11] = TI83->SP;
}

EXPORT bool TI83_GetMemoryArea(TI83_t* TI83, MemoryArea_t which, void** ptr, u32* len) {
	switch (which) {
		case MEM_ROM:
			if (ptr) *ptr = TI83->ROM;
			if (len) *len = sizeof (TI83->ROM);
			return true;
		case MEM_RAM:
			if (ptr) *ptr = TI83->RAM;
			if (len) *len = sizeof (TI83->RAM);
			return true;
		case MEM_VRAM:
			if (ptr) *ptr = TI83->VRAM;
			if (len) *len = sizeof (TI83->VRAM);
			return true;
	}

	return false;
}

EXPORT u8 TI83_ReadMemory(TI83_t* TI83, u16 addr) {
	return ReadMem(TI83, addr);
}

EXPORT void TI83_WriteMemory(TI83_t* TI83, u16 addr, u8 val) {
	WriteMem(TI83, addr, val);
}

EXPORT u64 TI83_GetCycleCount(TI83_t* TI83) {
	return TI83->CycleCount;
}

EXPORT void TI83_SetMemoryCallback(TI83_t* TI83, MemoryCallbackId_t id, MemoryCallback_t callback) {
	switch (id) {
		case MEMORY_CB_READ: TI83->ReadCallback = callback; break;
		case MEMORY_CB_WRITE: TI83->WriteCallback = callback; break;
		case MEMORY_CB_EXECUTE: TI83->ExecuteCallback = callback; break;
	}
}

EXPORT void TI83_SetTraceCallback(TI83_t* TI83, TraceCallback_t callback) {
	TI83->TraceCallback = callback;
}

EXPORT void TI83_SetInputCallback(TI83_t* TI83, InputCallback_t callback) {
	TI83->InputCallback = callback;
}
