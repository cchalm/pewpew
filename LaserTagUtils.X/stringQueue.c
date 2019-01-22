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
        // Clear the end-of-string flag, in case it was set before
        _stringQueue_setIsEndOfString(queue, circularBuffer_size(&queue->buffer), false);
    }

    if (is_end_of_string)
    {
        // Indicate that the last byte in the queue is the end of a string
        _stringQueue_setIsEndOfString(queue, circularBuffer_size(&queue->buffer), true);
    }

    return true;
}

bool stringQueue_pop(string_queue_t* queue, uint8_t max_length, uint8_t* data_out, uint8_t* length_out)
{
    bool found_last_byte = false;

    uint8_t queue_size = stringQueue_size(queue);
    if (max_length > queue_size)
        max_length = queue_size;

    uint8_t i = 0;
    // No need to check for an empty byte queue - the last byte will be flagged
    // as the end of a string
    while (i < max_length && !found_last_byte)
    {
        circularBuffer_popFront(&queue->buffer, data_out + i);
        // We just popped a byte from the front of the queue, so index 0 points one past that byte
        found_last_byte = _stringQueue_getIsEndOfString(queue, 0);

        i++;
    }

    if (found_last_byte)
    {
        _stringQueue_setIsEndOfString(queue, 0, false);
    }

    *length_out = i;

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

    if (stringQueue_size(queue) == 0)
        return false;

    // Length of the "end of string" flag array in bytes
    uint8_t bitarray_length = (queue->buffer.length + 7) >> 3;

    // Inclusive start index into bytes of the bitarray
    uint8_t start_byte_index = increment(queue->buffer.front_index, queue->buffer.length) >> 3;

    // Exclusive end index into bytes of the bitarray
    uint8_t end_byte_index = increment(queue->buffer.back_index >> 3, bitarray_length);

    // TODO mask first and last byte to avoid picking up errant bits

    uint8_t byte_index = start_byte_index;

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
    uint8_t index = 0;

    while (!_stringQueue_getIsEndOfString(queue, index))
        index++;

    return index;
}

bool _stringQueue_getIsEndOfString(string_queue_t* queue, uint8_t index)
{
    return bitArray_getBit(queue->string_end_flags, _circularBuffer_getPhysicalIndex(&queue->buffer, index));
}

void _stringQueue_setIsEndOfString(string_queue_t* queue, uint8_t index, bool is_end_of_string)
{
    bitArray_setBit(queue->string_end_flags, _circularBuffer_getPhysicalIndex(&queue->buffer, index), is_end_of_string);
}

bool _stringQueue_popMiddle(string_queue_t* queue, uint8_t index, uint8_t max_length, uint8_t* data_out,
                            uint8_t* length_out)
{
    bool found_last_byte = false;

    uint8_t i = 0;

    while (i < max_length && !found_last_byte)
    {
        data_out[i] = circularBuffer_get(&queue->buffer, index + i);
        // Increment before checking the string end flag, because the string end flags line up with exclusive end
        // indexes
        i++;

        found_last_byte = _stringQueue_getIsEndOfString(queue, index + i);
    }

    if (found_last_byte)
    {
        _stringQueue_setIsEndOfString(queue, index + i, false);
    }

    uint8_t popped_length = i;
    *length_out = popped_length;

    // Copy bytes from before the popped section forward to fill the gap
    for (uint8_t j = 0; j < index; j++)
    {
        uint8_t copy_from_index = index - j - 1;
        uint8_t copy_to_index = copy_from_index + popped_length;

        uint8_t value = circularBuffer_get(&queue->buffer, copy_from_index);
        circularBuffer_set(&queue->buffer, copy_to_index, value);

        bool is_end_of_string = _stringQueue_getIsEndOfString(queue, copy_from_index + 1);
        _stringQueue_setIsEndOfString(queue, copy_to_index + 1, is_end_of_string);
    }

    // Trim the bytes from the front of the queue that we just shifted forward, and clear their string end flags
    for (uint8_t j = 0; j < popped_length; j++)
    {
        uint8_t dummy;
        circularBuffer_popFront(&queue->buffer, &dummy);

        // Clear the end-of-string flag at the index after the one we just popped. The index we just popped is now at
        // index -1, because we popped it from the front, so the index after it is 0
        _stringQueue_setIsEndOfString(queue, 0, false);
    }

    return found_last_byte;
}
