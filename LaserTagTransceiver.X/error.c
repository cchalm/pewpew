#include "error.h"

#include "LEDDisplay.h"
#include "system.h"

#include <stdint.h>
#include <xc.h>

void fatal(uint16_t error_code)
{
    GIE = 0;
    while (1)
    {
        setLEDDisplay(error_code);
        delay(50);
        setLEDDisplay(0);
        delay(100);
        setLEDDisplay(error_code);
        delay(50);
        setLEDDisplay(0);
        delay(100);
        setLEDDisplay(error_code);
        delay(50);
        setLEDDisplay(0);
        delay(300);
    }
}
