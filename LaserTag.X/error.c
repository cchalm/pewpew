#include "error.h"

#include "LEDs.h"
#include "system.h"

#include <stdint.h>
#include <xc.h>

void fatal(uint8_t error_code)
{
    // Stop all interrupt handling
    GIE = 0;

    // Disable all system modules to stop asynchronous behavior and release shared buses
    shutdownSystem();

    while (1)
    {
        for (int i = 0; i < 5; i++)
        {
            if (((error_code >> i) & 1) == 1)
            {
                flashMuzzleLight();
            }
            else
            {
                flashHitLight();
            }
            delay(300);
        }
        delay(1000);
    }
}
