#include "bitArray.h"

bool bitArray_getBit(uint8_t* arr, uint8_t index)
{
    // `>> 3` is equivalent to `/ 8`, but ~40x faster. xc8 doesn't optimize power-of-two division.
    // `& 0b111` is equivalent to `% 8`, but ~100x faster. xc8 *does* optimize power-of-two modulo, but writing out
    // the optimized form just in case.
    return (arr[index >> 3] & (1 << (index & 0b111))) != 0;
}

void bitArray_setBit(uint8_t* arr, uint8_t index, bool b)
{
    if (b)
        arr[index >> 3] |= (1 << (index & 0b111));
    else
        arr[index >> 3] &= ~(1 << (index & 0b111));
}
