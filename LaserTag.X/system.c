#include "system.h"

#include "IRReceiver.h"
#include "IRTransmitter.h"
#include "LEDDisplay.h"
#include "realTimeClock.h"

#include <xc.h>

void configureSystem(void)
{
    // Configure the internal oscillator for 16/32MHz
    OSCCON = 0b11111000;

    INTCONbits.GIE = 0; // Disable active interrupts. This should be set to 1 before starting program logic
    INTCONbits.PEIE = 1; // Enable peripheral interrupts

    initializeRTC();
    initializeReceiver();
    initializeTransmitter();

    // Disable analog inputs. This fixes a read-modify-write issue with setting
    // individual output pins.
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELD = 0;
    ANSELE = 0;
    // Set B4 - B5 to output
    TRISB &= ~0b111110;
    // Set D0 - D1 to input
    TRISD |= 0b11;

    initializeLEDDisplay();
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

void error(uint16_t error_code)
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

