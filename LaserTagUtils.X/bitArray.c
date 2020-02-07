#include "bitArray.h"

#include <stdbool.h>
#include <stdint.h>

static inline uint8_t getByteIndex(uint8_t index)
{
    // Bytes are in little endian order, e.g. index 0 is byte 0, index 8 is byte 1, etc
    // `>> 3` is equivalent to `/ 8`, but ~40x faster. xc8 doesn't optimize power-of-two division.
    return index >> 3;
}

static inline uint8_t getBitIndex(uint8_t index)
{
    // Bits are in big endian order, e.g. index 0 is bit 7, index 1 is bit 6, etc
    // `& 0b111` is equivalent to `% 8`, but ~100x faster. xc8 *does* optimize power-of-two modulo, but writing out
    // the optimized form just in case.
    return 7 - (index & 0b111);
}

bool bitArray_getBit(uint8_t* arr, uint8_t index)
{
    return (arr[getByteIndex(index)] & (1 << getBitIndex(index))) != 0;
}

void bitArray_setBit(uint8_t* arr, uint8_t index, bool b)
{
    if (b)
        arr[getByteIndex(index)] |= (1 << getBitIndex(index));
    else
        arr[getByteIndex(index)] &= ~(1 << getBitIndex(index));
}
