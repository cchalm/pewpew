#ifndef STRINGQUEUE_H
#define STRINGQUEUE_H

#include "circularBuffer.h"

#include <stdbool.h>
#include <stdint.h>

// A StringQueue is a queue of variable-length strings. In this context "string"
// means a sequence of bytes. No assumptions are made about the nature of the
// bytes, i.e. they may or may not be interpreted as characters by the user.

typedef struct
{
    circular_buffer_t buffer;
    uint8_t* string_end_flags;
} string_queue_t;

// Create a queue using the given storage. In this context memory cannot be allocated dynamically, so the given storage
// is statically-allocated and deallocation is not a concern. The maximum number of bytes that can be stored in the
// string queue is approximately 90% of length.
string_queue_t stringQueue_create(uint8_t* storage, uint8_t length);

// Push a string onto the back of the queue. Returns false if the queue does not have enough space for the string, true
// otherwise. Not thread safe
bool stringQueue_push(string_queue_t* queue, uint8_t* string, uint8_t string_length);
// Push part of a string. The final parameter indicates whether this push concludes the string. If the final parameter
// is `true` and the given partial string length is `0`, marks the most recently added byte as the end of the string
bool stringQueue_pushPartial(string_queue_t* queue, uint8_t* partial_string, uint8_t partial_string_length,
                             bool is_end_of_string);
// Pop a string from the front of the queue. Takes the maximum number of bytes to pop. Returns true if the last byte
// popped is the last byte of a string, false otherwise. Returns the data and the length of the returned data as two out
// parameters. A return value of false with a returned data length of zero indicates that the queue is empty. Partial
// strings can be popped.
bool stringQueue_pop(string_queue_t* queue, uint8_t max_length, uint8_t* data_out, uint8_t* length_out);

// The total number of bytes the string queue can hold
uint8_t stringQueue_capacity(string_queue_t* queue);
// The number of bytes currently in the string queue. Includes bytes of a partially-added string, if any
uint8_t stringQueue_size(string_queue_t* queue);
// The difference between capacity and size
uint8_t stringQueue_freeCapacity(string_queue_t* queue);

// Returns true if the queue contains at least one full string. Returns false if the queue is empty or only contains a
// partial string
bool stringQueue_hasFullString(string_queue_t* queue);
// Returns true if the queue contains a full string at or after the given index, false otherwise
bool stringQueue_hasFullStringAt(string_queue_t* queue, uint8_t index);

// Peek at the length of the string at the front of the queue. Linear complexity, with the length of the string
uint8_t stringQueue_peekStringLength(string_queue_t* queue);

// Only to be used for extension
// Indexes to these functions are exclusive end indexes, i.e. they point to one byte past the end of a string
bool _stringQueue_getIsEndOfString(string_queue_t* queue, uint8_t index);
void _stringQueue_setIsEndOfString(string_queue_t* queue, uint8_t index, bool is_end_of_string);
// Pop from anywhere in the queue. O(n+m), where n is the distance from the queue's current index to the given index,
// and m is max_data_length. If popping from the front of the queue, O(m).
bool _stringQueue_popMiddle(string_queue_t* queue, uint8_t index, uint8_t max_length, uint8_t* data_out,
                            uint8_t* length_out);

#endif /* STRINGQUEUE_H */
