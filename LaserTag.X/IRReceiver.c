/*
 * File:   IRReceiver.c
 * Author: Chris
 *
 * Created on August 11, 2018, 9:45 PM
 */

#include <xc.h>
#include "IRReceiver.h"

#include "transmissionConstants.h"

#include <stdbool.h>

static void configureTimer1(void)
{
    //         +------- TMR1CS[1:0] - Set clock source: instruction clock (Fosc / 4)
    //         | +----- T1CKPS[1:0] - Set prescaler 1:8
    //         | |   +- TMR1ON - Enable Timer1
    //        ||||   |
    T1CON = 0b00110001;
}

void initializeReceiver()
{
    OPTION_REGbits.INTEDG = 0; // External interrupts: start by looking for falling edges
    INTCONbits.INTE = 1; // Enable external interrupts
    TRISBbits.TRISB0 = 1; // B0 to input

    configureTimer1();
}

typedef unsigned short TMR1_t;

static volatile bool g_transmission_received = false;
static volatile unsigned int g_transmission_data = 0;

static volatile TMR1_t g_pulses_received[TRANSMISSION_DATA_LENGTH];
#ifdef DEBUG
static volatile TMR1_t g_gaps_received[TRANSMISSION_DATA_LENGTH];
#endif

void handleSignalReceptionInterrupt()
{
    // Shot reception logic

    /*
     * HOW IT WORKS
     *
     * A shot transmission contains some number of bits. A long pulse is a 1,
     * and a short pulse is a 0, and these pulses are separated by a
     * known-length silence.
     * 
     * The current implementation uses the INT pin, a pin that can be set up to
     * trigger interrupts on rising or falling edges. The interrupt handler
     * reverses the edge on each call, allowing us to handle both rising and
     * falling edges.
     * 
     * When we see any edge, we save the value of timer 1 and reset it to 0.
     * Because we always reset it to 0, the value of the timer is the elapsed
     * time since the last edge. We time both pulses and gaps this way.
     *
     * If a pulse is an expected length, we handle the bit and continue. If it
     * is an unexpected length, we disable end-of-pulse interrupts. We handle
     * data when we see the ends of pulses, so disabling those interrupts stops
     * data reception. We also stop accepting data after receiving the expected
     * number of bits from a transmission.
     *
     * Data reception resets upon observing a long silence.
     */

    // Data accumulator
    static unsigned int data = 0;
    static unsigned char bit_count = 0;

    static bool wait_for_silence = false;
    
    static bool next_edge_rising = false;

    if (INTE && INTF)
    {
        // Investigate using the INT pin instead of the CCP module
        
        TMR1ON = 0;
        TMR1_t pulse_width = TMR1;
        TMR1 = 0;
        TMR1ON = 1;
   
        if (next_edge_rising)
        {
            // Rising edge detected (end of pulse)
            
            // Only process the pulse if we're not waiting for a long silence to
            // reset.
            if (!wait_for_silence)
            {
                g_pulses_received[bit_count] = pulse_width;

                if (pulse_width > 250 && pulse_width < 350)
                {
                    // Received a 0
                    data <<= 1;
                    bit_count++;
                }
                else if (pulse_width > 400 && pulse_width < 500)
                {
                    // Received a 1
                    data = (data << 1) | 1;
                    bit_count++;
                }
                else
                {
                    // Invalid pulse width. Something has gone wrong, so we're
                    // going to stop reading in data and wait until we see a
                    // long period of silence

#ifndef DEBUG
                    wait_for_silence = true;
#else
                    bit_count++;
#endif
                }

                if (bit_count == TRANSMISSION_DATA_LENGTH)
                {
                    g_transmission_received = true;
                    g_transmission_data = data;

                    // We could just reset the data accumulator here and let
                    // more transmission data come through immediately, on the
                    // off chance that the time between the end of one
                    // transmission and the start of the next is less than the
                    // silence reset, but instead we're just going to wait for
                    // silence. This is more robust and has a negligible impact
                    // on transmission throughput.
                    wait_for_silence = true;
                }
            }
        }
        else // !next_edge_rising
        {
            // Falling edge detected (end of silence)
#ifdef DEBUG
            g_gaps_received[bit_count] = pulse_width;
#endif
            
#ifndef DEBUG
            if (pulse_width > 300)
#else
            if (bit_count == SHOT_DATA_LENGTH)
#endif
            {
                // Long silence. Reset
                data = 0;
                bit_count = 0;

                wait_for_silence = false;
            }
        }
        
        next_edge_rising = !next_edge_rising;

        // Toggle the edge we're looking for. This can cause spurious
        // interrupts, so we disable them during the change.
        INTE = 0;
        // 1 to capture rising edges, 0 to capture falling edges
        INTEDG = next_edge_rising;
        INTF = 0;
        INTE = 1;
    }
}

bool transmissionReceived()
{
    return g_transmission_received;
}

unsigned int getTransmissionData()
{
    g_transmission_received = false;
    return g_transmission_data;
}

