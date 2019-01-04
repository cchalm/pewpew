#include "stringQueue.h"

#include "bitArray.h"

#include <stdbool.h>
#include <stdint.h>

string_queue_t stringQueue_create(uint8_t* storage, uint8_t length)
{
    // For every eight data bytes, we need one byte in the bitarray so that
    // there is one bit in the array for each data byte. We must therefore
    // divide the total storage by nine and allocate eight parts to the data,
    // and one part to the bitarray. The bitarray must be at *least* one part
    // per nine, so the formula for the bitarray length is ceil(length/9), which
    // we can use integer division to calculate
    uint8_t bitarray_length = (length + 8) / 9;
    uint8_t byte_queue_length = length - bitarray_length;
    string_queue_t string_queue = {.storage = storage, .length = byte_queue_length, .index = 0, .end_index = 0};

    return string_queue;
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
    // Wrap around to `end` if we're at zero
    if (index == 0)
        return end - 1;
    else
        return index - 1;
}

static void incrementIndex(string_queue_t* queue)
{
    queue->index = increment(queue->index, queue->length);
}

static void incrementEndIndex(string_queue_t* queue)
{
    queue->end_index = increment(queue->end_index, queue->length);
}

static bool isEmpty(string_queue_t* queue)
{
    return queue->end_index == queue->index;
}

static bool isFull(string_queue_t* queue)
{
    return queue->end_index == queue->length;
}

// When setting from "full" to "not full", this must be called *before*
// incrementing queue->index
static void setIsFull(string_queue_t* queue, bool set_is_full)
{
    if (set_is_full)
        queue->end_index = queue->length;
    else if (isFull(queue))
        queue->end_index = queue->index;
}

static uint8_t* getBitArray(string_queue_t* queue)
{
    return queue->storage + queue->length;
}

bool stringQueue_push(string_queue_t* queue, uint8_t* string, uint8_t string_length)
{
    return stringQueue_pushPartial(queue, string, string_length, true);
}

bool stringQueue_pushPartial(string_queue_t* queue, uint8_t* partial_string, uint8_t partial_string_length,
                             bool is_end_of_string)
{
    // Check if the bytes of the string will fit in the byte queue
    if (stringQueue_freeCapacity(queue) < partial_string_length)
        return false;

    for (uint8_t i = 0; i < partial_string_length; i++)
    {
        queue->storage[queue->end_index] = partial_string[i];

        // Increment end index before setting the string end flag, because the string end flags line up with exclusive
        // end indexes
        incrementEndIndex(queue);

        bool is_final_byte = is_end_of_string && (i == (partial_string_length - 1));
        bitArray_setBit(getBitArray(queue), queue->end_index, is_final_byte);
    }

    if (queue->end_index == queue->index)
        setIsFull(queue, true);

    return true;
}

bool stringQueue_pop(string_queue_t* queue, uint8_t max_data_length, uint8_t* data_out, uint8_t* data_length_out)
{
    bool found_last_byte = false;

    uint8_t i = 0;
    // No need to check for an empty byte queue - the last byte will be flagged
    // as the end of a string
    while (i < max_data_length && !found_last_byte)
    {
        *data_out = queue->storage[queue->index];
        setIsFull(queue, false);

        // Increment index before checking the string end flag, because the string end flags line up with exclusive end
        // indexes
        incrementIndex(queue);

        found_last_byte = bitArray_getBit(getBitArray(queue), queue->index);

        i++;
    }

    *data_length_out = i;

    return found_last_byte;
}

uint8_t stringQueue_capacity(string_queue_t* queue)
{
    return queue->length;
}

uint8_t stringQueue_size(string_queue_t* queue)
{
    if (isFull(queue))
        return queue->length;

    return queue->end_index - queue->index;
}

uint8_t stringQueue_freeCapacity(string_queue_t* queue)
{
    return stringQueue_capacity(queue) - stringQueue_size(queue);
}

bool stringQueue_hasFullString(string_queue_t* queue)
{
    return stringQueue_size(queue) != 0 && bitArray_getBit(getBitArray(queue), queue->end_index);
}
