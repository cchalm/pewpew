#include "queue.h"

#include "circularBuffer.h"

queue_t queue_create(uint8_t* storage, uint8_t length)
{
    return circularBuffer_create(storage, length);
}

bool queue_push(queue_t* queue, uint8_t data)
{
    return circularBuffer_pushBack(queue, data);
}

bool queue_pop(queue_t* queue, uint8_t* data_out)
{
    return circularBuffer_popBack(queue, data_out);
}

uint8_t queue_capacity(queue_t* queue)
{
    return circularBuffer_capacity(queue);
}
uint8_t queue_size(queue_t* queue)
{
    return circularBuffer_size(queue);
}

uint8_t queue_freeCapacity(queue_t* queue)
{
    return circularBuffer_freeCapacity(queue);
}
