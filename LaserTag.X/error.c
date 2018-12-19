#include "error.h"

#include "LEDs.h"
#include "system.h"

#include <stdint.h>
#include <xc.h>

void fatal(uint16_t error_code)
{
    GIE = 0;
    while (1)
    {
        /*
        setBarDisplay1(error_code);
        delay(50);
        setBarDisplay1(0);
        delay(100);
        setBarDisplay1(error_code);
        delay(50);
        setBarDisplay1(0);
        delay(100);
        setBarDisplay1(error_code);
        delay(50);
        setBarDisplay1(0);
        delay(300);
        */

        for (int i = 0; i < 10; i++)
        {
            if (((error_code >> i) & 1) == 1)
            {
                LATAbits.LATA4 = 0;
                NOP();
                LATAbits.LATA4 = 1;
            }
            delay(200);
        }
    }
}
