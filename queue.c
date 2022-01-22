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