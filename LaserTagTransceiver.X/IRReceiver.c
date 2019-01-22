#include "IRReceiver.h"
#include <xc.h>

#include "IRReceiverStats.h"
#include "error.h"
#include "pins.h"
#include "transmissionConstants.h"

#include <stdbool.h>

/*
 * HOW IT WORKS
 *
 * We're using a Signal Measurement Timer (SMT) module for timing incoming
 * transmissions. We are using it in High and Low Measure Mode. We only have the
 * falling edge interrupt enabled.
 *
 * On a rising edge:
 *    - Automatic: SMTxTMR is latched into SMTxCPR
 *    - Automatic: SMTxTMR is reset (but not stopped)
 *
 * On a falling edge:
 *    - Automatic: SMTxTMR is latched into SMTxCPW
 *    - Automatic: SMTxTMR is reset (but not stopped)
 *    - Automatic: Sets SMTxPWAIF (interrupt)
 *    - Interrupt handler: records SMTxCPW as the last HIGH pulse width and
 *          SMTxCPR as the last LOW pulse width
 *
 * On SMTxTMR period match:
 *    - Automatic: halt timer until reset
 *    - No interrupt
 *
 *
 * Note some key behaviors:
 *    - The timer never overflows because it automatically stops incrementing at
 *          some high value. We only care about short pulses; we throw out all
 *          long pulses.
 *    - The interrupt handler only runs on falling edge events. This makes the
 *          interrupt handler simpler and reduces the time wasted spinning the
 *          interrupt code up and down.
 *
 *
 * We are using an additional, regular timer to detect gaps longer than some
 * predetermined length before they finish. A long gap indicates the end of a
 * transmission.
 *
 * The timer operates in level reset, edge-triggered hardware limit one-shot
 * mode. That's a mouthful. Here's what happens:
 * - When the transmission line is ACTIVE, the timer is held at 0.
 * - When the transmission line goes INACTIVE, the timer starts counting after a
 *   delay.
 * - When a period match occurs, the timer is turned OFF. It will not count
 *   again until an ACTIVE --> INACTIVE edge occurs after the timer is turned
 *   back on in software.
 *
 * If a period match occurs, we know there was silence on the transmission line
 * for at least the duration determined by the period register. We handle the
 * period match interrupt by setting a global boolean indicating that a
 * transmission is available and turning the timer back on.
 *
 * If the timer is turned off by a period match and an active pulse begins and
 * ends before the timer is turned on again, then the following gap will not be
 * measured by this timer. This is unlikely, since we're turning the timer back
 * on in the interrupt handler for it being turned off.
 */

#define SMT1_CLOCK_FREQ 500000
// Ratio between SMT1 clock frequency and transmission carrier wave frequency.
// Assumes they divide evenly
#define SMT1_MOD_FREQ_RATIO ((SMT1_CLOCK_FREQ) / (MODULATION_FREQ))

static void configureSMT1(void)
{
    // Don't start incrementing the timer yet
    SMT1GO = 0;
    // Enable SMT1
    SMT1CON0bits.EN = 1;
    // Set mode to High and Low Measurement
    SMT1CON1bits.MODE = 0b0011;
    // Enable repeated acquisitions, i.e. the timer doesn't stop when a data
    // acquisition is made
    SMT1REPEAT = 1;

    // Set clock source to MFINTOSC
    SMT1CLK = 0b101;
    // Set prescaler to 1:1
    SMT1CON0bits.SMT1PS = 0b00;

    // Signal input source: pin selected by SMT1SIGPPS
    SMT1SIG = 0b00000;

    // Set signal source to IR receiver pin
    SMT1SIGPPS = PPS_IN_VAL_IR_RECEIVER;
    // Set IR receiver pin to input
    TRIS_IR_RECEIVER = 1;
    // Set signal source polarity to active-low
    SMT1CON0bits.SPOL = 1;

    // Halt on period match
    SMT1CON0bits.STP = 1;
    // Don't allow SMT1TMR to increment beyond an 8-bit value
    SMT1PR = 255;
    // Disable period match interrupts
    SMT1IE = 0;

    // Enable falling edge (end of pulse) interrupts
    SMT1PWAIE = 1;

    // Clear the timer register
    SMT1TMR = 0;
    // Start SMT
    SMT1GO = 1;
}

#define TMR4_CLOCK_FREQ 500000
// Ratio between TMR4 clock frequency and transmission carrier wave frequency.
// Assumes they divide evenly
#define TMR4_MOD_FREQ_RATIO ((TMR4_CLOCK_FREQ) / (MODULATION_FREQ))

// Minimum gap between distinct transmissions in terms of TMR4 clock cycles
#define MIN_TRANSMISSION_GAP_LENGTH_TMR4_CYCLES ((MIN_TRANSMISSION_GAP_LENGTH_MOD_CYCLES) * (TMR4_MOD_FREQ_RATIO))

static void configureTMR4(void)
{
    // Set Timer4 clock source to Fosc/4 (8MHz)
    T4CLKCONbits.CS = 0b0000;
    // Set Timer4 clock prescaler to 1:16 (500kHz)
    T4CONbits.CKPS = 0b100;

    // Set Timer4 mode to rising edge start + low level reset (transmission line
    // is active-low)
    T4HLTbits.MODE = 0b01110;

    // Set external reset signal to pin selected by T4INPPS
    T4RSTbits.RSEL = 0b0000;

    // Set external reset signal pin to the IR receiver pin
    T4PPS = PPS_IN_VAL_IR_RECEIVER;

    T4PR = MIN_TRANSMISSION_GAP_LENGTH_TMR4_CYCLES;

    // Enable period match interrupts
    TMR4IE = 1;

    // Start timer
    TMR4ON = 1;
}

void receiverStaticAsserts(void);

static void disableReceptionModules(void)
{
    TMR4ON = 0;
    SMT1CON0bits.EN = 0;
}

void irReceiver_initialize(void)
{
    receiverStaticAsserts();

    configureSMT1();
    configureTMR4();
}

void irReceiver_shutdown(void)
{
    disableReceptionModules();
}

// Minimum gap between distinct transmissions in terms of SMT1 clock cycles
#define MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES ((MIN_TRANSMISSION_GAP_LENGTH_MOD_CYCLES) * (SMT1_MOD_FREQ_RATIO))

// Upper and lower bounds of pulse lengths, in terms of modulation cycles.
// To increase resolution, these hold values 10x the real ones
#define ZERO_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10 \
    ((ZERO_PULSE_LENGTH_MOD_CYCLES)*10 + (RECEIVER_PULSE_LENGTH_BIAS_UPPER_BOUND_MOD_CYCLES_x10))
#define ZERO_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10 \
    ((ZERO_PULSE_LENGTH_MOD_CYCLES)*10 - (RECEIVER_PULSE_LENGTH_BIAS_LOWER_BOUND_MOD_CYCLES_x10))
#define ONE_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10 \
    ((ONE_PULSE_LENGTH_MOD_CYCLES)*10 + (RECEIVER_PULSE_LENGTH_BIAS_UPPER_BOUND_MOD_CYCLES_x10))
#define ONE_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10 \
    ((ONE_PULSE_LENGTH_MOD_CYCLES)*10 - (RECEIVER_PULSE_LENGTH_BIAS_LOWER_BOUND_MOD_CYCLES_x10))

#define ZERO_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES \
    ((ZERO_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10) * (SMT1_MOD_FREQ_RATIO) / 10)
#define ZERO_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES \
    ((ZERO_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10) * (SMT1_MOD_FREQ_RATIO) / 10)
#define ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES \
    ((ONE_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10) * (SMT1_MOD_FREQ_RATIO) / 10)
#define ONE_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES \
    ((ONE_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10) * (SMT1_MOD_FREQ_RATIO) / 10)

// We have limited the period of the SMT timer such that its value always fits
// in 8 bits
typedef uint8_t SMT1_t;

// Index of the bit we're waiting to receive
static uint8_t g_bit_index = 0;
// Stores transmission data as it comes in
static uint16_t g_transmission_data_accumulator = 0;
static bool g_wait_for_silence = false;

static bool g_transmission_received = false;
static uint8_t g_transmission_length = 0;
static uint16_t g_transmission_data = 0;

static volatile bool g_pulse_received = false;
// Set by the interrupt handler, cleared by mainline code when read
static volatile SMT1_t g_pulse_length;
static volatile SMT1_t g_gap_length;

// Record the pulse lengths of the last received transmission
#ifdef DEBUG_RECEIVED_PULSE_LENGTHS
static SMT1_t g_pulse_lengths[MAX_TRANSMISSION_LENGTH];
static SMT1_t g_gap_lengths[MAX_TRANSMISSION_LENGTH];
#endif

static void SMT1InterruptHandler()
{
    if (!(SMT1PWAIE && SMT1PWAIF))
        return;

    if (g_pulse_received)
        fatal(ERROR_UNHANDLED_PULSE_MEASUREMENT);

    // We only need to grab the low (L) 8 bits because we've limited the max
    // timer value
    g_gap_length = SMT1CPRL;
    g_pulse_length = SMT1CPWL;
    g_pulse_received = true;

    SMT1PWAIF = 0;
}

static void TMR4InterruptHandler()
{
    if (!(TMR4IE && TMR4IF))
        return;

    if (!g_wait_for_silence)
    {
        g_transmission_data = g_transmission_data_accumulator;
        g_transmission_length = g_bit_index;
        g_transmission_received = true;
    }

    // Turn the timer back on, as the period match that triggered this interrupt
    // also turned off the timer. It will resume counting on the next
    // active --> inactive transition on the transmission line
    TMR4ON = 1;

    TMR4IF = 0;
}

void irReceiver_interruptHandler()
{
    TMR4InterruptHandler();
    SMT1InterruptHandler();
}

void irReceiver_eventHandler(void)
{
    if (g_pulse_received)
    {
        // Copy these to reduce the chance of them getting written to while
        // we're using them
        SMT1_t pulse_length = g_pulse_length;
        SMT1_t gap_length = g_gap_length;

        // Since we copied the values, let another pulse come in
        g_pulse_received = false;

        if (gap_length >= MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES)
        {
            // Long silence, reset
            g_transmission_data_accumulator = 0;
            g_bit_index = 0;

            g_wait_for_silence = false;
        }

        if (!g_wait_for_silence)
        {
#ifdef DEBUG_RECEIVED_PULSE_LENGTHS
            g_pulse_lengths[g_bit_index] = pulse_length;
            g_gap_lengths[g_bit_index] = gap_length;
#endif

            if (pulse_length > ZERO_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES
                && pulse_length < ZERO_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES)
            {
                // Received a 0
                g_transmission_data_accumulator <<= 1;
                g_bit_index++;
            }
            else if (pulse_length > ONE_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES
                     && pulse_length < ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES)
            {
                // Received a 1
                g_transmission_data_accumulator = (g_transmission_data_accumulator << 1) | 1;
                g_bit_index++;
            }
            else
            {
                // Invalid pulse width. Something has gone wrong, so we're going to stop reading in data and wait until
                // we see a long period of silence

                g_wait_for_silence = true;
            }
        }
    }
}

bool irReceiver_tryGetTransmission(uint16_t* data_out, uint8_t* data_length_out)
{
    if (!g_transmission_received)
        return false;

    *data_out = g_transmission_data;
    *data_length_out = g_transmission_length;
    // Reset transmission received flag until the next transmission
    g_transmission_received = false;

    return true;
}

#ifdef DEBUG_RECEIVED_PULSE_LENGTHS
const size_t PULSE_LENGTH_ARRAY_SIZE = MAX_TRANSMISSION_LENGTH * sizeof(SMT1_t);

SMT1_t* getPulseLengthArray()
{
    return g_pulse_lengths;
}
SMT1_t* getGapLengthArray()
{
    return g_gap_lengths;
}
#endif

void receiverStaticAsserts(void)
{
    // Pulse lengths in terms of SMT1 cycles must fit in 8 bits with room to spare
    if ((ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES) > 200)
        fatal(ERROR_PULSE_MEASUREMENT_DOESNT_FIT_SMT1);

    // The transmission gap length, in terms of SMT1 cycles, must fit in 8 bits
    if ((MIN_TRANSMISSION_GAP_LENGTH_MOD_CYCLES) * (SMT1_MOD_FREQ_RATIO) > 255)
        fatal(ERROR_GAP_MEASUREMENT_DOESNT_FIT_SMT1);

    // The 0/1 pulse length ranges must not overlap
    if (((ZERO_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES) > (ONE_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES)
         && (ZERO_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES) < (ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES))
        || ((ZERO_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES) > (ONE_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES)
            && (ZERO_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES) < (ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES)))
        fatal(ERROR_OVERLAPPING_PULSE_LENGTH_RANGES);

    // The transmission gap length, in terms of TMR4 cycles, must fit in T4PR
    if (MIN_TRANSMISSION_GAP_LENGTH_TMR4_CYCLES > 255)
        fatal(ERROR_TRANSMISSION_GAP_LENGTH_DOESNT_FIT_TMR4);
}

#define EVALUATE_CONSTANTS
#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const volatile uint32_t SMT1_CLOCK_FREQ_eval = SMT1_CLOCK_FREQ;
const volatile uint16_t SMT1_MOD_FREQ_RATIO_eval = SMT1_MOD_FREQ_RATIO;
const volatile uint16_t MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES_eval = MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES;
const volatile uint8_t ZERO_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10_eval = ZERO_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10;
const volatile uint8_t ZERO_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10_eval = ZERO_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10;
const volatile uint8_t ONE_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10_eval = ONE_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10;
const volatile uint8_t ONE_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10_eval = ONE_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10;
const volatile uint16_t ZERO_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES_eval = ZERO_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES;
const volatile uint16_t ZERO_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES_eval = ZERO_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES;
const volatile uint16_t ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES_eval = ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES;
const volatile uint16_t ONE_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES_eval = ONE_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES;
#endif
