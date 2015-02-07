/*
 * File:   system.c
 * Author: Chris
 *
 * Created on January 6, 2015, 9:31 PM
 */

#include <xc.h>
#include "system.h"

void configurePSMC(void);
void configureCCP(void);

void configureSystem(void)
{
    // Configure the internal oscillator for 16/32MHz
    OSCCON = 0b11111000;

    // Set up Timer0
    //         +--- GIE    - Disable active interrupts. This should be set to 1 before starting program logic
    //         |+-- TMR0IE - Enable Timer0 interrupt
    //         ||+- PEIE   - Enable peripheral interrupts
    //         |||
    INTCON = 0b01100000;
    //             +-------- RBPU    - Disable Port B pull-ups
    //             | +------ TMR0CS  - Select Timer0 clock source: Internal instruction cycle clock
    //             | | +---- PSA     - Enable Timer0 prescaler
    //             | | | +-- PS[2:0] - Set prescaler 1:32
    //             | | ||-|
    OPTION_REG = 0b10000100;

    configurePSMC();
    configureCCP();

    // Disable analog inputs. This fixes a read-modify-write issue with setting
    // individual output pins.
    ANSELA = 0;
    ANSELB = 0;
    ANSELD = 0;
    // Set port A to output
    TRISA = 0;
    // Set port B to output
    TRISB = 0;
    // Set D0 - D3 to output, D5 - D7 to input
    TRISD = 0b11100000;
    // Set C0 to output, C1 - C2 to input
    TRISC = 0b110;
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

void configureCCP(void)
{
    //         +------- TMR1CS[1:0] - Set clock source: instruction clock (Fosc / 4)
    //         | +----- T1CKPS[1:0] - Set prescaler 1:1
    //         | |   +- TMR1ON - Enable Timer1
    //        ||||   |
    T1CON = 0b00000001;
    //                +-- CCP1M[3:0] - Set CCP1 to capture rising edges
    //              |--|
    CCP1CON = 0b00000101;
    //                +-- CCP1M[3:0] - Set CCP2 to capture falling edges
    //              |--|
    CCP2CON = 0b00000100;
    //                +-- CCP3M[3:0] - Set CCP3 to generate a software interrupt on match
    //              |--|
    CCP3CON = 0b00001010;
    CCP1IE = 1;
    CCP2IE = 1;
}

void setLEDDisplay(unsigned int bits)
{
    LATD = ~((LATD & 0b11110000) | (bits >> 6 & 0b1111));
    LATA = ~((LATA & 0b11000000) | (bits & 0b111111));
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
