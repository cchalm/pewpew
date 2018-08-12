/*
 * File:   system.c
 * Author: Chris
 *
 * Created on January 6, 2015, 9:31 PM
 */

#include "system.h"

#include "LEDDisplay.h"
#include "IRReceiver.h"
#include "IRTransmitter.h"

#include <xc.h>

void configureTimer1(void);

void configureSystem(void)
{
    // Configure the internal oscillator for 16/32MHz
    OSCCON = 0b11111000;

    // Set up Timer0
    //         +---- GIE    - Disable active interrupts. This should be set to 1 before starting program logic
    //         |+--- PEIE   - Enable peripheral interrupts
    //         ||+-- TMR0IE - Enable Timer0 interrupt
    //         |||
    //         |||
    INTCON = 0b01110000;
    
    //             +-------- RBPU    - Disable Port B pull-ups
    //             |
    //             | +------ TMR0CS  - Select Timer0 clock source: Internal instruction cycle clock
    //             | | +---- PSA     - Enable Timer0 prescaler
    //             | | | +-- PS[2:0] - Set prescaler 1:32
    //             | | ||-|
    OPTION_REG = 0b10000100;

    initializeReceiver();
    configureTimer1();
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

void configureTimer1(void)
{
    //         +------- TMR1CS[1:0] - Set clock source: instruction clock (Fosc / 4)
    //         | +----- T1CKPS[1:0] - Set prescaler 1:8
    //         | |   +- TMR1ON - Enable Timer1
    //        ||||   |
    T1CON = 0b00110001;
}

void _delay_gen(unsigned long d, volatile unsigned int multiplier)
{
    volatile unsigned long i = 0;
    while (i != d)
    {
        volatile unsigned int x = 0;
        while (x != multiplier)
            x++;
        i++;
    }
}

void delay(unsigned long d)
{
    _delay_gen(d, 600);
}

void delayTiny(unsigned long d)
{
    _delay_gen(d, 0);
}

void error(unsigned int error_code)
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

