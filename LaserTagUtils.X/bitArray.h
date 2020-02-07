#ifndef BITARRAY_H
#define BITARRAY_H

#include <stdbool.h>
#include <stdint.h>

// Set and get bits from a bitarray. Bytes are in little endian order, bits are in big endian order. E.g. the bit at
// index 0 is the most significant bit in byte zero.
// These functions do no bounds checking. The given index must be less than or equal to eight times the length of the
// given array.
bool bitArray_getBit(uint8_t* arr, uint8_t index);
void bitArray_setBit(uint8_t* arr, uint8_t index, bool b);

// The minimum number of bytes necessary to store the given number of bits
#define NUM_BYTES(num_bits) (((num_bits) + 7) >> 3)

#endif /* BITARRAY_H */
