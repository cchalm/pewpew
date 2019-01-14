#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t* storage;
    // The length of the buffer in bytes
    uint8_t length;
    // Index of the first byte in the buffer
    uint8_t front_index;
    // Index one past the last byte in the buffer. Special condition: back_index is set to `length` when the buffer is
    // full
    uint8_t back_index;
} circular_buffer_t;

// Create a circular buffer using the given storage. In this context memory cannot be allocated dynamically, so the
// given storage is statically-allocated and deallocation is not a concern
circular_buffer_t circularBuffer_create(uint8_t* storage, uint8_t length);

// Push a byte onto the front/back of the circular buffer. Returns false if the buffer is full, true otherwise. Not
// thread safe
bool circularBuffer_pushFront(circular_buffer_t* buffer, uint8_t data);
bool circularBuffer_pushBack(circular_buffer_t* buffer, uint8_t data);
// Pop a byte from the front/back of the circular buffer. Returns true and sets the out parameter to the popped byte if
// there is at least one byte in the buffer, false otherwise. Not thread safe
bool circularBuffer_popFront(circular_buffer_t* buffer, uint8_t* data_out);
bool circularBuffer_popBack(circular_buffer_t* buffer, uint8_t* data_out);

// Get the byte at the given index of the buffer. The index is relative to the front of the buffer. The given index must
// be less than the current size of the buffer
uint8_t circularBuffer_get(circular_buffer_t* buffer, uint8_t index);
// Set the byte at the given index of the buffer. The index is relative to the front of the buffer. The given index must
// be less than the current size of the buffer
void circularBuffer_set(circular_buffer_t* buffer, uint8_t index, uint8_t value);

// Returns the address of the given index of the buffer. The index is relative to the front of the buffer. The given
// index must be less than the current size of the buffer
uint8_t* circularBuffer_at(circular_buffer_t* buffer, uint8_t index);

// Capacity: the number of bytes that can fit in the buffer
uint8_t circularBuffer_capacity(circular_buffer_t* buffer);
// Size: the number of bytes that are in the buffer
uint8_t circularBuffer_size(circular_buffer_t* buffer);
// Free Capacity: capacity minus size
uint8_t circularBuffer_freeCapacity(circular_buffer_t* buffer);

#endif /* CIRCULARBUFFER_H */
