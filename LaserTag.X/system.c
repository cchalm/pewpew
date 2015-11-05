/*
 * File:   system.c
 * Author: Chris
 *
 * Created on January 6, 2015, 9:31 PM
 */

#include <xc.h>
#include "system.h"

void configurePSMC(void);
void configureTimer1(void);
void configureTimer2(void);

void configureSystem(void)
{
    // Configure the internal oscillator for 16/32MHz
    OSCCON = 0b11111000;

    // Set up Timer0
    //         +---- GIE    - Disable active interrupts. This should be set to 1 before starting program logic
    //         |+--- PEIE   - Enable peripheral interrupts
    //         ||+-- TMR0IE - Enable Timer0 interrupt
    //         |||+- INTE   - Enable external interrupts
    //         ||||
    INTCON = 0b01110000;
    
    //             +-------- RBPU    - Disable Port B pull-ups
    //             |+------- INTEDG  - Start by looking for falling edges
    //             ||+------ TMR0CS  - Select Timer0 clock source: Internal instruction cycle clock
    //             ||| +---- PSA     - Enable Timer0 prescaler
    //             ||| | +-- PS[2:0] - Set prescaler 1:32
    //             ||| ||-|
    OPTION_REG = 0b10000100;

    configurePSMC();
    configureTimer1();
    configureTimer2();

    // Disable analog inputs. This fixes a read-modify-write issue with setting
    // individual output pins.
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELD = 0;
    ANSELE = 0;
    // Set B0 to input, B4 - B5 to output
    TRISB = 0b000001;
    // Set D0 - D1 to input, D3 - D7 to output
    TRISD = 0b00000011;
    // Set C0 to output
    TRISC = 0b0;
}

void configurePSMC(void)
{
    // Set up PSMC1 for modulated fixed duty cycle PWM
    // Disable the PSMC module before configuring it
    PSMC1CON = 0;
    //           +-------- P1MDLEN     - Enable modulation
    //           |     +-- P1MSRC[3:0] - Select P1MDLBIT as the modulation source
    //           |   |--|
    PSMC1MDL = 0b10000000;
    //              +----- P1CPRE[1:0] - Set clock prescaler to 1:1
    //              |   +- P1CSRC[1:0] - Select PSMC1 clock source: 64MHz internal
    //             ||  ||
    PSMC1CLK = 0b00000001;
    //                  +- P1PRST - Set period event to occur when PSMC1TMR == PSMC1PR
    //                  |
    PSMC1PRS = 0b00000001;
    // Set period register to 799. F = (64MHz / (PSMC1PR + 1)) / 2 = 40KHz
    PSMC1PR = 0b0000001100011111;
    //                  +- P1OEA - Enable PWM on PSMC1A
    //                  |
    PSMC1OEN = 0b00000001;

    //           +-------- PSMC1EN     - Enable PSMC1
    //           |     +-- P1MODE[3:0] - Select fixed duty cycle, variable frequency, single PWM
    //           |   |--|
    PSMC1CON = 0b10001010;
}

void configureTimer1(void)
{
    //         +------- TMR1CS[1:0] - Set clock source: instruction clock (Fosc / 4)
    //         | +----- T1CKPS[1:0] - Set prescaler 1:8
    //         | |   +- TMR1ON - Enable Timer1
    //        ||||   |
    T1CON = 0b00110001;
}

void configureTimer2(void)
{
    //           +------ T2OUTPS[3:0] - Set output postscaler 1:1
    //           | +---- TMR2ON       - Turn TMR2 off for now, we'll turn it on when we shoot
    //           | | +-- T2CKPS[1:0]  - Set prescaler 1:16
    //         |--||||
    T2CON = 0b00000010;
    TMR2IE = 1;
}

void setLEDDisplay(unsigned int bits)
{
    bits = ~bits;
    LATD = (LATD & 0b00000011) | ((bits & 0b0000000011) << 2) | ((bits & 0b1111000000) >> 2);
    LATC = (LATC & 0b00001111) | ((bits & 0b0000111100) << 2);
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
