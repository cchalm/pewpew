#include "IRTransmitter.h"

#include "bitwiseUtils.h"
#include "error.h"
#include "system.h"
#include "transmissionConstants.h"

#include <stdbool.h>
#include <xc.h>

/*
 * HOW IT WORKS
 *
 * Creating a transmission involves four modules: DSM, Timer0, PWM7, and Timer6.
 *
 * The output is ultimately produced by the Data Signal Modulator (DSM) module.
 * The DSM combines a carrier signal with a modulation signal. The carrier
 * carrier signal is a 40kHz PWM signal. When the modulation signal is HIGH, the
 * carrier wave is fed to the output. When the modulation signal is LOW, the
 * output is low.
 *
 * The carrier signal is produced by a PWM module, which uses an 8-bit timer.
 *
 * The modulation signal is produced by Timer0. The output of Timer0 toggles on
 * each rising edge of an internal signal that pulses on each period match. Each
 * period match also clears the timer register. In the interrupt handler for a
 * period match we change the timer period to set the length of the ongoing
 * pulse or gap.
 *
 * The clock source for the modulation timer is the PWM signal itself.
 *
 * To start a transmission we preload the modulation timer's period register,
 * clear the timer, and start the timer.
 */

static void configureTimer6(void)
{
    // Timer 6 is used by PWM7, which is used to construct the modulated high
    // carrier signal

    // Set clock source to Fosc / 4 (8MHz). !!This is the only valid clock
    // source for PWM!!
    T6CLK = 0b0001;

    // Set timer 6 period such that the PWM frequency (f) is 56kHz
    // T6PR = -1 + Fosc / (f * 4 * prescale)
    //      = -1 + 32MHz / (56kHz * 4 * 1)
    //      = 141.9
    T6PR = 142;

    // No interrupts from this timer
    TMR6IE = 0;
}

#define PWM7_OUT_PPS 0x0F

static void configurePWM7(void)
{
    // Set PWM7 timer source to TMR6
    CCPTMRS1bits.P7TSEL = 0b11;
    // Set duty cycle register
    // on:off ratio = PWMxDC / (4 * (T6PR + 1))
    // PWMxDC = 0.50 * (4 * (142 + 1)) = 286
    // NOTE: the 6 LSBs are dummies
    PWM7DC = 286 << 6;

    // Internally supplied to DSM

    // Externally supplied to TMR0 via A7
    RA7PPS = PWM7_OUT_PPS;
    // Enable A7 output driver. We're using A6 is both an output and an input,
    // and in output mode it can do both
    TRISA7 = 0;
}

#define RA7_IN_PPS 0x07
#define TIMER0_OUT_PPS 0x18

static void configureTimer0(void)
{
    // Timer0 is used to time output pulses. Timer 0 is the only usable timer
    // for this purpose. Timer1/3/5 doesn't have an adjustable period, and
    // Timer2/4/6 has a double-buffered period register that only updates on
    // specific, inconvenient events, and seems to have bugs when clocked from
    // anything other than the instruction clock.

    // Timer0 is also perfectly suited for this application because it
    // automatically toggles its output on each period match.

    // We are using Timer0 as an 8-bit timer.

    // Set clock source to external pin (inverted)
    T0CON1bits.T0CS = 0b001;
    // Configure A7 as the clock source pin, we will get the PWM signal from
    // this pin
    T0CKIPPS = RA7_IN_PPS;
    // DO NOT configure A6 as an input. We need the output driver to be active
    // because we are outputting the PWM signal to the pin. A pin configured
    // as an output can still be read. The PWM configure function will enable
    // the output driver

    // Configure C0 as the timer output pin
    RC0PPS = TIMER0_OUT_PPS;
    // Enable C0 output driver
    TRISC0 = 0;

    // Enable period match interrupts
    TMR0IE = 1;
}

#define DSM_OUT_PPS 0x1B

#define A6_IN_PPS 0x06

static void configureDSM(void)
{
    // Synchronize output with the high carrier signal (the modulated one)
    MDCHSYNC = 1;
    // Set the modulation source to the external MDMSRC pin, whose location is
    // configured by MDSRCPPS
    MDSRC = 0b00000;
    // Set carrier high signal source to PWM7_out
    MDCARH = 0b1010;
    // Set carrier low signal source to Vss. There doesn't seem to be a way to
    // do this properly, so for now we're using the output of the NCO module,
    // because it should be unused. To be safe, we're disabling the module too.
    // TODO look into why we can't set the source to Vss
    MDCARL = 0b1011;
    NCOMD = 1;

    // Select A6 as the modulation source pin
    MDSRCPPS = A6_IN_PPS;
    TRISA6 = 1; // A6 to input

    // Select A0 as the output pin for the DSM
    RA0PPS = DSM_OUT_PPS;
}

// TMR0 clocks at the carrier signal frequency
#define TMR0_FREQ (MODULATION_FREQ)

// Ratio of TMR0 clock frequency to the carrier signal modulation frequency.
// Assumes they divide evenly
#define TMR0_MOD_CLOCK_RATIO ((TMR0_FREQ) / (MODULATION_FREQ))

/*
 * The DSM output is synchronized to the high carrier signal, which means
 * modulation line changes are reflected in the following cycle of the high
 * carrier signal rather than immediately. Both the rising and falling edges of
 * the modulation signal are synchronized.
 */

// Pulse lengths in terms of TMR0 cycles
#define PULSE_GAP_LENGTH_TMR0_CYCLES ((PULSE_GAP_LENGTH_MOD_CYCLES) * (TMR0_MOD_CLOCK_RATIO))
#define ZERO_PULSE_LENGTH_TMR0_CYCLES ((ZERO_PULSE_LENGTH_MOD_CYCLES) * (TMR0_MOD_CLOCK_RATIO))
#define ONE_PULSE_LENGTH_TMR0_CYCLES ((ONE_PULSE_LENGTH_MOD_CYCLES) * (TMR0_MOD_CLOCK_RATIO))

// Number of TMR0 cycles in a single period of the PWM carrier signal
#define MOD_PERIOD_TMR0_CYCLES (TMR0_MOD_CLOCK_RATIO)
#define MOD_TIMER_PERIOD_PRELOAD (MOD_PERIOD_TMR0_CYCLES)

typedef uint8_t TMR0_t;

static bool g_transmitting = false;
static uint16_t g_data_to_transmit = 0;

// Set to true by interrupt handler to indicate that a period match occurred
static volatile bool g_period_match = false;

static void setNextPeriod(bool* next_match_ends_transmission_out)
{
    // Send MSB first
    static uint8_t transmission_data_index = TRANSMISSION_LENGTH;
    static bool is_output_high = false;

    // We're always one output cycle ahead. When the output is low, we're
    // setting the period of the following high pulse, and vice versa.

    if (is_output_high)
    {
        // Output is high. Set the period of the next low pulse

        if (transmission_data_index == 0)
        {
            // We're in the middle of the final high pulse, so the next period
            // match is the final match in this transmission

            *next_match_ends_transmission_out = true;

            // Reset the data index for the next transmission
            transmission_data_index = TRANSMISSION_LENGTH;

            // Set the period to 0. When this gets copied into the buffer on the
            // next match, it will stop the timer from incrementing or toggling
            // the output
            TMR0H = 0;
        }
        else
        {
            // Load the desired gap length into the timer's period register.
            // The timer resets to zero on the clock cycle following a match,
            // not on the same clock cycle as the match. Subtract one from the
            // pulse length to compensate
            TMR0H = PULSE_GAP_LENGTH_TMR0_CYCLES - 1;
        }
    }
    else // output is low
    {
        // Output is low. Set the period of the next high pulse

        transmission_data_index--;

        TMR0_t pulse_width =
                ((g_data_to_transmit >> transmission_data_index) & 1) ?
                    ONE_PULSE_LENGTH_TMR0_CYCLES : ZERO_PULSE_LENGTH_TMR0_CYCLES;

        // Load the desired pulse length into the modulation timer's period
        // register. The timer resets to zero on the clock cycle following a
        // match, not on the same clock cycle as the match. Subtract one from
        // the pulse length to compensate
        TMR0H = pulse_width - 1;
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
    PWM7EN = 1;
    // Clear the modulation timer register
    TMR0L = 0;
    // Preload the period register to trigger a match after some number of
    // cycles. This will get copied into TMR0H_ACTUAL when we start the timer
    TMR0H = MOD_TIMER_PERIOD_PRELOAD;
    // Clear modulation timer's interrupt flag
    TMR0IF = 0;
    // Enable modulation timer
    T0EN = 1;

    // Set the period of the next pulse. This will get copied to the actual
    // period buffer on the next period match, the one we hard-coded above
    setNextPeriod(NULL);

    // Enable DSM
    MDEN = 1;
    // Enable A0 output driver for DSM
    TRISA0 = 0;
}

static void disableTransmissionModules(void)
{
    // Disable A0 output driver for DSM
    TRISA0 = 1;
    // Disable DSM
    MDEN = 0;

    // Disable modulation timer
    T0EN = 0;

    // Disable PWM
    PWM7EN = 0;

    // Disable PWM timer
    TMR6ON = 0;
}

void initializeTransmitter()
{
    // DSM uses PWM7 and Timer0. PWM7 uses TMR6
    configureTimer6();
    configurePWM7();
    configureTimer0();
    configureDSM();
}

void transmitterInterruptHandler()
{
    if (!(TMR0IF && TMR0IE))
        return;

    if (g_period_match)
        fatal(ERROR_UNHANDLED_PERIOD_MATCH);

    g_period_match = true;

    // Reset flag
    TMR0IF = 0;
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
        else if (TMR0L > 0)
        {
            // I don't fully understand what's going on here, but I know that if
            // you write to TMR0H while TMR0L is zero here, TMR0H immediately
            // gets copied into TMR0H_ACTUAL. We don't want that, we want
            // TMR0H_ACTUAL to be updated on the next match, so wait until
            // TMR0L > 0 to set the next period

            setNextPeriod(&next_match_ends_transmission);
            g_period_match = false;
        }
    }
}

bool transmitAsync(uint16_t data)
{
    if (g_transmitting)
        return false;
    g_transmitting = true;

    // Append parity bits to the transmission
    uint8_t bit_sum = sumBits(data);
    uint8_t parity_bits = bit_sum & ((1 << NUM_PARITY_BITS) - 1);
    g_data_to_transmit = (data << NUM_PARITY_BITS) | parity_bits;

    // This starts the asynchronous dominoes that send the transmission
    enableTransmissionModules();

    return true;
}

#define EVALUATE_CONSTANTS
#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const volatile uint32_t TMR0_FREQ_eval = TMR0_FREQ;
const volatile uint16_t TMR0_MOD_CLOCK_RATIO_eval = TMR0_MOD_CLOCK_RATIO;
const volatile uint16_t PULSE_GAP_LENGTH_TMR0_CYCLES_eval = PULSE_GAP_LENGTH_TMR0_CYCLES;
const volatile uint16_t ZERO_PULSE_LENGTH_TMR0_CYCLES_eval = ZERO_PULSE_LENGTH_TMR0_CYCLES;
const volatile uint16_t ONE_PULSE_LENGTH_TMR0_CYCLES_eval = ONE_PULSE_LENGTH_TMR0_CYCLES;
const volatile uint16_t MOD_PERIOD_TMR0_CYCLES_eval = MOD_PERIOD_TMR0_CYCLES;
#endif
