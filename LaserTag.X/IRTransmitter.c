#include "IRTransmitter.h"

#include "bitwiseUtils.h"
#include "system.h"
#include "transmissionConstants.h"

#include <stdbool.h>
#include <xc.h>

/*
 * HOW IT WORKS
 *
 * There are a whopping five modules involved in transmitting, compared to the
 * two used in past iterations. We may remove modules in the future if we find
 * them to be too much of a maintenance or power consumption cost. The advantage
 * of using all these modules is that it makes the output timing precise enough
 * to be used with a high-frequency carrier wave in the future.
 *
 * The output is ultimately produced by the Data Signal Modulator (DSM) module.
 * The DSM combines a carrier signal with a modulation signal. The carrier
 * carrier signal is a 40kHz PWM signal. When the modulation signal is HIGH, the
 * carrier wave is fed to the output. When the modulation signal is LOW, the
 * output is low.
 *
 * The carrier signal is produced by a PWM module, which uses an 8-bit timer.
 *
 * The modulation signal is produced by a CCP module in Compare mode. The CCP
 * module uses a 16-bit timer. It is configured to toggle the output, clear the
 * timer, and trigger an interrupt when the 16-bit CCPR register matches the
 * 16-bit timer register. In the interrupt handler we set CCPR for the next
 * transition. We assume the interrupt handler will run before the timer reaches
 * an applicable CCPR value.
 *
 * To start a transmission, we clear the timer and set CCPR to 0 to immediately
 * trigger the Compare event.
 *
 * TODO look into using the PWM itself as the modulation signal timer source,
 * e.g. TxCKIPPS. Try setting PWM6OUT and T4IN to the same pin and setting timer
 * 4 clock source to TxCKIPPS.
 */

static void configureTimer6(void)
{
    // Timer 6 is used by PWM6, which is used to construct the modulated high
    // carrier signal

    // Set clock source to Fosc / 4 (8MHz). !!This is the only valid clock
    // source for PWM!!
    T6CLK = 0b0001;

    // Set timer 6 period such that the PWM frequency (f) is 40kHz
    // T6PR = -1 + Fosc / (f * 4 * prescale)
    //      = -1 + 32MHz / (40kHz * 4 * 1)
    //      = 199
    T6PR = 199;

    // No interrupts from this timer
    TMR6IE = 0;
}

static void configurePWM6(void)
{
    // Set PWM6 timer source to TMR6
    CCPTMRS1bits.P6TSEL = 0b11;
    // Set duty cycle register
    // on:off ratio = PWM6DC / (4 * (T6PR + 1))
    // PWM6DC = 0.50 * (4 * 200) = 400
    // NOTE: the 6 LSBs are dummies
    PWM6DC = 400 << 6;

    // Internally supplied to DSM

#define OUTPUT_PWM_ON_A3

#ifdef OUTPUT_PWM_ON_A3
    RA3PPS = 0x0E;
    TRISA3 = 0;
#endif
}

static void configureTimer5(void)
{
    // Timer5 is used by the CCP module to time output pulses

    // Set prescaler to 1:1
    T5CONbits.CKPS = 0b00;
    // Set clock source to Fosc / 4 (8MHz). !!This is the only valid clock
    // source for Compare!!
    T5CLK = 0b0001;

    // No interrupts from this timer
    TMR6IE = 0;
}

#define CCP5_OUT_PPS 0x0D

static void configureCCP5(void)
{
    // Set CCP5 to Compare. Toggle output on match, and clear TMR5
    CCP5CONbits.CCP5MODE = 0b0001;
    // Set CCP5 timer source to TMR5
    CCPTMRS1bits.C5TSEL = 0b11;

    // Set CCP5 output pin to A2
    RA2PPS = CCP5_OUT_PPS;

    // Enable CCP5 interrupts
    CCP5IE = 1;
}

#define DSM_OUT_PPS 0x1B

#define A1_IN_PPS 0x01

static void configureDSM(void)
{
    MDEN = 1;
    // Synchronize output with the high carrier signal (the modulated one)
    MDCHSYNC = 1;
    // Set the modulation source to the external MDMSRC pin, whose location is
    // configured by MDSRCPPS
    MDSRC = 0b00000;
    // Set carrier high signal source to PWM6_out
    MDCARH = 0b1001;
    // Set carrier low signal source to Vss. There doesn't seem to be a way to
    // do this properly, so for now we're using the output of the NCO module,
    // because it should be unused. To be safe, we're disabling the module too.
    // TODO look into why we can't set the source to Vss
    MDCARL = 0b1011;
    NCOMD = 1;

    // Select A1 as the modulation source pin
    MDSRCPPS = A1_IN_PPS;
    TRISA1 = 1; // A1 to input

    // Select A0 as the output pin for the DSM
    RA0PPS = DSM_OUT_PPS;

    // Enable A0 output driver for DSM
    TRISA0 = 0;
}

// TMR5 clocks at 8MHz (Fosc/4) and is prescaled 1x
#define TMR5_FREQ 8000000

// Ratio of TMR5 clock frequency to the carrier signal modulation frequency.
// Assumes they divide evenly
#define TMR5_MOD_CLOCK_RATIO ((TMR5_FREQ) / (MODULATION_FREQ))

/*
 * The DSM output is synchronized to the high carrier signal, which means
 * modulation line changes are reflected in the following cycle of the high
 * carrier signal rather than immediately. Both the rising and falling edges of
 * the modulation signal are synchronized.
 *
 * Because the PWM module runs independently of the DSM module, we don't know
 * how the carrier signal and the modulation signal are phase-shifted relative
 * to each other. This may make it difficult to output a precise number of
 * pulses. TODO investigate.
 */

// Pulse lengths in terms of TMR5 cycles
#define PULSE_GAP_LENGTH_TMR5_CYCLES ((PULSE_GAP_LENGTH_MOD_CYCLES) * (TMR5_MOD_CLOCK_RATIO))
#define ZERO_PULSE_LENGTH_TMR5_CYCLES ((ZERO_PULSE_LENGTH_MOD_CYCLES) * (TMR5_MOD_CLOCK_RATIO))
#define ONE_PULSE_LENGTH_TMR5_CYCLES ((ONE_PULSE_LENGTH_MOD_CYCLES) * (TMR5_MOD_CLOCK_RATIO))

#define MAX_PULSE_LENGTH_TMR5_CYCLES ((MAX_PULSE_LENGTH_MOD_CYCLES) * (TMR5_MOD_CLOCK_RATIO))

#define TMR5_PRELOAD MAX_PULSE_LENGTH_TMR5_CYCLES

// Number of TMR5 cycles in a single period of the PWM carrier signal
#define MOD_PERIOD_TMR5_CYCLES (TMR5_MOD_CLOCK_RATIO)
// Preload the CCP module such that a match occurs after a delay of one PWM
// cycle, to make sure the PWM signal has time to spin up
#define CCP5_PRELOAD ((TMR5_PRELOAD) + (MOD_PERIOD_TMR5_CYCLES))

static void enableTransmissionModules(void)
{
    // Note that we don't care about interrupts with either timer, so we don't
    // have to clear their interrupt flags

    // Clear PWM timer register
    TMR6 = 0;
    // Enable PWM timer
    TMR6ON = 1;

    // Enable PWM
    PWM6EN = 1;

    // Preload the CCP timer register. This is to allow us to trigger a *single*
    // Compare event immediately. If we cleared the timer and then compared on
    // zero, the compare would trigger repeatedly because the timer is
    // automatically cleared on match
    TMR5 = TMR5_PRELOAD;
    // Enable CCP timer
    TMR5ON = 1;

    // Set compare target to force a match after a delay long enough to allow
    // the PWM signal to spin up
    CCPR5 = CCP5_PRELOAD;
    // Enable CCP5
    CCP5EN = 1;
    // Enable A2 output driver for CCP5
    TRISA2 = 0;
}

static void disableTransmissionModules(void)
{
    // Disable A2 output driver for CCP5
    TRISA2 = 0;
    // Disable CCP
    CCP5EN = 0;

    // Disable CCP timer
    TMR5ON = 0;

    // Disable PWM
    PWM6EN = 0;

    // Disable PWM timer
    TMR6ON = 0;
}

void initializeTransmitter()
{
    // Disable all modules before configuration. We'll turn them all on when we
    // transmit
    disableTransmissionModules();

    // DSM uses PWM6 and CCP5. PWM6 uses TMR6. CCP5 uses TMR4
    configureTimer6();
    configurePWM6();
    configureTimer5();
    configureCCP5();
    configureDSM();
}

typedef uint16_t TMR5_t;

static volatile bool g_transmitting = false;
static uint16_t g_transmission_data = 0;

void handleTransmissionTimingInterrupt()
{
    if (!(CCP5IF && CCP5IE))
        return;

    // Send MSB first
    static uint8_t transmission_data_index = TRANSMISSION_LENGTH;
    static bool next_edge_rising = true;

    if (next_edge_rising)
    {
        // Output is now HIGH

        next_edge_rising = false;
        transmission_data_index--;

        TMR5_t pulse_width =
                ((g_transmission_data >> transmission_data_index) & 1) ?
                    ONE_PULSE_LENGTH_TMR5_CYCLES : ZERO_PULSE_LENGTH_TMR5_CYCLES;
        CCPR5 = pulse_width;
    }
    else // !next_edge_rising
    {
        // Output is now LOW

        next_edge_rising = true;

        if (transmission_data_index == 0)
        {
            // This is the final falling edge of the shot

            disableTransmissionModules();

            // Reset the data index
            transmission_data_index = TRANSMISSION_LENGTH;
            // Let the main program know we're done transmitting
            g_transmitting = false;
        }
        else
        {
            CCPR5 = PULSE_GAP_LENGTH_TMR5_CYCLES;
        }
    }

    // Reset flag
    CCP5IF = 0;
}

bool transmitAsync(uint16_t data)
{
    if (g_transmitting)
        return false;
    g_transmitting = true;

    // Append parity bits to the transmission
    uint8_t bit_sum = sumBits(data);
    uint8_t parity_bits = bit_sum & ((1 << NUM_PARITY_BITS) - 1);
    g_transmission_data = (data << NUM_PARITY_BITS) | parity_bits;

    enableTransmissionModules();

    return true;
}

#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const uint32_t TMR5_FREQ_eval = TMR5_FREQ;
const uint16_t TMR5_MOD_CLOCK_RATIO_eval = TMR5_MOD_CLOCK_RATIO;
const uint16_t PULSE_GAP_LENGTH_TMR2_CYCLES_eval = PULSE_GAP_LENGTH_TMR2_CYCLES;
const uint16_t ZERO_PULSE_LENGTH_TMR2_CYCLES_eval = ZERO_PULSE_LENGTH_TMR2_CYCLES;
const uint16_t ONE_PULSE_LENGTH_TMR2_CYCLES_eval = ONE_PULSE_LENGTH_TMR2_CYCLES;
const uint16_t MAX_PULSE_LENGTH_TMR5_CYCLES_eval = MAX_PULSE_LENGTH_TMR5_CYCLES;
const uint16_t TMR5_PRELOAD_eval = TMR5_PRELOAD;
const uint16_t MOD_PERIOD_TMR5_CYCLES_eval = MOD_PERIOD_TMR5_CYCLES;
const uint16_t CCP5_PRELOAD_eval = CCP5_PRELOAD;
#endif
