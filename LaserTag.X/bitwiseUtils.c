#include "bitwiseUtils.h"

uint8_t sumBits(uint16_t bits)
{
    uint8_t sum = 0;
    while (bits != 0)
    {
        sum++;
        // Remove least significant ON bit
        bits = ((bits - 1) & bits);
    }
    return sum;
}
