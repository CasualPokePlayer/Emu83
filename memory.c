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
#include "events.h"
#include "link.h"

u8 ReadMem(TI83_t* TI83, u16 addr) {
	return TI83->ReadPtrs[addr >> 14][addr];
}

void WriteMem(TI83_t* TI83, u16 addr, u8 val) {
	TI83->WritePtrs[addr >> 14][addr] = val;
}

static void DispMove(TI83_t* TI83) {
	switch (TI83->DisplayMove) {
		case 0: --TI83->DisplayY; break;
		case 1: ++TI83->DisplayY; break;
		case 2: --TI83->DisplayX; break;
		case 3: ++TI83->DisplayX; break;
	}

	TI83->DisplayX &= 0x0F;
	TI83->DisplayY &= 0x3F;
}

static void SetROMPagePtr(TI83_t* TI83) {
	TI83->ReadPtrs[1] = TI83->ROM + (0x4000 * TI83->ROMPage) - 0x4000;
}

static u32 TimerPeriods[16] = {
	0x2B67, 0x6545, 0x9F24, 0xD903,
	0x2B67, 0x6545, 0x9F24, 0xD903,
	0x2710, 0x5B25, 0x8F3A, 0xC350,
	0x2710, 0x5B25, 0x8F3A, 0xC350,
};

typedef enum {
	PORT_LINK = 0,
	PORT_KEYBOARD = 1,
	PORT_ROMPAGE = 2,
	PORT_STATUS = 3,
	PORT_INTCTRL = 4,
	PORT_DISPCTRL = 16,
	PORT_DISPDATA = 17,
} Port_t;

u8 ReadPort(TI83_t* TI83, u8 port) {
	switch (port) {
		case PORT_LINK:
		case PORT_INTCTRL:
		{
			UpdateLinkPort(TI83);
			return ((TI83->ROMPage & 8) << 1) | (LinkState(TI83) << 2) | TI83->LinkOutput;
		}
		case PORT_KEYBOARD:
		{
			TI83->Lagged = false;
			return TI83->InputCallback ? TI83->InputCallback(TI83->KeyboardMask) : 0xFF;
		}
		case PORT_ROMPAGE:
		{
			return TI83->ROMPage & 0x7;
		}
		case PORT_STATUS:
		{
			return (!TI83->OnPressed << 3) | (TI83->TimerIntPending << 1) | TI83->OnIntPending;
		}
		case PORT_DISPCTRL:
		{
			break; // ???
		}
		case PORT_DISPDATA:
		{
			if (TI83->CursorMoved) {
				TI83->CursorMoved = false;
				return 0x00;
			}

			u8 ret;
			if (TI83->DisplayMode) {
				ret = TI83->VRAM[(TI83->DisplayY * 12) + TI83->DisplayX];
			} else {
				u32 column = 6 * TI83->DisplayX;
				u32 offset = TI83->DisplayY * 12 + (column >> 3);
				u32 shift = 10 - (column & 7);
				ret = ((TI83->VRAM[offset] << 8) | TI83->VRAM[offset + 1]) >> shift;
			}

			DispMove(TI83);
			return ret;
		}
	}

	return 0xFF;
}

void WritePort(TI83_t* TI83, u8 port, u8 val, u64 cycleCount) {
	switch (port) {
		case PORT_LINK:
		{
			TI83->ROMPage = ((val >> 1) & 0x08) | (TI83->ROMPage & 0x07);
			TI83->LinkOutput = val & 0x03;
			SetROMPagePtr(TI83);
			if (TI83->LinkAwaitingResponse && TI83->PC < 0x4000) {
				UpdateLinkPort(TI83);
			}
			break;
		}
		case PORT_KEYBOARD:
		{
			TI83->KeyboardMask = val;
			break;
		}
		case PORT_ROMPAGE:
		{
			TI83->ROMPage = (TI83->ROMPage & 0x08) | (val & 0x07);
			SetROMPagePtr(TI83);
			break;
		}
		case PORT_STATUS:
		{
			if (val & 1) {
				TI83->OnIntEn = true;
			} else {
				TI83->OnIntEn = TI83->OnIntPending = false;
			}

			if (val & 2) {
				TI83->TimerIntEn = true;
				TI83->TimerLastUpdate += TI83->TimerPeriod * ((cycleCount - TI83->TimerLastUpdate) / TI83->TimerPeriod);
				ScheduleEvent(TI83, TIMER_IRQ, TI83->TimerLastUpdate + TI83->TimerPeriod);
			} else {
				TI83->TimerIntEn = TI83->TimerIntPending = false;
				ScheduleEvent(TI83, TIMER_IRQ, EVENT_TIME_NEVER);
			}
			break;
		}
		case PORT_INTCTRL:
		{
			TI83->TimerLastUpdate += TI83->TimerPeriod * ((cycleCount - TI83->TimerLastUpdate) / TI83->TimerPeriod);
			TI83->TimerPeriod = TimerPeriods[(val & 0x1F) >> 1];
			if (TI83->TimerIntEn) {
				ScheduleEvent(TI83, TIMER_IRQ, TI83->TimerLastUpdate + TI83->TimerPeriod);
			}
			break;
		}
		case PORT_DISPCTRL:
		{
			if (val <= 1) {
				TI83->DisplayMode = val;
			} else if (val >= 4 && val <= 7) {
				TI83->DisplayMove = val - 4;
			} else if ((val & 0xC0) == 0x40) {
				// hardware scroll?
			} else if ((val & 0xE0) == 0x20) {
				TI83->DisplayX = val & 0x1F;
				TI83->CursorMoved = true;
			} else if ((val & 0xC0) == 0x80) {
				TI83->DisplayY = val & 0x3F;
				TI83->CursorMoved = true;
			}
			break;
		}
		case PORT_DISPDATA:
		{
			if (TI83->DisplayMode) {
				TI83->VRAM[TI83->DisplayY * 12 + TI83->DisplayX] = val;
			} else {
				u32 column = 6 * TI83->DisplayX;
				u32 offset = TI83->DisplayY * 12 + (column >> 3);
				if (offset < 0x300) {
					u32 shift = column & 7;
					u32 mask = ~(252 >> shift);
					u32 data = val << 2;
					TI83->VRAM[offset] = (TI83->VRAM[offset] & mask) | (data >> shift);
					if (shift > 2 && offset < 0x2FF) {
						++offset;
						shift = 8 - shift;
						mask = ~(252 << shift);
						TI83->VRAM[offset] = (TI83->VRAM[offset] & mask) | (data << shift);
					}
				}
			}
			DispMove(TI83);
			break;
		}
	}
}