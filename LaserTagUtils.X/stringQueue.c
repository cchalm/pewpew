#include "stringQueue.h"

#include "bitArray.h"
#include "circularBuffer.h"

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
    string_queue_t string_queue = {.buffer = circularBuffer_create(storage, byte_queue_length),
                                   .string_end_flags = storage + byte_queue_length};

    return string_queue;
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
        circularBuffer_pushBack(&queue->buffer, partial_string[i]);
    }

    if (is_end_of_string)
    {
        // Set the "end of string" flag
        bitArray_setBit(queue->string_end_flags, queue->buffer.back_index, true);
    }

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
        // Pop the byte before checking the string end flag, because the string end flags line up with exclusive end
        // indexes, and we're going to borrow the buffer's front index
        circularBuffer_popFront(&queue->buffer, data_out + i);
        found_last_byte = bitArray_getBit(queue->string_end_flags, queue->buffer.front_index);

        i++;
    }

    if (found_last_byte)
    {
        // Clear the "end of string" flag
        bitArray_setBit(queue->string_end_flags, queue->buffer.front_index, false);
    }

    *data_length_out = i;

    return found_last_byte;
}

uint8_t stringQueue_capacity(string_queue_t* queue)
{
    return circularBuffer_capacity(&queue->buffer);
}

uint8_t stringQueue_size(string_queue_t* queue)
{
    return circularBuffer_size(&queue->buffer);
}

uint8_t stringQueue_freeCapacity(string_queue_t* queue)
{
    return circularBuffer_freeCapacity(&queue->buffer);
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

bool stringQueue_hasFullString(string_queue_t* queue)
{
    // If there are any set bits in the "end of string" flag array bytes covering the range between `index` and
    // `end_index`, then there is a full string in the queue

    // Length of the "end of string" flag array in bytes
    uint8_t bitarray_length = (queue->buffer.length + 7) / 8;

    // Inclusive start index into bytes of the bitarray
    uint8_t byte_index = increment(queue->buffer.front_index, queue->buffer.length) / 8;
    // Exclusive end index into bytes of the bitarray
    uint8_t end_byte_index = increment(queue->buffer.back_index / 8, bitarray_length);

    while (byte_index != end_byte_index)
    {
        if (*(queue->string_end_flags + byte_index) != 0)
            return true;

        byte_index = increment(byte_index, bitarray_length);
    }

    return false;
}

uint8_t stringQueue_peekStringLength(string_queue_t* queue)
{
    uint8_t string_length = 0;
    // Start one ahead, since string end flags are at exclusive indexes
    uint8_t cursor = increment(queue->buffer.front_index, queue->buffer.length);

    while (!bitArray_getBit(queue->string_end_flags, cursor))
    {
        cursor = increment(cursor, queue->buffer.length);
        string_length++;
    }

    return string_length;
}
