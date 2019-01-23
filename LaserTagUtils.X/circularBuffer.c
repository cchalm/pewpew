#include "circularBuffer.h"

#include <stdbool.h>
#include <stdint.h>

circular_buffer_t circularBuffer_create(uint8_t* storage, uint8_t length)
{
    circular_buffer_t circularBuffer = {
        .storage = storage, .length = length, .front_index = 0, .back_index = 0,
    };

    return circularBuffer;
}

static uint8_t increment(uint8_t index, uint8_t end)
{
    // Increment our local copy
    index++;

    // Wrap around to zero if we're at the end
    if (index == end)
        return 0;

    return index;
}

static uint8_t decrement(uint8_t index, uint8_t end)
{
    if (index == 0)
        return end - 1;
    else
        return index - 1;
}

static bool isEmpty(circular_buffer_t* buffer)
{
    return buffer->back_index == buffer->front_index;
}

static bool isFull(circular_buffer_t* buffer)
{
    return buffer->back_index == decrement(buffer->front_index, buffer->length);
}

bool circularBuffer_pushFront(circular_buffer_t* buffer, uint8_t data)
{
    if (isFull(buffer))
        return false;

    buffer->front_index = decrement(buffer->front_index, buffer->length);
    buffer->storage[buffer->front_index] = data;

    return true;
}

bool circularBuffer_pushBack(circular_buffer_t* buffer, uint8_t data)
{
    if (isFull(buffer))
        return false;

    buffer->storage[buffer->back_index] = data;
    buffer->back_index = increment(buffer->back_index, buffer->length);

    return true;
}

bool circularBuffer_popFront(circular_buffer_t* buffer, uint8_t* data_out)
{
    if (isEmpty(buffer))
        return false;

    *data_out = buffer->storage[buffer->front_index];
    buffer->front_index = increment(buffer->front_index, buffer->length);

    return true;
}

bool circularBuffer_popBack(circular_buffer_t* buffer, uint8_t* data_out)
{
    if (isEmpty(buffer))
        return false;

    buffer->back_index = decrement(buffer->back_index, buffer->length);
    *data_out = buffer->storage[buffer->back_index];

    return true;
}

uint8_t circularBuffer_get(circular_buffer_t* buffer, uint8_t index)
{
    return *circularBuffer_at(buffer, index);
}

void circularBuffer_set(circular_buffer_t* buffer, uint8_t index, uint8_t value)
{
    *circularBuffer_at(buffer, index) = value;
}

uint8_t* circularBuffer_at(circular_buffer_t* buffer, uint8_t index)
{
    if (index >= circularBuffer_capacity(buffer))
        return 0;

    return buffer->storage + _circularBuffer_getPhysicalIndex(buffer, index);
}

uint8_t circularBuffer_capacity(circular_buffer_t* buffer)
{
    return buffer->length - 1;
}
uint8_t circularBuffer_size(circular_buffer_t* buffer)
{
    // The relative index of the physical back index is the size of the buffer
    return _circularBuffer_getRelativeIndex(buffer, buffer->back_index);
}

uint8_t circularBuffer_freeCapacity(circular_buffer_t* buffer)
{
    return circularBuffer_capacity(buffer) - circularBuffer_size(buffer);
}

uint8_t _circularBuffer_getPhysicalIndex(circular_buffer_t* buffer, uint8_t index)
{
    if (buffer->length - buffer->front_index > index)
        return buffer->front_index + index;
    else
        return index - (buffer->length - buffer->front_index);
}

uint8_t _circularBuffer_getRelativeIndex(circular_buffer_t* buffer, uint8_t physical_index)
{
    if (physical_index >= buffer->front_index)
        return physical_index - buffer->front_index;
    else
        return (buffer->length - buffer->front_index) + physical_index;
}
