#include "ti83.h"
#include "memory.h"

void ScheduleEvent(TI83_t* TI83, EventId_t eventId, u64 newEventTime) {
	TI83->EventSchedule[eventId] = newEventTime;

	// schedule the next event
	EventId_t nextEventId = NUM_EVENTS;
	u64 nextEventTime = EVENT_TIME_NEVER;
	for (u32 i = 0; i < NUM_EVENTS; i++) {
		if (TI83->EventSchedule[i] < nextEventTime) {
			nextEventId = i;
			nextEventTime = TI83->EventSchedule[i];
		}
	}
	TI83->NextEventId = nextEventId;
	TI83->NextEventTime = nextEventTime;
}

u64 Event(TI83_t* TI83, u64 cycleCount) {
	switch (TI83->NextEventId) {
		case TIMER_IRQ:
		{
			TI83->TimerIntPending = true;
			TI83->TimerLastUpdate = TI83->EventSchedule[TIMER_IRQ];
			ScheduleEvent(TI83, TIMER_IRQ, TI83->TimerLastUpdate + TI83->TimerPeriod);
			if (!TI83->IFF || TI83->EventSchedule[INTERRUPT] != EVENT_TIME_NEVER) {
				break;
			}
		} // fallthrough
		case INTERRUPT:
		{
			switch (TI83->IM) {
				case 0:
				{
					cycleCount += 11;
					break; // not supported on TI83, assume a nop
				}
				case 1:
				{
					cycleCount += 7;
					WriteMem(TI83, --TI83->SP, TI83->PCH);
					cycleCount += 3;
					WriteMem(TI83, --TI83->SP, TI83->PCL);
					TI83->WZ = TI83->PC = 0x0038;
					cycleCount += 3;
					break;
				}
				case 2:
				{
					cycleCount += 7;
					WriteMem(TI83, --TI83->SP, TI83->PCH);
					cycleCount += 3;
					WriteMem(TI83, --TI83->SP, TI83->PCL);
					cycleCount += 3;
					u16 addr = TI83->I << 8;
					TI83->Z = TI83->PCL = ReadMem(TI83, addr);
					cycleCount += 3;
					TI83->W = TI83->PCH = ReadMem(TI83, addr + 1);
					cycleCount += 3;
					break;
				}
			}
			TI83->IFF = TI83->Halted = false;
			TI83->R = (TI83->R & 0x80) | ((TI83->R + 1) & 0x7F);
			ScheduleEvent(TI83, INTERRUPT, EVENT_TIME_NEVER);
			break;
		}
		case END_FRAME:
		{
			ScheduleEvent(TI83, END_FRAME, EVENT_TIME_NEVER);
			break;
		}
		case NUM_EVENTS: __builtin_unreachable();
	}

	return cycleCount;
}