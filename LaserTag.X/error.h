#ifndef ERROR_H
#define	ERROR_H

#include <stdint.h>

enum
{
    ERROR_INVALID_SHOT_DATA_RECEIVED        = 0b1010101010,
    ERROR_I2C_BUFFER_CONTENTION             = 0b1101101100,
    ERROR_I2C_NO_ACK                        = 0b1110111000,
    ERROR_LED_DRIVER_INVALID_REG_RANGE      = 0b1111011110,
    ERROR_LED_DRIVER_TRANSMISSION_CONFLICT  = 0b1111011110,
};

void fatal(uint16_t error_code);

#endif	/* ERROR_H */

