#ifndef KEYEDSTRINGQUEUE_H
#define KEYEDSTRINGQUEUE_H

#include "stringQueue.h"

#include <stdbool.h>
#include <stdint.h>

// Same as stringQueue_pop, except it treats the first byte of each string in the queue as a key, and pops the first
// one with the given key. O(n) complexity, n being the size of the stringQueue.
bool keyedStringQueue_pop(string_queue_t* queue, uint8_t key, uint8_t max_length, uint8_t* data_out,
                          uint8_t* length_out);

#endif /* KEYEDSTRINGQUEUE_H */
