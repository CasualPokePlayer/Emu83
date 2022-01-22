#include "ti83.h"
#include "assert.h"

void Queue_Enqueue(Queue_t* queue, u8 val) {
	assert(queue->Length);
	assert(queue->Length >= queue->Index);
	if (queue->Length == queue->Index) {
		queue->Length <<= 1;
		queue->Data = realloc(queue->Data, queue->Length);
		assert(queue->Data);
	}
	queue->Data[queue->Index++] = val;
}

u8 Queue_Dequeue(Queue_t* queue) {
	assert(queue->Index);
	assert(queue->Length >= queue->Index);
	u8 ret = queue->Data[0];
	memmove(queue->Data, queue->Data + 1, --queue->Index);
	return ret;
}

void Queue_Clear(Queue_t* queue) {
	queue->Index = 0;
}