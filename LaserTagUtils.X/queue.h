#ifndef QUEUE_H
#define QUEUE_H

#include "circularBuffer.h"

#include <stdbool.h>
#include <stdint.h>

// A queue is just a circular buffer with a subset of the functionality exposed. See circularBuffer.h for documentation
typedef circular_buffer_t queue_t;

queue_t queue_create(uint8_t* storage, uint8_t length);

// See circularBuffer_pushBack
bool queue_push(queue_t* queue, uint8_t data);
// See circularBuffer_popFront
bool queue_pop(queue_t* queue, uint8_t* data_out);

uint8_t queue_capacity(queue_t* queue);
uint8_t queue_size(queue_t* queue);
uint8_t queue_freeCapacity(queue_t* queue);

#endif /* QUEUE_H */
