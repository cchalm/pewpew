#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t* storage;
    // The length of the queue in bytes
    uint8_t length;
    // Index of the first byte in the queue
    uint8_t index;
    // Index one past the last byte in the queue. Special condition: end_index is set to `length` when the queue is full
    uint8_t end_index;
} queue_t;

// Create a queue using the given storage. In this context memory cannot be
// allocated dynamically, so the given storage is statically-allocated and
// deallocation is not a concern
queue_t queue_create(uint8_t* storage, uint8_t length);

// Push a byte onto the back of the queue. Returns false if the queue is full,
// true otherwise. Not thread safe
bool queue_push(queue_t* queue, uint8_t data);
// Pop a byte from the front of the queue. Returns false if the queue is empty,
// true otherwise. Returns the byte through the out parameter. Not thread safe
bool queue_pop(queue_t* queue, uint8_t* data_out);

uint8_t queue_capacity(queue_t* queue);
uint8_t queue_size(queue_t* queue);
uint8_t queue_freeCapacity(queue_t* queue);

#endif /* QUEUE_H */
