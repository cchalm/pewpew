#include "system.h"

#include "crc.h"
#include "IRReceiver.h"
#include "IRTransmitter.h"
#include "LEDDisplay.h"
#include "realTimeClock.h"

#include <xc.h>

void configureSystem(void)
{
    // Internal oscillator is configured for 32MHz at startup, via the RSTOSC
    // configuration word

    INTCONbits.GIE = 0; // Disable active interrupts. This should be set to 1 before starting program logic
    INTCONbits.PEIE = 1; // Enable peripheral interrupts

    // Disable analog inputs. This fixes a read-modify-write issue with setting
    // individual output pins.
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;

    initializeLEDDisplay();

    initializeRTC();
    initializeReceiver();
    initializeTransmitter();

    initializeCRC();

    // Set A4 - A5 to output
    TRISA &= ~0b110000;
    // TODO enable inputs after moving transmission to separate MCU
    // Set C6 - C7 to input
    //TRISC |= 0b11000000;
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

