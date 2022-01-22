#ifndef QUEUE_H
#define QUEUE_H

void Queue_Enqueue(Queue_t* queue, u8 val);
u8 Queue_Dequeue(Queue_t* queue);
void Queue_Clear(Queue_t* queue);

#endif
