#include "stringQueue.h"

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
    uint8_t queue_length = length - bitarray_length;
    string_queue_t string_queue = {
        .byte_queue = queue_create(storage, queue_length),
        .string_end_flags = bitQueue_create(storage + queue_length, bitarray_length)
    };

    return string_queue;
}

bool stringQueue_push(string_queue_t* string_queue, uint8_t* string, uint8_t string_length)
{    
    return stringQueue_pushPartial(string_queue, string, string_length, true);
}

bool stringQueue_pushPartial(string_queue_t* string_queue, uint8_t* partial_string, uint8_t partial_string_length, bool is_end_of_string)
{
    // Check if the bytes of the string will fit in the byte queue. No need to
    // check the bit queue, it fits at least as many bits as the byte queue fits
    // bytes
    if (queue_freeCapacity(&string_queue->byte_queue) < partial_string_length)
        return false;

    for (uint8_t i = 0; i < partial_string_length; i++)
    {
        queue_push(&string_queue->byte_queue, partial_string[i]);
        bool is_final_byte = is_end_of_string && (i == (partial_string_length - 1));
        bitQueue_push(&string_queue->string_end_flags, is_final_byte);
    }

    return true;
}

bool stringQueue_pop(string_queue_t* string_queue, uint8_t max_data_length, uint8_t* data_out, uint8_t* data_length_out)
{
    bool found_last_byte = false;

    uint8_t i = 0;
    // No need to check for an empty byte queue - the last byte will be flagged
    // as the end of a string
    while (i < max_data_length && !found_last_byte)
    {
        bitQueue_pop(&string_queue->string_end_flags, &found_last_byte);
        queue_pop(&string_queue->byte_queue, data_out + i);

        i++;
    }

    *data_length_out = i;

    return found_last_byte;
}

uint8_t stringQueue_capacity(string_queue_t* string_queue)
{
    return queue_capacity(&string_queue->byte_queue);
}

uint8_t stringQueue_size(string_queue_t* string_queue)
{
    return queue_size(&string_queue->byte_queue);
}

uint8_t stringQueue_freeCapacity(string_queue_t* string_queue)
{
    return queue_freeCapacity(&string_queue->byte_queue);
}
