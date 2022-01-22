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
#include "crc32.h"
#include "queue.h"

#define PACKED __attribute__((packed))

typedef struct {
	u8 FileExists;
	u32 Length;
	u32 Index;
} PACKED StreamState_t;

typedef struct {
	u8 Data[8]; // data sent by the calculator, or finalize file data
	u32 Length;
	u32 Index;
} PACKED QueueState_t;

typedef struct {
	u16 AF;
	u16 BC;
	u16 DE;
	u16 HL;
} PACKED RegisterState_t;

typedef struct {
	u32 ROMCRC;
	u8 RAM[0x8000];
	u8 VRAM[0x300];

	u8 ROMPage;

	RegisterState_t MainRegs;
	RegisterState_t AltRegs;

	u16 IX;
	u16 IY;
	u16 PC;
	u16 SP;
	u16 WZ;

	u8 I, R, IM;
	u8 IFF;
	u8 OnIntEn, TimerIntEn;
	u8 OnIntPending, TimerIntPending;

	u8 Halted;

	u8 CursorMoved;
	u8 DisplayMode;
	u8 DisplayMove;
	u8 DisplayX, DisplayY;

	u8 KeyboardMask;

	u32 LinkFileCRCs[256];
	StreamState_t LinkFiles[256];
	u8 CurrentLinkFile;
	QueueState_t CurrentLinkData;
	StreamState_t VariableData;
	u8 LinkStatus;
	u8 CurrentLinkByte;
	u8 LinkBytesLeft, LinkBitsLeft, LinkStepsLeft;
	u8 LinkActionId;
	u8 LinkInput, LinkOutput;
	u8 LinkAwaitingResponse;

	u64 TimerLastUpdate;
	u32 TimerPeriod;

	u64 EventSchedule[NUM_EVENTS];

	u8 NextEventId;
	u64 NextEventTime;

	u64 CycleCount;
} PACKED TI83State_t;

u64 StateSize(void) {
	return sizeof (TI83State_t);
}

bool SaveState(TI83_t* TI83, void* buf) {
	if (!TI83->LinkFilesAreLoaded) {
		return false;
	}

	TI83State_t TI83State;

	TI83State.ROMCRC = CRC32(TI83->ROM, sizeof (TI83->ROM));
	memcpy(TI83State.RAM, TI83->RAM, sizeof (TI83->RAM));
	memcpy(TI83State.VRAM, TI83->VRAM, sizeof (TI83->VRAM));

	TI83State.ROMPage = TI83->ROMPage;

	TI83State.MainRegs.AF = TI83->MainRegs.AF;
	TI83State.MainRegs.BC = TI83->MainRegs.BC;
	TI83State.MainRegs.DE = TI83->MainRegs.DE;
	TI83State.MainRegs.HL = TI83->MainRegs.HL;

	TI83State.AltRegs.AF = TI83->AltRegs.AF;
	TI83State.AltRegs.BC = TI83->AltRegs.BC;
	TI83State.AltRegs.DE = TI83->AltRegs.DE;
	TI83State.AltRegs.HL = TI83->AltRegs.HL;

	TI83State.IX = TI83->IX;
	TI83State.IY = TI83->IY;
	TI83State.PC = TI83->PC;
	TI83State.SP = TI83->SP;
	TI83State.WZ = TI83->WZ;

	TI83State.I = TI83->I;
	TI83State.R = TI83->R;
	TI83State.IM = TI83->IM;
	TI83State.IFF = TI83->IFF;
	TI83State.OnIntEn = TI83->OnIntEn;
	TI83State.TimerIntEn = TI83->TimerIntEn;
	TI83State.OnIntPending = TI83->OnIntPending;
	TI83State.TimerIntPending = TI83->TimerIntPending;

	TI83State.Halted = TI83->Halted;

	TI83State.CursorMoved = TI83->CursorMoved;
	TI83State.DisplayMode = TI83->DisplayMode;
	TI83State.DisplayMove = TI83->DisplayMove;
	TI83State.DisplayX = TI83->DisplayX;
	TI83State.DisplayY = TI83->DisplayY;

	TI83State.KeyboardMask = TI83->KeyboardMask;

	for (u32 i = 0; i < 256; i++) {
		bool fileExists = TI83->LinkFiles[i].Data;
		TI83State.LinkFileCRCs[i] = fileExists ? CRC32(TI83->LinkFiles[i].Data, TI83->LinkFiles[i].Length) : 0;
		TI83State.LinkFiles[i].FileExists = fileExists;
		TI83State.LinkFiles[i].Length = fileExists ? TI83->LinkFiles[i].Length : 0;
		TI83State.LinkFiles[i].Index = fileExists ? TI83->LinkFiles[i].Index : 0;
	}
	TI83State.CurrentLinkFile = TI83->CurrentLinkFile;
	memset(TI83State.CurrentLinkData.Data, 0, sizeof (TI83State.CurrentLinkData.Data));
	if (TI83->LinkStatus == LINK_PREP_SEND || TI83->LinkStatus == LINK_SEND || TI83->LinkActionId == ACTION_FINALIZE_FILE) {
		memcpy(TI83State.CurrentLinkData.Data, TI83->CurrentLinkData.Data, TI83->CurrentLinkData.Index);
	}
	TI83State.CurrentLinkData.Length = TI83->CurrentLinkData.Length;
	TI83State.CurrentLinkData.Index = TI83->CurrentLinkData.Index;
	bool variableDataExists = TI83->VariableData.Index;
	TI83State.VariableData.FileExists = variableDataExists;
	TI83State.VariableData.Length = variableDataExists ? TI83->VariableData.Length : 0;
	TI83State.VariableData.Index = variableDataExists ? TI83->VariableData.Data - TI83->LinkFiles[TI83->CurrentLinkFile].Data : 0;
	TI83State.LinkStatus = TI83->LinkStatus;
	TI83State.CurrentLinkByte = TI83->CurrentLinkByte;
	TI83State.LinkBytesLeft = TI83->LinkBytesLeft;
	TI83State.LinkBitsLeft = TI83->LinkBitsLeft;
	TI83State.LinkStepsLeft = TI83->LinkStepsLeft;
	TI83State.LinkActionId = TI83->LinkActionId;
	TI83State.LinkInput = TI83->LinkInput;
	TI83State.LinkOutput = TI83->LinkOutput;
	TI83State.LinkAwaitingResponse = TI83->LinkAwaitingResponse;

	TI83State.TimerLastUpdate = TI83->TimerLastUpdate;
	TI83State.TimerPeriod = TI83->TimerPeriod;

	memcpy(TI83State.EventSchedule, TI83->EventSchedule, sizeof (TI83->EventSchedule));

	TI83State.NextEventId = TI83->NextEventId;
	TI83State.NextEventTime = TI83->NextEventTime;

	TI83State.CycleCount = TI83->CycleCount;

	memcpy(buf, &TI83State, StateSize());
	return true;
}

bool LoadState(TI83_t* TI83, void* buf) {
	if (!TI83->LinkFilesAreLoaded) {
		return false;
	}

	TI83State_t* TI83State = buf;

	if (TI83State->ROMCRC != CRC32(TI83->ROM, sizeof (TI83->ROM))) {
		return false;
	}

	for (u32 i = 0; i < 256; i++) {
		bool stateLinkFileExists = TI83State->LinkFiles[i].FileExists;
		bool actualLinkFileExists = TI83->LinkFiles[i].Data;
		if (stateLinkFileExists ^ actualLinkFileExists) {
			return false;
		}

		if (TI83State->LinkFiles[i].Length != TI83->LinkFiles[i].Length) {
			return false;
		}

		if (actualLinkFileExists) {
			if (TI83State->LinkFileCRCs[i] != CRC32(TI83->LinkFiles[i].Data, TI83->LinkFiles[i].Length)) {
				return false;
			}
			if (TI83State->LinkFiles[i].Length < TI83State->LinkFiles[i].Index) {
				return false;
			}
		} else {
			if (TI83State->LinkFileCRCs[i] || TI83->LinkFiles[i].Length || TI83->LinkFiles[i].Index) {
				return false;
			}
		}
	}

	if (TI83State->ROMPage & 0xF0) {
		return false;
	}

	if (TI83State->LinkActionId > ACTION_DO_NOTHING) {
		return false;
	}

	memcpy(TI83->RAM, TI83State->RAM, sizeof (TI83->RAM));
	memcpy(TI83->VRAM, TI83State->VRAM, sizeof (TI83->VRAM));

	TI83->ROMPage = TI83State->ROMPage;
	TI83->ReadPtrs[1] = TI83->ROM + (0x4000 * TI83->ROMPage) - 0x4000;

	TI83->MainRegs.AF = TI83State->MainRegs.AF;
	TI83->MainRegs.BC = TI83State->MainRegs.BC;
	TI83->MainRegs.DE = TI83State->MainRegs.DE;
	TI83->MainRegs.HL = TI83State->MainRegs.HL;

	TI83->AltRegs.AF = TI83State->AltRegs.AF;
	TI83->AltRegs.BC = TI83State->AltRegs.BC;
	TI83->AltRegs.DE = TI83State->AltRegs.DE;
	TI83->AltRegs.HL = TI83State->AltRegs.HL;

	TI83->IX = TI83State->IX;
	TI83->IY = TI83State->IY;
	TI83->PC = TI83State->PC;
	TI83->SP = TI83State->SP;
	TI83->WZ = TI83State->WZ;

	TI83->I = TI83State->I;
	TI83->R = TI83State->R;
	TI83->IM = TI83State->IM;
	TI83->IFF = TI83State->IFF;
	TI83->OnIntEn = TI83State->OnIntEn;
	TI83->TimerIntEn = TI83State->TimerIntEn;
	TI83->OnIntPending = TI83State->OnIntPending;
	TI83->TimerIntPending = TI83State->TimerIntPending;

	TI83->Halted = TI83State->Halted;

	TI83->CursorMoved = TI83State->CursorMoved;
	TI83->DisplayMode = TI83State->DisplayMode;
	TI83->DisplayMove = TI83State->DisplayMove;
	TI83->DisplayX = TI83State->DisplayX;
	TI83->DisplayY = TI83State->DisplayY;

	TI83->KeyboardMask = TI83State->KeyboardMask;

	for (u32 i = 0; i < 256; i++) {
		TI83->LinkFiles[i].Index = TI83State->LinkFiles[i].Index;
	}
	TI83->CurrentLinkFile = TI83State->CurrentLinkFile;
	TI83->CurrentLinkData.Data = realloc(TI83->CurrentLinkData.Data, TI83State->CurrentLinkData.Length);
	TI83->CurrentLinkData.Length = TI83State->CurrentLinkData.Length;
	TI83->CurrentLinkData.Index = TI83State->CurrentLinkData.Index;
	bool variableDataExists = TI83State->VariableData.FileExists;
	TI83->VariableData.Data = variableDataExists ? TI83->LinkFiles[TI83->CurrentLinkFile].Data + TI83State->VariableData.Index : NULL;
	TI83->VariableData.Length = variableDataExists ? TI83State->VariableData.Length : 0;
	TI83->VariableData.Index = variableDataExists;
	if (TI83State->LinkStatus == LINK_PREP_SEND || TI83State->LinkStatus == LINK_SEND || TI83->LinkActionId == ACTION_FINALIZE_FILE) {
		memcpy(TI83->CurrentLinkData.Data, TI83State->CurrentLinkData.Data, TI83->CurrentLinkData.Index);
	} else if (TI83State->LinkActionId == ACTION_RECEIVE_REQ_ACK) {
		u8 data[2 + 13 + 2];
		data[0] = 0x03;
		data[1] = 0xC9;
		u8* header = TI83->LinkFiles[TI83->CurrentLinkFile].Data + TI83->LinkFiles[TI83->CurrentLinkFile].Index - TI83->VariableData.Length - 13;
		for (u32 i = 0; i < 13; i++) {
			data[i+2] = header[i];
		}
		u16 checksum = 0;
		for (u32 i = 2; i < 13; i++) {
			checksum += data[i + 2];
		}
		data[15] = checksum & 0xFF;
		data[16] = checksum >> 8;
		u32 len = TI83->CurrentLinkData.Index;
		memcpy(TI83->CurrentLinkData.Data, data + sizeof (data) - len, len);
	} else if (TI83State->LinkActionId == ACTION_RECEIVE_DATA_ACK) {
		u8 data[4 + 2 + TI83->VariableData.Length + 2];
		data[0] = 0x03;
		data[1] = 0x56;
		data[2] = 0x00;
		data[3] = 0x00;
		data[4] = 0x03;
		data[5] = 0x15;
		memcpy(data + 4 + 2, TI83->VariableData.Data, TI83->VariableData.Length);
		u16 checksum = 0;
		for (u32 i = 2; i < TI83->VariableData.Length; i++) {
			checksum += data[i + 4 + 2];
		}
		data[4 + 2 + TI83->VariableData.Length] = checksum & 0xFF;
		data[4 + 2 + TI83->VariableData.Length + 1] = checksum >> 8;
		u32 len = TI83->CurrentLinkData.Index;
		memcpy(TI83->CurrentLinkData.Data, data + sizeof (data) - len, len);
	}
	TI83->LinkStatus = TI83State->LinkStatus;
	TI83->CurrentLinkByte = TI83State->CurrentLinkByte;
	TI83->LinkBytesLeft = TI83State->LinkBytesLeft;
	TI83->LinkBitsLeft = TI83State->LinkBitsLeft;
	TI83->LinkStepsLeft = TI83State->LinkStepsLeft;
	TI83->LinkActionId = TI83State->LinkActionId;
	TI83->LinkInput = TI83State->LinkInput;
	TI83->LinkOutput = TI83State->LinkOutput;
	TI83->LinkAwaitingResponse = TI83State->LinkAwaitingResponse;

	TI83->TimerLastUpdate = TI83State->TimerLastUpdate;
	TI83->TimerPeriod = TI83State->TimerPeriod;

	memcpy(TI83->EventSchedule, TI83State->EventSchedule, sizeof (TI83->EventSchedule));

	TI83->NextEventId = TI83State->NextEventId;
	TI83->NextEventTime = TI83State->NextEventTime;

	TI83->CycleCount = TI83State->CycleCount;

	return true;
}
