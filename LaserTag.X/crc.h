#ifndef CRC_H
#define CRC_H

#include <stdbool.h>
#include <stdint.h>

void initializeCRC(void);

// Calculates a CRC for 8 bits of data. The length of the CRC is determined by
// the CRC_LENGTH constant
uint8_t crc(uint8_t data);

#endif /* CRC_H */
