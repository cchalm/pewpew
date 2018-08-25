#ifndef ERROR_H
#define	ERROR_H

#include <stdint.h>

enum
{
    ERROR_INVALID_SHOT_DATA_RECEIVED        = 0b1000000001,
    ERROR_TRANSMISSION_OVERLAP              = 0b1000000010,
    ERROR_SMT1_PERIOD_TOO_LARGE             = 0b1000000100,
    ERROR_OVERLAPPING_PULSE_LENGTH_RANGES   = 0b1000001000
};

void fatal(uint16_t error_code);

#endif	/* ERROR_H */

