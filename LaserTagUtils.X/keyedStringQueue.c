#include "keyedStringQueue.h"

#include "circularBuffer.h"
#include "stringQueue.h"

bool keyedStringQueue_pop(string_queue_t* queue, uint8_t key, uint8_t max_length, uint8_t* data_out,
                          uint8_t* length_out)
{
    // Find the key
    uint8_t key_index = 0;
    uint8_t queue_size = stringQueue_size(queue);
    while (circularBuffer_get(&queue->buffer, key_index) != key)
    {
        do
        {
            key_index++;
        } while (!_stringQueue_getIsEndOfString(queue, key_index));

        if (key_index == queue_size)
        {
            // Key not found
            return false;
        }
    }

    // Pop from the index after the key. We'll pop off the key only if we pop off the end of string, otherwise we'll
    // leave it there for the next partial pop to find
    uint8_t pop_from = key_index + 1;

    bool popped_end_of_string = _stringQueue_popMiddle(queue, pop_from, max_length, data_out, length_out);

    if (popped_end_of_string)
    {
        // Pop off the key
        uint8_t dummyData;
        uint8_t dummyLength;
        _stringQueue_popMiddle(queue, key_index, 1, &dummyData, &dummyLength);
    }

    return popped_end_of_string;
}
