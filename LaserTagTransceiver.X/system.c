#include "system.h"

#include "IRReceiver.h"
#include "IRTransmitter.h"
#include "i2cSlave.h"
#include "pins.h"

#include <xc.h>

void configureSystem(void)
{
    // Configure internal oscillator for 32MHz (16MHz x2 PLL)
    OSCCONbits.IRCF = 0b1111;

    INTCONbits.GIE = 0;   // Disable active interrupts. This should be set to 1
                          // before starting program logic
    INTCONbits.PEIE = 1;  // Enable peripheral interrupts

    // Disable analog inputs. This fixes a read-modify-write issue with setting
    // individual output pins.
    ANSELA = 0;
    ANSELC = 0;

    irReceiver_initialize();
    irTransmitter_initialize();
    i2cSlave_initialize();

    // Wait for the oscillator to be ready before continuing
    while (!OSTS)
        ;

    TRIS_ERROR_LED = 0;
}

void shutdownSystem(void)
{
    irReceiver_shutdown();
    irTransmitter_shutdown();
    i2cSlave_shutdown();
}

void _delay_gen(uint32_t d, volatile uint16_t multiplier)
{
    volatile uint32_t i = 0;
    while (i != d)
    {
        volatile uint16_t x = 0;
        while (x != multiplier)
            x++;
        i++;
    }
}

void delay(uint32_t d)
{
    _delay_gen(d, 600);
}
void delayTiny(uint32_t d)
{
    _delay_gen(d, 0);
}
