/*
 * File:   IRTransmitter.c
 * Author: Chris
 *
 * Created on August 11, 2018, 9:45 PM
 */

#include <xc.h>
#include "IRTransmitter.h"

#include "system.h"
#include "transmissionConstants.h"

#include <stdbool.h>

#define TRANSMIT_PIN P1MDLBIT

typedef unsigned char TMR2_t;

static void configureTimer2()
{
    // Timer 2 is used to time P1MDLBIT switches
    
    //           +------ T2OUTPS[3:0] - Set output postscaler 1:1
    //           | +---- TMR2ON       - Turn TMR2 off for now, we'll turn it on when we shoot
    //           | | +-- T2CKPS[1:0]  - Set prescaler 1:16
    //         |--||||
    T2CON = 0b00000010;
    TMR2IE = 1;

    TRANSMIT_PIN = LOW;
}

static void configurePSMC(void)
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

void initializeTransmitter()
{
    configureTimer2();
    configurePSMC();
    TRISCbits.TRISC0 = 0; // C0 to output
}

static volatile bool g_transmitting = false;
static unsigned int g_transmission_data = 0;

void handleTransmissionTimingInterrupt()
{
    if (!(TMR2IF && TMR2IE))
        return;

    // Send MSB first
    static unsigned char transmission_data_index = TRANSMISSION_DATA_LENGTH;
    static bool next_edge_rising = true;

    if (next_edge_rising)
    {
        TRANSMIT_PIN = HIGH;
        next_edge_rising = false;
        transmission_data_index--;
        
        TMR2_t pulse_width =
                ((g_transmission_data >> transmission_data_index) & 1) ? 
                    ONE_PULSE_TMR2_WIDTH : ZERO_PULSE_TMR2_WIDTH;
        PR2 = pulse_width;
    }
    else // !next_edge_rising
    {
        TRANSMIT_PIN = LOW;
        next_edge_rising = true;

        if (transmission_data_index == 0)
        {
            // This is the final falling edge of the shot
            // Turn off the timer
            TMR2ON = 0;
            // Reset the data index
            transmission_data_index = TRANSMISSION_DATA_LENGTH;
            // Let the main program know we're done transmitting
            g_transmitting = false;
        }
        else
        {
            PR2 = PULSE_GAP_TMR2_WIDTH;
        }
    }

    // Reset flag
    TMR2IF = 0;
}

bool transmitAsync(unsigned int data)
{
    if (g_transmitting)
        return false;
    g_transmitting = true;
    
    g_transmission_data = data;
    
    TMR2 = 0;
    TMR2ON = 1;
    // Force the interrupt handler to run to start the shot process
    TMR2IF = 1;
    
    return true;
}
