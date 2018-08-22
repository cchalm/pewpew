#include "IRTransmitter.h"

#include "bitwiseUtils.h"
#include "system.h"
#include "transmissionConstants.h"

#include <stdbool.h>
#include <xc.h>

/*
 * HOW IT WORKS
 *
 * There are a whopping six modules involved in transmitting, compared to the
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
 * The modulation signal is produced by a CLC module that ORs two inputs: one is
 * software-controlled, the other is the output of the CCP module. The
 * software-controlled input is LOW the vast majority of the time; the CCP
 * output is the primary modulation signal driver. See the CCP interrupt handler
 * for details.
 *
 * The modulation signal is (mostly) driven by a CCP module in Compare mode. The
 * CCP module uses a 16-bit timer. It is configured to SET or CLEAR the output
 * and generate an interrupt when the 16-bit CCPR register matches the 16-bit
 * timer register. In the interrupt handler we toggle the set/clear behavior and
 * add the next desired period to the CCPR register.
 *
 * We cannot use the built-in output toggle option of the CCP module because the
 * toggle seems to clock on the instruction clock regardless of our choice of
 * timer source for the CCP module. If we choose a timer with a low-frequency
 * clock source, this quirk makes the output oscillate instead of toggling.
 *
 * The CCP module is set up to work with a variety of clock sources up to
 * Fosc/4, but the PWM signal is convenient because that is the signal we're
 * counting pulses of.
 *
 * To start a transmission we clear the CCP timer's register and set the CCP
 * register to a preload value to manually trigger a Compare event after that
 * many cycles.
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

    // Externally supplied to TMR5 via C0
    RC0PPS = PWM7_OUT_PPS;
    // Enable C0 output driver. We're using C0 is both an output and an input,
    // and in output mode it can do both
    TRISC0 = 0;
}

#define RC0_IN_PPS 0x10

static void configureTimer5(void)
{
    // Timer5 is used by the CCP module to time output pulses

    // Set prescaler to 1:1
    T5CONbits.CKPS = 0b00;
    // Set clock source to external pin
    T5CLK = 0b0000;
    // Configure C0 as the clock source pin, we will get the PWM signal from
    // this pin
    T5CKIPPS = RC0_IN_PPS;
    // DO NOT configure C0 as an input. We need the output driver to be active
    // because we are outputting the PWM signal to the pin. A pin configured
    // as an output can still be read. The PWM configure function will enable
    // the output driver
}

#define CCP5_HIGH_OVERRIDE_LAT LATA6

#define CLC1_OUT_PPS 0x01

#define CLC_INPUT_CCP5 0b010111
#define CLC_INPUT_CLCIN0PPS 0b000000
#define CLC_LOGIC_AND 0b010

#define A6_IN_PPS 0x06

static void configureCLC1(void)
{
    // Set CLC1 output pin to A2
    RA2PPS = CLC1_OUT_PPS;
    // Enable A2 output driver
    TRISA2 = 0;

    // Two inputs: CCP5 output, and a PPS-selected pin
    CLC1SEL0 = CLC_INPUT_CCP5;
    CLC1SEL1 = CLC_INPUT_CLCIN0PPS;

    // Select input pin A6
    CLCIN0PPS = A6_IN_PPS;
    // Set A6 as an OUTPUT. We will write to and read from this pin, and in
    // output mode it can do both
    TRISA6 = 0;

    // Gate 0 ORs inputs 0 and 1
    CLC1GLS0 = 0b00001010;
    LC1G1POL = 0;

    // Gates 1, 2, and 3 always output HIGH
    CLC1GLS1 = 0;
    LC1G2POL = 1;
    CLC1GLS2 = 0;
    LC1G3POL = 1;
    CLC1GLS3 = 0;
    LC1G4POL = 1;

    // Logic function is 4-input AND
    CLC1CONbits.LC1MODE = CLC_LOGIC_AND;

    // Enable the logic circuit
    LC1EN = 1;
}

#define CCP5_OUT_PPS 0x0D

static void configureCCP5(void)
{
    // Don't configure the CCP mode yet, we'll do that when we enable the module

    // Set CCP5 timer source to TMR5
    CCPTMRS1bits.C5TSEL = 0b11;

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
    // Set carrier high signal source to PWM7_out
    MDCARH = 0b1010;
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

// TMR5 clocks at the carrier signal frequency
#define TMR5_FREQ (MODULATION_FREQ)

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
#define CCP5_PRELOAD (MOD_PERIOD_TMR5_CYCLES)

static void enableTransmissionModules(void)
{
    // Note that we don't care about interrupts with either timer, so we don't
    // have to clear their interrupt flags

    // Clear PWM timer register
    TMR6 = 0;
    // Enable PWM timer
    TMR6ON = 1;

    // Enable PWM
    PWM7EN = 1;

    // Clear the CCP timer register
    TMR5 = 0;
    // Enable CCP timer
    TMR5ON = 1;

    // Configure compare module to set the output on match
    CCP5CONbits.CCP5MODE = 0b1000;
    // Set compare target to force a match after a delay long enough to allow
    // the PWM signal to spin up
    CCPR5 = CCP5_PRELOAD;
    // Enable CCP5
    CCP5EN = 1;

    // Enable CLC1
    LC1EN = 1;
}

static void disableTransmissionModules(void)
{
    // Disable CLC1
    LC1EN = 0;

    // Disable CCP
    CCP5EN = 0;

    // Disable CCP timer
    TMR5ON = 0;

    // Disable PWM
    PWM7EN = 0;

    // Disable PWM timer
    TMR6ON = 0;
}

void initializeTransmitter()
{
    // Disable all modules before configuration. We'll turn them all on when we
    // transmit
    disableTransmissionModules();

    // DSM uses PWM7 and CLC1.  PWM7 uses TMR6. CLC1 uses CCP5. CCP5 uses TMR5
    configureTimer6();
    configurePWM7();
    configureTimer5();
    configureCLC1();
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
        // Output is now HIGH. Next will be low
        next_edge_rising = false;

        transmission_data_index--;

        TMR5_t pulse_width =
                ((g_transmission_data >> transmission_data_index) & 1) ?
                    ONE_PULSE_LENGTH_TMR5_CYCLES : ZERO_PULSE_LENGTH_TMR5_CYCLES;

        CCPR5 += pulse_width;

        // Configure the CCP module to clear the output on the next match.
        // Override the output to hide the blip that occurs when we change the
        // CCP mode
        CCP5_HIGH_OVERRIDE_LAT = 1;
        CCP5CONbits.CCP5MODE = 0b1001;
        CCP5_HIGH_OVERRIDE_LAT = 0;
    }
    else // !next_edge_rising
    {
        // Output is now LOW. Next will be high
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
            CCPR5 += PULSE_GAP_LENGTH_TMR5_CYCLES;
        }

        // Configure the CCP module to set the output on the next match. Disable
        // it during the change to avoid an errant high pulse
        CCP5EN = 0;
        CCP5CONbits.CCP5MODE = 0b1000;
        CCP5EN = 1;
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

#define EVALUATE_CONSTANTS
#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const volatile uint32_t TMR5_FREQ_eval = TMR5_FREQ;
const volatile uint16_t TMR5_MOD_CLOCK_RATIO_eval = TMR5_MOD_CLOCK_RATIO;
const volatile uint16_t PULSE_GAP_LENGTH_TMR5_CYCLES_eval = PULSE_GAP_LENGTH_TMR5_CYCLES;
const volatile uint16_t ZERO_PULSE_LENGTH_TMR5_CYCLES_eval = ZERO_PULSE_LENGTH_TMR5_CYCLES;
const volatile uint16_t ONE_PULSE_LENGTH_TMR5_CYCLES_eval = ONE_PULSE_LENGTH_TMR5_CYCLES;
const volatile uint16_t MAX_PULSE_LENGTH_TMR5_CYCLES_eval = MAX_PULSE_LENGTH_TMR5_CYCLES;
const volatile uint16_t TMR5_PRELOAD_eval = TMR5_PRELOAD;
const volatile uint16_t MOD_PERIOD_TMR5_CYCLES_eval = MOD_PERIOD_TMR5_CYCLES;
const volatile uint16_t CCP5_PRELOAD_eval = CCP5_PRELOAD;
#endif
