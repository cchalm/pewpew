#include "error.h"

#include "pins.h"
#include "system.h"

#include <stdint.h>
#include <xc.h>

void fatal(uint16_t error_code)
{
    GIE = 0;
    // Disable all output drivers
    //TRISA = ~0;
    //TRISB = ~0;
    //TRISC = ~0;
    // Enable error LED output driver
    TRIS_ERROR_LED = 0;
    while (1)
    {
        for (int i = 0; i < 16; i++)
        {
            PIN_ERROR_LED = 0;
            delay(50);
            if ((error_code & (1 << i)) != 0)
            {
                PIN_ERROR_LED = 1;
            }
            else
            {
                PIN_ERROR_LED = 0;
            }
            delay(200);
        }
    }
}
