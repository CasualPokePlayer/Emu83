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
#include "stream.h"
#include "queue.h"

static void SendNextFile(TI83_t* TI83) {
	u8 header[13];
	if (Stream_Read(&TI83->LinkFiles[TI83->CurrentLinkFile], header, 13) != 13) {
		++TI83->CurrentLinkFile;
		TI83->VariableData.Index = false;
		return;
	}

	u16 size = (header[3] << 8) | header[2];
	if (TI83->LinkFiles[TI83->CurrentLinkFile].Index + size + 2 > TI83->LinkFiles[TI83->CurrentLinkFile].Length) {
		++TI83->CurrentLinkFile;
		TI83->VariableData.Index = false;
		return;
	}
	TI83->VariableData.Data = TI83->LinkFiles[TI83->CurrentLinkFile].Data + TI83->LinkFiles[TI83->CurrentLinkFile].Index;
	TI83->VariableData.Length = size + 2;
	TI83->VariableData.Index = true;
	TI83->LinkFiles[TI83->CurrentLinkFile].Index += size + 2;

	Queue_Clear(&TI83->CurrentLinkData);

	Queue_Enqueue(&TI83->CurrentLinkData, 0x03);
	Queue_Enqueue(&TI83->CurrentLinkData, 0xC9);
	for (u32 i = 0; i < sizeof (header); i++) {
		Queue_Enqueue(&TI83->CurrentLinkData, header[i]);
	}

	u16 checksum = 0;
	for (u32 i = 2; i < sizeof (header); i++) {
		checksum += header[i];
	}

	Queue_Enqueue(&TI83->CurrentLinkData, checksum & 0xFF);
	Queue_Enqueue(&TI83->CurrentLinkData, checksum >> 8);

	TI83->LinkStatus = LINK_PREP_RECEIVE;
	TI83->LinkActionId = ACTION_RECEIVE_REQ_ACK;
	TI83->LinkAwaitingResponse = true;
}

static void ReceiveReqAck(TI83_t* TI83) {
	TI83->LinkAwaitingResponse = false;
	Queue_Clear(&TI83->CurrentLinkData);

	TI83->LinkBytesLeft = 8;
	TI83->LinkStatus = LINK_PREP_SEND;
	TI83->LinkActionId = ACTION_SEND_VARIABLE_DATA;
}

static void SendVariableData(TI83_t* TI83) {
	Queue_Dequeue(&TI83->CurrentLinkData);
	Queue_Dequeue(&TI83->CurrentLinkData);
	Queue_Dequeue(&TI83->CurrentLinkData);
	Queue_Dequeue(&TI83->CurrentLinkData);
	Queue_Dequeue(&TI83->CurrentLinkData);

	if (Queue_Dequeue(&TI83->CurrentLinkData) == 0x36) {
		TI83->LinkAwaitingResponse = false;
		Queue_Clear(&TI83->CurrentLinkData);

		TI83->LinkBytesLeft = 3;
		TI83->LinkStatus = LINK_PREP_SEND;
		TI83->LinkActionId = ACTION_END_OUT_OF_MEMORY;
	} else {
		Queue_Clear(&TI83->CurrentLinkData);

		Queue_Enqueue(&TI83->CurrentLinkData, 0x03);
		Queue_Enqueue(&TI83->CurrentLinkData, 0x56);
		Queue_Enqueue(&TI83->CurrentLinkData, 0x00);
		Queue_Enqueue(&TI83->CurrentLinkData, 0x00);

		Queue_Enqueue(&TI83->CurrentLinkData, 0x03);
		Queue_Enqueue(&TI83->CurrentLinkData, 0x15);

		for (u32 i = 0; i < TI83->VariableData.Length; i++) {
			Queue_Enqueue(&TI83->CurrentLinkData, TI83->VariableData.Data[i]);
		}

		u16 checksum = 0;
		for (u32 i = 2; i < TI83->VariableData.Length; i++) {
			checksum += TI83->VariableData.Data[i];
		}

		Queue_Enqueue(&TI83->CurrentLinkData, checksum & 0xFF);
		Queue_Enqueue(&TI83->CurrentLinkData, checksum >> 8);

		TI83->LinkStatus = LINK_PREP_RECEIVE;
		TI83->LinkActionId = ACTION_RECEIVE_DATA_ACK;
		TI83->LinkAwaitingResponse = true;
	}
}

static void ReceiveDataAck(TI83_t* TI83) {
	TI83->LinkAwaitingResponse = false;
	Queue_Clear(&TI83->CurrentLinkData);

	TI83->LinkBytesLeft = 4;
	TI83->LinkStatus = LINK_PREP_SEND;
	TI83->LinkActionId = ACTION_END_TRANSMISSION;
}

static void EndTransmission(TI83_t* TI83) {
	Queue_Clear(&TI83->CurrentLinkData);

	Queue_Enqueue(&TI83->CurrentLinkData, 0x03);
	Queue_Enqueue(&TI83->CurrentLinkData, 0x92);
	Queue_Enqueue(&TI83->CurrentLinkData, 0x00);
	Queue_Enqueue(&TI83->CurrentLinkData, 0x00);

	TI83->LinkStatus = LINK_PREP_RECEIVE;
	TI83->LinkActionId = ACTION_FINALIZE_FILE;
	TI83->LinkAwaitingResponse = true;
}

static void EndOutOfMemory(TI83_t* TI83) {
	Queue_Clear(&TI83->CurrentLinkData);

	Queue_Enqueue(&TI83->CurrentLinkData, 0x03);
	Queue_Enqueue(&TI83->CurrentLinkData, 0x56);
	Queue_Enqueue(&TI83->CurrentLinkData, 0x01);
	Queue_Enqueue(&TI83->CurrentLinkData, 0x00);

	TI83->LinkStatus = LINK_PREP_RECEIVE;
	TI83->LinkActionId = ACTION_FINALIZE_FILE;
	TI83->LinkAwaitingResponse = true;
}

static void FinalizeFile(TI83_t* TI83) {
	Queue_Clear(&TI83->CurrentLinkData);
	TI83->LinkAwaitingResponse = false;
	TI83->LinkActionId = ACTION_DO_NOTHING;
	SendNextFile(TI83);
}

static void DoNothing(TI83_t* TI83) {
	(void)TI83;
}

typedef void (*LinkAction_t)(TI83_t* TI83);

static LinkAction_t LinkActions[7] = {
	ReceiveReqAck, // ACTION_RECEIVE_REQ_ACK
	SendVariableData, // ACTION_SEND_VARIABLE_DATA
	ReceiveDataAck, // ACTION_RECEIVE_DATA_ACK
	EndTransmission, // ACTION_END_TRANSMISSION
	EndOutOfMemory, // ACTION_END_OUT_OF_MEMORY
	FinalizeFile, // ACTION_FINALIZE_FILE
	DoNothing, // ACTION_DO_NOTHING
};

u8 LinkState(TI83_t* TI83) {
	return (TI83->LinkOutput | TI83->LinkInput) ^ 3;
}

void UpdateLinkPort(TI83_t* TI83) {
	if (TI83->LinkStatus == LINK_PREP_RECEIVE) {
		TI83->CurrentLinkByte = Queue_Dequeue(&TI83->CurrentLinkData);
		TI83->LinkStatus = LINK_RECEIVE;
		TI83->LinkBitsLeft = 8;
		TI83->LinkStepsLeft = 5;
	}

	if (TI83->LinkStatus == LINK_PREP_SEND && LinkState(TI83) != 3) {
		TI83->LinkStatus = LINK_SEND;
		TI83->LinkBitsLeft = 8;
		TI83->LinkStepsLeft = 5;
		TI83->CurrentLinkByte = 0;
	}

	if (TI83->LinkStatus == LINK_RECEIVE) {
		switch (TI83->LinkStepsLeft) {
			case 5:
			{
				TI83->LinkInput = (TI83->CurrentLinkByte & 1) + 1;
				TI83->CurrentLinkByte >>= 1;
				--TI83->LinkStepsLeft;
				break;
			}
			case 4:
			{
				if (!(LinkState(TI83) & 3)) {
					--TI83->LinkStepsLeft;
				}
				break;
			}
			case 3:
			{
				TI83->LinkInput = 0;
				--TI83->LinkStepsLeft;
				break;
			}
			case 2:
			{
				if ((LinkState(TI83) & 3) == 3) {
					--TI83->LinkStepsLeft;
				}
				break;
			}
			case 1:
			{
				if (--TI83->LinkBitsLeft) {
					TI83->LinkStepsLeft = 5;
				} else {
					if (TI83->CurrentLinkData.Index) {
						TI83->LinkStatus = LINK_PREP_RECEIVE;
					} else {
						TI83->LinkStatus = LINK_INACTIVE;
						LinkActions[TI83->LinkActionId](TI83);
					}
				}
				break;
			}
		}
	} else if (TI83->LinkStatus == LINK_SEND) {
		switch (TI83->LinkStepsLeft) {
			case 5:
			{
				if (LinkState(TI83) != 3) {
					u8 bit = LinkState(TI83) & 1;
					u8 shift = 8 - TI83->LinkBitsLeft;
					TI83->CurrentLinkByte |= bit << shift;
					--TI83->LinkStepsLeft;
				}
				break;
			}
			case 4:
			{
				TI83->LinkInput = TI83->LinkOutput ^ 3;
				--TI83->LinkStepsLeft;
				break;
			}
			case 3:
			{
				if (!(TI83->LinkOutput & 3)) {
					--TI83->LinkStepsLeft;
				}
				break;
			}
			case 2:
			{
				TI83->LinkInput = 0;
				--TI83->LinkStepsLeft;
				break;
			}
			case 1:
			{
				if (--TI83->LinkBitsLeft) {
					TI83->LinkStepsLeft = 5;
				} else {
					Queue_Enqueue(&TI83->CurrentLinkData, TI83->CurrentLinkByte);
					if (--TI83->LinkBytesLeft) {
						TI83->LinkStatus = LINK_PREP_SEND;
					} else {
						TI83->LinkStatus = LINK_INACTIVE;
						LinkActions[TI83->LinkActionId](TI83);
					}
				}
				break;
			}
		}
	}
}

void SendNextLinkFile(TI83_t* TI83) {
	if (TI83->LinkStatus == LINK_INACTIVE && TI83->LinkFiles[TI83->CurrentLinkFile].Data) {
		Stream_Seek(&TI83->LinkFiles[TI83->CurrentLinkFile], 55, SEEK_SET);
		SendNextFile(TI83);
	}
}