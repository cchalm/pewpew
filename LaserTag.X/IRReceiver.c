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

// TMR1 clocks at (Fosc / 4) and is prescaled 8x. 32MHz / 4 / 8 = 1MHz
#define TMR1_FREQ 1000000 // 1MHz
// TMR1 period in microseconds. Assumes period is a whole integer
#define TMR1_PERIOD_US (1000000 / (TMR1_FREQ))

// Pulse lengths in terms of TMR1 cycles
#define PULSE_GAP_LENGTH_TMR1_CYCLES  ((PULSE_GAP_LENGTH_US) / (TMR1_PERIOD_US))
#define ZERO_PULSE_LENGTH_TMR1_CYCLES ((ZERO_PULSE_LENGTH_US) / (TMR1_PERIOD_US))
#define ONE_PULSE_LENGTH_TMR1_CYCLES  ((ONE_PULSE_LENGTH_US) / (TMR1_PERIOD_US))

#define MIN_TRANSMISSION_GAP_LENGTH_TMR1_CYCLES ((MIN_TRANSMISSION_GAP_LENGTH_US) / (TMR1_PERIOD_US))

// Pulses come out of the receiver longer than sent, and gaps come out shorter
// than sent. Adjust lengths using this value to get accurate measurements.
// Units are microseconds. Estimated from TSOP2240 datasheet, figure 4, then
// adjusted empirically
#define PULSE_LENGTH_BIAS_US 35
// Units are TMR1 cycles
#define PULSE_LENGTH_BIAS_TMR1_CYCLES ((PULSE_LENGTH_BIAS_US) / (TMR1_PERIOD_US))

// Pulse lengths within this many microseconds of each other will be considered
// equivalent
#define PULSE_LENGTH_EPSILON_US 50
// Units are TMR1 cycles
#define PULSE_LENGTH_EPSILON_TMR1_CYCLES ((PULSE_LENGTH_EPSILON_US) / (TMR1_PERIOD_US))

#define PULSE_GAP_UPPER_LENGTH_TMR1_CYCLES ((PULSE_GAP_LENGTH_TMR1_CYCLES) + (PULSE_LENGTH_EPSILON_TMR1_CYCLES))
#define PULSE_GAP_LOWER_LENGTH_TMR1_CYCLES ((PULSE_GAP_LENGTH_TMR1_CYCLES) - (PULSE_LENGTH_EPSILON_TMR1_CYCLES))
#define ZERO_PULSE_UPPER_LENGTH_TMR1_CYCLES ((ZERO_PULSE_LENGTH_TMR1_CYCLES) + (PULSE_LENGTH_EPSILON_TMR1_CYCLES))
#define ZERO_PULSE_LOWER_LENGTH_TMR1_CYCLES ((ZERO_PULSE_LENGTH_TMR1_CYCLES) - (PULSE_LENGTH_EPSILON_TMR1_CYCLES))
#define ONE_PULSE_UPPER_LENGTH_TMR1_CYCLES ((ONE_PULSE_LENGTH_TMR1_CYCLES) + (PULSE_LENGTH_EPSILON_TMR1_CYCLES))
#define ONE_PULSE_LOWER_LENGTH_TMR1_CYCLES ((ONE_PULSE_LENGTH_TMR1_CYCLES) - (PULSE_LENGTH_EPSILON_TMR1_CYCLES))

typedef unsigned short TMR1_t;

static volatile bool g_transmission_received = false;
static volatile unsigned int g_transmission_data = 0;

#define RECORD_GAPS

static volatile TMR1_t g_pulses_received[TRANSMISSION_DATA_LENGTH];
#ifdef RECORD_GAPS
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

            pulse_width -= PULSE_LENGTH_BIAS_US;

            // Only process the pulse if we're not waiting for a long silence to
            // reset.
            if (!wait_for_silence)
            {
                g_pulses_received[bit_count] = pulse_width;

                // Based on the TSOP2440 spec, we may want to measure pulse
                // periods instead of high/low times. At low and high
                // irradiances, high/low times become irregular, but the period
                // remains consistent

                // The application (laser tag) means we may well receive
                // low-irradiance signals at times, e.g. at near-maximum optical
                // range, or a near-miss shot

                // Of particular concern is that around 0.1 mW/m^2 the high
                // time drops by over 100us, which could cause a one to be
                // interpreted as a zero

                // We could measure period in addition to duty cycle, which
                // might provide a way for a tagger to detect a near-hit. E.g.
                // if the duty cycle is low, irradiance was low, indicating an
                // off-target or distant shot

                if (pulse_width > ZERO_PULSE_LOWER_LENGTH_TMR1_CYCLES
                        && pulse_width < ZERO_PULSE_UPPER_LENGTH_TMR1_CYCLES)
                {
                    // Received a 0
                    data <<= 1;
                    bit_count++;
                }
                else if (pulse_width > ONE_PULSE_LOWER_LENGTH_TMR1_CYCLES
                        && pulse_width < ONE_PULSE_UPPER_LENGTH_TMR1_CYCLES)
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

#ifdef DEBUG
                    bit_count++;
#else
                    wait_for_silence = true;
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

            pulse_width += PULSE_LENGTH_BIAS_US;

#ifdef RECORD_GAPS
            g_gaps_received[bit_count] = pulse_width;
#endif

#ifdef DEBUG
            if (bit_count == SHOT_DATA_LENGTH)
#else
            if (pulse_width >= MIN_TRANSMISSION_GAP_LENGTH_TMR1_CYCLES)
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

