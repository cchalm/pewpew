#include "bitQueue.h"

#include "bitArray.h"

#include <stdbool.h>
#include <stdint.h>

bit_queue_t bitQueue_create(uint8_t* storage, uint8_t length)
{
    bit_queue_t queue = {
        .storage = storage,
        .length = length * 8,
        .index = 0,
        // Special condition: end_index is set to length when the queue is full
        .end_index = 0,
    };

    return queue;
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

static void incrementIndex(bit_queue_t* queue)
{
    queue->index = increment(queue->index, queue->length);
}

static void incrementEndIndex(bit_queue_t* queue)
{
    queue->end_index = increment(queue->end_index, queue->length);
}

static bool isEmpty(bit_queue_t* queue)
{
    return queue->end_index == queue->index;
}

static bool isFull(bit_queue_t* queue)
{
    return queue->end_index == queue->length;
}

// When setting from "full" to "not full", this must be called *before*
// incrementing queue->index
static void setIsFull(bit_queue_t* queue, bool set_is_full)
{
    if (set_is_full)
        queue->end_index = queue->length;
    else if (isFull(queue))
        queue->end_index = queue->index;
}

bool bitQueue_push(bit_queue_t* queue, bool data)
{
    if (isFull(queue))
        return false;

    bitArray_setBit(queue->storage, queue->end_index, data);
    incrementEndIndex(queue);

    if (queue->index == queue->end_index)
        setIsFull(queue, true);

    return true;
}

bool bitQueue_pop(bit_queue_t* queue, bool* data_out)
{
    if (isEmpty(queue))
        return false;

    *data_out = bitArray_getBit(queue->storage, queue->index);
    setIsFull(queue, false);
    incrementIndex(queue);

    return true;
}

uint8_t bitQueue_capacity(bit_queue_t* queue)
{
    return queue->length;
}
uint8_t bitQueue_size(bit_queue_t* queue)
{
    if (isFull(queue))
        return 0;

    return queue->end_index - queue->index;
}

uint8_t bitQueue_freeCapacity(bit_queue_t* queue)
{
    return bitQueue_capacity(queue) - bitQueue_size(queue);
}
