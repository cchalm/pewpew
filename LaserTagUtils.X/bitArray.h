#ifndef BITARRAY_H
#define BITARRAY_H

#include <stdbool.h>
#include <stdint.h>

// These functions do no bounds checking. The given index must be less than or
// equal to eight times the length of the given array
bool bitArray_getBit(uint8_t* arr, uint8_t index);
void bitArray_setBit(uint8_t* arr, uint8_t index, bool b);

#endif /* BITARRAY_H */
