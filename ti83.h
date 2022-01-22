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

#ifndef TI83_H
#define TI83_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define typeof __typeof__

#define EVENT_TIME_NOW 0
#define EVENT_TIME_NEVER 0xFFFFFFFFFFFFFFFFull

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef union {
	struct {
		u16 AF;
		u16 BC;
		u16 DE;
		u16 HL;
	};
	struct {
		u8 F, A;
		u8 C, B;
		u8 E, D;
		u8 L, H;
	};
} Registers_t;

typedef enum {
	TIMER_IRQ,
	INTERRUPT,
	END_FRAME,
	NUM_EVENTS,
} EventId_t;

typedef enum {
	MEM_ROM,
	MEM_RAM,
	MEM_VRAM,
} MemoryArea_t;

typedef enum {
	LINK_INACTIVE,
	LINK_PREP_RECEIVE,
	LINK_PREP_SEND,
	LINK_RECEIVE,
	LINK_SEND,
} LinkStatus_t;

typedef enum {
	ACTION_RECEIVE_REQ_ACK,
	ACTION_SEND_VARIABLE_DATA,
	ACTION_RECEIVE_DATA_ACK,
	ACTION_END_TRANSMISSION,
	ACTION_END_OUT_OF_MEMORY,
	ACTION_FINALIZE_FILE,
	ACTION_DO_NOTHING,
} LinkActionId_t;

typedef struct {
	u8* Data;
	u32 Length;
	u32 Index;
} Stream_t;

typedef Stream_t Queue_t;

typedef enum {
	MEM_CB_READ,
	MEM_CB_WRITE,
	MEM_CB_EXECUTE,
} MemoryCallbackId_t;

typedef void (*MemoryCallback_t)(u16 addr, u64 cycleCount);
typedef void (*TraceCallback_t)(u64 cycleCount);
typedef u8 (*InputCallback_t)(u8 keyboardMask);

typedef struct {
	u8 ROM[0x40000];
	u8 RAM[0x8000];
	u8 VRAM[0x300];
	u8 DisabledWritePage[0x4000];

	u8* ReadPtrs[4];
	u8* WritePtrs[4];

	u8 ROMPage;

	Registers_t MainRegs;
	Registers_t AltRegs;

	union {
		struct {
			u16 IX;
			u16 IY;
			u16 PC;
			u16 SP;
		};
		struct {
			u8 IXL, IXH;
			u8 IYL, IYH;
			u8 PCL, PCH;
			u8 SPL, SPH;
		};
	};

	union {
		u16 WZ;
		struct {
			u8 Z, W;
		};
	};

	u8 I, R, IM;
	bool IFF; // technically two exist, but this doesn't matter as the TI83 has no NMI
	bool OnIntEn, TimerIntEn;
	bool OnIntPending, TimerIntPending;
	bool OnPressed;

	bool Halted;
	bool Lagged;

	MemoryCallback_t ReadCallback;
	MemoryCallback_t WriteCallback;
	MemoryCallback_t ExecuteCallback;

	TraceCallback_t TraceCallback;

	bool CursorMoved;
	bool DisplayMode;
	u8 DisplayMove;
	u8 DisplayX, DisplayY;

	InputCallback_t InputCallback;
	u8 KeyboardMask;

	Stream_t LinkFiles[256]; // nobody should need more than 255 files sent, right?
	u8 CurrentLinkFile;
	Queue_t CurrentLinkData;
	Stream_t VariableData;
	LinkStatus_t LinkStatus;
	u8 CurrentLinkByte;
	u8 LinkBytesLeft, LinkBitsLeft, LinkStepsLeft;
	LinkActionId_t LinkActionId;
	u8 LinkInput, LinkOutput;
	bool LinkAwaitingResponse;
	bool LinkFilesAreLoaded;

	u64 TimerLastUpdate;
	u32 TimerPeriod;

	u64 EventSchedule[NUM_EVENTS];

	EventId_t NextEventId;
	u64 NextEventTime;

	u64 CycleCount;
} TI83_t;


TI83_t* TI83_CreateContext(u8* ROMData, u32 ROMSize);
void TI83_DestroyContext(TI83_t* TI83);
bool TI83_LoadLinkFile(TI83_t* TI83, u8* linkFile, u32 len);
void TI83_SetLinkFilesAreLoaded(TI83_t* TI83);
bool TI83_GetLinkActive(TI83_t* TI83);
bool TI83_Advance(TI83_t* TI83, bool onPressed, bool sendNextLinkFile, u32* videoBuffer, u32 backgroundColor, u32 foreColor);
u64 TI83_GetStateSize(void);
bool TI83_SaveState(TI83_t* TI83, void* buf);
bool TI83_LoadState(TI83_t* TI83, void* buf);
void TI83_GetRegs(TI83_t* TI83, u32* buf);
bool TI83_GetMemoryArea(TI83_t* TI83, MemoryArea_t which, void** ptr, u32* len);
u8 TI83_ReadMemory(TI83_t* TI83, u16 addr);
void TI83_WriteMemory(TI83_t* TI83, u16 addr, u8 val);
u64 TI83_TotalCyclesExecuted(TI83_t* TI83);
void TI83_SetMemoryCallback(TI83_t* TI83, MemoryCallbackId_t id, MemoryCallback_t callback);
void TI83_SetTraceCallback(TI83_t* TI83, TraceCallback_t callback);
void TI83_SetInputCallback(TI83_t* TI83, InputCallback_t callback);

#endif
