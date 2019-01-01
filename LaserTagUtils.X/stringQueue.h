#ifndef STRINGQUEUE_H
#define	STRINGQUEUE_H

#include "bitQueue.h"
#include "queue.h"

#include <stdbool.h>
#include <stdint.h>

// A StringQueue is a queue of variable-length strings. In this context "string"
// means a sequence of bytes. No assumptions are made about the nature of the
// bytes, i.e. they may or may not be interpreted as characters by the user.

typedef struct {
    queue_t byte_queue;
    bit_queue_t string_end_flags;
} string_queue_t;

// Create a queue using the given storage. In this context memory cannot be
// allocated dynamically, so the given storage is statically-allocated and
// deallocation is not a concern. The maximum number of bytes that can be stored
// in the string queue is approximately 90% of length.
string_queue_t stringQueue_create(uint8_t* storage, uint8_t length);

// Push a string onto the back of the queue. Returns false if the queue does not
// have enough space for the string, true otherwise. Not thread safe
bool stringQueue_push(string_queue_t* queue, uint8_t* string, uint8_t string_length);
// Push part of a string. The final parameter indicates whether this push
// concludes the string
bool stringQueue_pushPartial(string_queue_t* queue, uint8_t* partial_string, uint8_t partial_string_length, bool is_end_of_string);
// Pop a string from the front of the queue. Takes the maximum number of bytes
// to pop. Returns true if the last byte popped is the last byte of a string,
// false otherwise. Returns the data and the length of the returned data as two
// out parameters. A return value of false with a returned data length of zero
// indicates that the queue is empty.
bool stringQueue_pop(string_queue_t* queue, uint8_t max_data_length, uint8_t* data_out, uint8_t* data_length_out);

uint8_t stringQueue_capacity(string_queue_t* queue);
uint8_t stringQueue_size(string_queue_t* queue);
uint8_t stringQueue_freeCapacity(string_queue_t* queue);

#endif	/* STRINGQUEUE_H */

