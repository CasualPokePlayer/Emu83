#ifndef EVENTS_H
#define EVENTS_H

#include "ti83.h"

void ScheduleEvent(TI83_t* TI83, EventId_t eventId, u64 newEventTime);
u64 Event(TI83_t* TI83, u64 cycleCount);

#endif
