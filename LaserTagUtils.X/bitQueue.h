#ifndef BITQUEUE_H
#define BITQUEUE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t* storage;
    uint8_t length;
    uint8_t index;
    uint8_t end_index;
} bit_queue_t;

// Create a bit queue using the given storage. The number of bits that can be
// stored in the bitQueue is be length * 8. In this context memory cannot be
// allocated dynamically, so the given storage is statically-allocated and
// deallocation is not a concern
bit_queue_t bitQueue_create(uint8_t* storage, uint8_t length);

// Push a byte onto the back of the queue. Returns false if the queue is full,
// true otherwise. Not thread safe
bool bitQueue_push(bit_queue_t* queue, bool data);
// Pop a byte from the front of the queue. Returns false if the queue is empty,
// true otherwise. Returns the byte through the out parameter. Not thread safe
bool bitQueue_pop(bit_queue_t* queue, bool* data_out);

uint8_t bitQueue_capacity(bit_queue_t* queue);
uint8_t bitQueue_size(bit_queue_t* queue);
uint8_t bitQueue_freeCapacity(bit_queue_t* queue);

#endif /* BITQUEUE_H */
