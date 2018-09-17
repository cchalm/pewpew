#ifndef ERROR_H
#define	ERROR_H

#include <stdint.h>

enum
{
    ERROR_INVALID_SHOT_DATA_RECEIVED        = 0b1000000001
};

void fatal(uint16_t error_code);

#endif	/* ERROR_H */

