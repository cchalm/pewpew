#include "IRTransmitter.h"

#include "error.h"
#include "pins.h"
#include "pps.h"
#include "system.h"
#include "transmissionConstants.h"

#include <stdbool.h>
#include <xc.h>

/*
 * HOW IT WORKS
 *
 * Creating a transmission involves five modules: CLC1, CLC2, Timer2, PWM3, and
 * Timer6.
 *
 * The output is ultimately produced by the CLC1 module. CLC1 ANDs together a
 * carrier signal and a modulation signal. The carrier signal is a 40kHz square
 * wave. When the modulation signal is HIGH, the carrier signal is fed to the
 * output. When the modulation signal is LOW, the output is low.
 *
 * CLC1 does not synchronize the output with the carrier signal, so the
 * modulation signal must be synchronized with the carrier signal. We achieve
 * this by a) clocking the modulation signal timer from the carrier signal
 * itself, as mentioned below, and b) toggling the modulation signal in hardware
 * rather than software, so that it is effectively instantaneous.
 *
 * The carrier signal is produced by a PWM module, which uses an 8-bit timer.
 *
 * The modulation signal is produced by CLC2. CLC2 is set up to toggle on each
 * rising edge of the Timer2 postscaler output.
 *
 * Timer2's postscaler output goes high for one clock cycle when the postscaler
 * reaches a certain value. We have that value set to one. The period of the
 * timer therefore determines the time between postscaler output pulses, which
 * determines the pulse length of CLC2's output. In the interrupt handler for a
 * period match we change the timer period to set the length of the next pulse
 * or gap.
 *
 * The clock source for the modulation timer is the PWM signal itself.
 *
 * To start a transmission we preload the modulation timer's period register,
 * clear the timer, and start the timer.
 */

static void configureTimer6(void)
{
    // Timer 6 is used by PWM3, which is used to construct the modulated high
    // carrier signal

    // Set clock source to Fosc / 4 (8MHz). !!This is the only valid clock
    // source for PWM!!
    T6CLKCONbits.CS = 0b0000;

    // Set timer 6 period such that the PWM frequency (f) is 56kHz
    // T6PR = -1 + Fosc / (f * 4 * prescale)
    //      = -1 + 32MHz / (56kHz * 4 * 1)
    //      = 141.9
    T6PR = 142;

    // No interrupts from this timer
    TMR6IE = 0;
}

static void configurePWM3(void)
{
    // Set PWM7 timer source to TMR6
    CCPTMRSbits.P3TSEL = 0b10;
    // Set duty cycle register
    // on:off ratio = PWMxDC / (4 * (T6PR + 1))
    // PWMxDC = 0.50 * (4 * (142 + 1)) = 286
    // NOTE: the 6 LSBs are dummies
    PWM3DC = 286 << 6;

    // Internally supplied to CLC1

    // Supplied to Timer2 via external pin
    PPS_TARGET_CARRIER_SIGNAL = PPS_SOURCE_PWM3_out;
    // Enable the output driver for the pin we're outputting the carrier signal
    // to. We're using the pin as both an output and an input, and in output
    // mode it can do both
    TRIS_CARRIER_SIGNAL = 0;
}

static void configureTimer2(void)
{
    // We use Timer2 to produce pulses marking the start and end of desired
    // modulation pulses. We then pipe the pulses to a flip-flop to produce the
    // modulation signal

    // Set clock source to external pin
    T2CLKCONbits.CS = 0b0110;
    // Sync the clock source to the instruction clock. This fixes a glitch with
    // period double-buffering where the period buffer isn't copied into the
    // period register when expected.
    T2PSYNC = 1;
    // Configure the transmission carrier signal as the clock source.
    // The documentation talks about T2INPPS and T2CKINPPS, neither of which
    // exist. So I'm assuming T2PPS is what it meant. This could be used for two
    // things though: external reset signal selection, or clock selection, so we
    // should verify that it does what we intend
    T2PPS = PPS_SOURCE_CARRIER_SIGNAL;
    // DO NOT configure the carrier signal pin as an input. We need the output
    // driver to be active because we are outputting the PWM signal to the pin.
    // A pin configured as an output can still be read. The PWM configure
    // function will enable the output driver

    // Timer2 postscaler output internally supplied to CLC2

    // Enable period match interrupts
    TMR2IE = 1;
}

static void configureCLC2(void)
{
    // Select Timer2 postscaler output as one input
    CLC2SEL0 = CLC_SOURCE_TMR2_postscaled;
    // Select the output of CLC2 (this module) as another input
    CLC2SEL1 = CLC_SOURCE_LC2_out;

    // We don't care what the remaining two inputs are set to, as we are not
    // going to wire them up to any of the gates

    // Gate 0 receives CLCIN0. Neither the input nor the output are inverted
    CLC2GLS0 = 0b00000010;
    LC2G1POL = 0;
    // Gate 1 receives CLCIN1. The output of this gate is inverted. The gate is
    // sent to the data input of the flip-flop, so that each clock cycle toggles
    // the output of the logic cell
    CLC2GLS1 = 0b00001000;
    LC2G2POL = 1;
    // Gate 2 receives nothing, always outputs LOW
    CLC2GLS2 = 0;
    LC2G3POL = 0;
    // Gate 3 receives nothing, always outputs LOW
    CLC2GLS3 = 0;
    LC2G4POL = 0;

    // Configure the logic circuit to AND the four gates
    CLC2CONbits.LC2MODE = CLC_LOGIC_FUNCTION_2IN_DFLIPFLOP_R;

    // The output of CLC2 is not inverted
    LC2POL = 0;

    // The output of CLC2 is provided internally to CLC1

    // For debugging: output modulation signal on a pin
    PPS_TARGET_MODULATION_SIGNAL = PPS_SOURCE_LC2_out;
    TRIS_MODULATION_SIGNAL = 0;
}

static void configureCLC1(void)
{
    // Select PWM3 as one input (the carrier signal)
    CLC1SEL0 = CLC_SOURCE_PWM3_out;
    // Select the output of CLC2 as another input (the modulation signal)
    CLC1SEL1 = CLC_SOURCE_LC2_out;

    // We don't care what the remaining two inputs are set to, as we are not
    // going to wire them up to any of the gates

    // Take the modulation signal as an input
    CLCIN0PPS = PPS_SOURCE_MODULATION_SIGNAL;
    // DO NOT configure the modulation signal pin as an input. We need the
    // output driver to be active because we are outputting the modulation
    // signal to the pin. A pin configured as an output can still be read. The
    // modulation signal output module will enable the output driver

    // Gate 0 receives CLCIN0. The output is inverted.
    // The output of this gate is inverted to smooth over an output glitch. The
    // modulation signal and carrier signals are synchronized, so leading edges
    // of the modulation signal always happen at the "same time" as leading
    // edges of the carrier signal. If we don't invert the carrier signal before
    // combining it with the modulation signal, at the end of a pulse the
    // modulation signal will try to go low while the carrier signal goes high,
    // producing a small output glitch. By inverting the carrier signal, we move
    // the contentious moment to the start of the pulse, where the modulation
    // signal goes high at the same time as the carrier signal goes low. Because
    // the modulation signal is driven by the carrier signal, though, it goes
    // high ever so slightly after the carrier signal goes low, so there is no
    // glitch.
    CLC1GLS0 = 0b00000010;
    LC1G1POL = 1;
    // Gate 1 receives CLCIN1. Neither the input nor the output are inverted
    CLC1GLS1 = 0b00001000;
    LC1G2POL = 0;
    // Gate 2 receives nothing, always outputs HIGH
    CLC1GLS2 = 0;
    LC1G3POL = 1;
    // Gate 3 receives nothing, always outputs HIGH
    CLC1GLS3 = 0;
    LC1G4POL = 1;

    // Configure the logic circuit to AND the four gates
    CLC1CONbits.LC1MODE = CLC_LOGIC_FUNCTION_4IN_AND;

    // The output of CLC1 is not inverted
    LC1POL = 0;

    // Direct the output of CLC1 to the IR LED
    PPS_TARGET_IR_LED = PPS_SOURCE_LC1_out;
}

// TMR2 clocks at the carrier signal frequency
#define TMR2_FREQ (MODULATION_FREQ)

// Ratio of TMR2 clock frequency to the carrier signal modulation frequency.
// Assumes they divide evenly
#define TMR2_MOD_CLOCK_RATIO ((TMR2_FREQ) / (MODULATION_FREQ))

// Pulse lengths in terms of TMR2 cycles
#define PULSE_GAP_LENGTH_TMR2_CYCLES ((PULSE_GAP_LENGTH_MOD_CYCLES) * (TMR2_MOD_CLOCK_RATIO))
#define ZERO_PULSE_LENGTH_TMR2_CYCLES ((ZERO_PULSE_LENGTH_MOD_CYCLES) * (TMR2_MOD_CLOCK_RATIO))
#define ONE_PULSE_LENGTH_TMR2_CYCLES ((ONE_PULSE_LENGTH_MOD_CYCLES) * (TMR2_MOD_CLOCK_RATIO))

// Number of TMR2 cycles in a single period of the PWM carrier signal
#define MOD_PERIOD_TMR2_CYCLES (TMR2_MOD_CLOCK_RATIO)
#define MOD_TIMER_PERIOD_PRELOAD (MOD_PERIOD_TMR2_CYCLES)

typedef uint8_t TMR2_t;

static bool g_transmitting = false;
static uint16_t g_data_to_transmit = 0;
static uint8_t g_data_length = 0;

// Set to true by interrupt handler to indicate that a period match occurred
static volatile bool g_period_match = false;

static void setNextPeriod(bool* next_match_ends_transmission_out)
{
    // Send MSB first
    static uint8_t transmission_data_index = 0;
    static bool is_output_high = false;

    // We're always one output cycle ahead. When the output is low, we're
    // setting the period of the following high pulse, and vice versa.

    if (is_output_high)
    {
        // Output is high. Set the period of the next low pulse

        if (transmission_data_index == g_data_length)
        {
            // We're in the middle of the final high pulse, so the next period
            // match is the final match in this transmission

            *next_match_ends_transmission_out = true;

            // Reset the data index for the next transmission
            transmission_data_index = 0;

            // Set the period to something high, to give us time to disable the
            // interrupts etc before the next match. Setting to zero doesn't
            // work, because the timer will be reset to 0 on the next cycle (if
            // it hasn't already)
            PR2 = 0xFF;
        }
        else
        {
            // Load the desired gap length into the timer's period register.
            // The timer resets to zero on the clock cycle following a match,
            // not on the same clock cycle as the match. Subtract one from the
            // pulse length to compensate
            PR2 = PULSE_GAP_LENGTH_TMR2_CYCLES - 1;
        }
    }
    else // output is low
    {
        // Output is low. Set the period of the next high pulse

        transmission_data_index++;

        TMR2_t pulse_width =
                ((g_data_to_transmit >> (g_data_length - transmission_data_index)) & 1) ?
                    ONE_PULSE_LENGTH_TMR2_CYCLES : ZERO_PULSE_LENGTH_TMR2_CYCLES;

        // Load the desired pulse length into the modulation timer's period
        // register. The timer resets to zero on the clock cycle following a
        // match, not on the same clock cycle as the match. Subtract one from
        // the pulse length to compensate
        PR2 = pulse_width - 1;
    }

    is_output_high = !is_output_high;
}

static void enableTransmissionModules(void)
{
    // Clear PWM timer register
    TMR6 = 0;
    // Enable PWM timer
    TMR6ON = 1;

    // Enable PWM
    PWM3EN = 1;

    // Preload the period register to trigger a match after some number of
    // cycles. This will get copied into TMR0H_ACTUAL when we start the timer
    PR2 = MOD_TIMER_PERIOD_PRELOAD;
    // Clear the modulation timer register
    TMR2 = 0;
    // Clear modulation timer's interrupt flag
    TMR2IF = 0;
    // Enable interrupts from modulation timer
    TMR2IE = 1;
    // Enable modulation timer
    TMR2ON = 1;

    // Set the period of the next pulse. This will get copied to the actual
    // period buffer on the next period match, the one we hard-coded above
    setNextPeriod(NULL);

    // Enable CLC2
    LC2EN = 1;

    // Enable CLC1
    LC1EN = 1;
    // Enable output driver for the IR LED pin
    TRIS_IR_LED = 0;
}

static void disableTransmissionModules(void)
{
    // Disable output driver for the IR LED pin
    TRIS_IR_LED = 1;
    // Disable CLC1
    LC1EN = 0;

    // Disable CLC2
    LC2EN = 0;

    // Disable interrupts from modulation timer
    TMR2IE = 0;
    // Disable modulation timer
    TMR2ON = 0;

    // Disable PWM
    PWM3EN = 0;

    // Disable PWM timer
    TMR6ON = 0;
}

void initializeTransmitter()
{
    disableTransmissionModules();

    configureTimer6();
    configurePWM3();
    configureTimer2();
    configureCLC2();
    configureCLC1();
}

void transmitterInterruptHandler()
{
    if (!(TMR2IF && TMR2IE))
        return;

    if (g_period_match)
    {
        fatal(ERROR_UNHANDLED_PERIOD_MATCH);
    }

    g_period_match = true;

    // Reset flag
    TMR2IF = 0;
}

static void endTransmission(void)
{
    disableTransmissionModules();
    // Let the main program know we're done transmitting
    g_transmitting = false;
}

void transmitterEventHandler(void)
{
    static bool next_match_ends_transmission = false;

    if (g_period_match)
    {
        if (next_match_ends_transmission)
        {
            endTransmission();
            next_match_ends_transmission = false;
            g_period_match = false;
        }
        else
        {
            setNextPeriod(&next_match_ends_transmission);
            g_period_match = false;
        }
    }
}

bool transmitAsync(uint16_t data, uint8_t length)
{
    if (g_transmitting)
        return false;
    g_transmitting = true;

    g_data_to_transmit = data;
    g_data_length = length;

    // This starts the asynchronous dominoes that send the transmission
    enableTransmissionModules();

    return true;
}

#define EVALUATE_CONSTANTS
#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const volatile uint32_t TMR2_FREQ_eval = TMR2_FREQ;
const volatile uint16_t TMR2_MOD_CLOCK_RATIO_eval = TMR2_MOD_CLOCK_RATIO;
const volatile uint16_t PULSE_GAP_LENGTH_TMR2_CYCLES_eval = PULSE_GAP_LENGTH_TMR2_CYCLES;
const volatile uint16_t ZERO_PULSE_LENGTH_TMR2_CYCLES_eval = ZERO_PULSE_LENGTH_TMR2_CYCLES;
const volatile uint16_t ONE_PULSE_LENGTH_TMR2_CYCLES_eval = ONE_PULSE_LENGTH_TMR2_CYCLES;
const volatile uint16_t MOD_PERIOD_TMR2_CYCLES_eval = MOD_PERIOD_TMR2_CYCLES;
#endif
