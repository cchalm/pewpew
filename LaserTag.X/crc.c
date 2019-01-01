#include "crc.h"

#include "crcConstants.h"

#include <stdbool.h>
#include <stdint.h>

static uint8_t calculateCRC(uint8_t data)
{
    uint8_t remainder = data;

    for (int i = 0; i < 8; i++)
    {
        // As per polynomial division, match the divisor up with the highest
        // order term of the dividend. In code language: only do the XOR when
        // we've shifted the dividend over such that the MSB is a 1.
        if ((remainder & 0b10000000) != 0)
        {
            // Shift out the "1" MSB and perform the XOR with the remainder of
            // the dividend, which works because the polynomial has an implicit
            // "1" as its MSB, so an implicit XOR of the "1" we shifted out of
            // the dividend and the implicit "1" in the divisor creates an
            // implicit "0" as the top bit of the remainder, which is
            // irrelevant.
            remainder = (remainder << 1) ^ POLYNOMIAL_FOR_ALGORITHM;
        }
        else
        {
            remainder <<= 1;
        }
    }

    return remainder >> (8 - CRC_LENGTH);
}

static uint8_t CRC_LOOKUP[256];

static void populateCRCLookup()
{
    for (int i = 0; i < 256; i++)
    {
        CRC_LOOKUP[i] = calculateCRC(i);
    }
}

void initializeCRC(void)
{
    populateCRCLookup();
}
uint8_t crc(uint8_t data)
{
    return CRC_LOOKUP[data];
}
