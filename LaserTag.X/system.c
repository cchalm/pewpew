#include "system.h"

#include "crc.h"
#include "i2cMaster.h"
#include "LEDDisplay.h"
#include "realTimeClock.h"

#include <xc.h>

void configureSystem(void)
{
    // Configure internal oscillator for 32MHz (16MHz x2 PLL)
    OSCCONbits.IRCF = 0b1111;

    INTCONbits.GIE = 0; // Disable active interrupts. This should be set to 1 before starting program logic
    INTCONbits.PEIE = 1; // Enable peripheral interrupts

    // Disable analog inputs. This fixes a read-modify-write issue with setting
    // individual output pins.
    ANSELA = 0;
    ANSELC = 0;

    initializeLEDDisplay();
    initializeRTC();
    initializeCRC();
    initializeI2CMaster();
    
    LATA = 0b00110000;
    LATC = 0b00000000;

    // Set A4 - A5 to output
    TRISA &= ~0b110000;
    // Set C1 - C2 to input for user inputs, and C3 - C4 to input for MSSP
    TRISC |= 0b11110;
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

