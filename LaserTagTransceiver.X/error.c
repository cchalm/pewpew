#include "error.h"

#include "pins.h"
#include "system.h"

#include <stdint.h>
#include <xc.h>

void fatal(uint16_t error_code)
{
    // Stop all interrupt handling
    GIE = 0;

    // Disable all system modules to stop asynchronous behavior and release shared buses
    shutdownSystem();

    // Disable all output drivers
    // TRISA = ~0;
    // TRISB = ~0;
    // TRISC = ~0;
    // Enable error LED output driver
    TRIS_ERROR_LED = 0;
    PIN_ERROR_LED = 0;
    delay(50);
    while (1)
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            if ((error_code & (1 << i)) != 0)
            {
                // Two flashes for a `1`
                PIN_ERROR_LED = 1;
                delay(100);
                PIN_ERROR_LED = 0;
                delay(200);
                PIN_ERROR_LED = 1;
                delay(100);
                PIN_ERROR_LED = 0;
                delay(200);
            }
            else
            {
                // One flash for a `0`
                PIN_ERROR_LED = 1;
                delay(100);
                PIN_ERROR_LED = 0;
                delay(200);
            }
            delay(300);
        }
        delay(1000);
    }
}
