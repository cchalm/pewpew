#include "bitArray.h"

bool bitArray_getBit(uint8_t* arr, uint8_t index)
{
    return (arr[index / 8] & (1 << (index % 8))) != 0;
}

void bitArray_setBit(uint8_t* arr, uint8_t index, bool b)
{
    if (b)
        arr[index / 8] |= (1 << (index % 8));
    else
        arr[index / 8] &= ~(1 << (index % 8));
}
