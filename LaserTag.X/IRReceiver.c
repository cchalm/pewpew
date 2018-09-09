#include <xc.h>
#include "IRReceiver.h"

#include "bitwiseUtils.h"
#include "error.h"
#include "IRReceiverStats.h"
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
 */

#define SMT1_CLOCK_FREQ 500000
// Ratio between SMT1 clock frequency and transmission carrier wave frequency.
// Assumes they divide evenly
#define SMT1_MOD_FREQ_RATIO ((SMT1_CLOCK_FREQ) / (MODULATION_FREQ))

#define RB3_PPS 0x0B

static void configureSMT1(void)
{
    // Don't start incrementing the timer yet
    SMT1GO = 0;
    // Enable SMT1
    SMT1EN = 1;
    // Set mode to High and Low Measurement
    SMT1CON1bits.MODE = 0b0011;
    // Enable repeated acquisitions, i.e. the timer doesn't stop when a data
    // acquisition is made
    SMT1REPEAT = 1;

    // Set clock source to MFINTOSC
    SMT1CLK = 0b100;
    // Set prescaler to 1:1
    SMT1CON0bits.SMT1PS = 0b00;

    // Signal input source: pin selected by SMT1SIGPPS
    SMT1SIG = 0b00000;

    // Set signal source to pin B3
    SMT1SIGPPS = RB3_PPS;
    // Set pin B3 to input
    TRISB3 = 1;
    // Set signal source polarity to active-low
    SMT1SPOL = 1;

    // Halt on period match
    SMT1STP = 1;
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

void receiverStaticAsserts(void);

void initializeReceiver(void)
{
    receiverStaticAsserts();

    configureSMT1();
}

// Minimum gap between distinct transmissions in terms of SMT1 clock cycles
#define MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES ((MIN_TRANSMISSION_GAP_LENGTH_MOD_CYCLES) * (SMT1_MOD_FREQ_RATIO))

// Upper and lower bounds of pulse lengths, in terms of modulation cycles.
// To increase resolution, these hold values 10x the real ones
#define ZERO_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10 ((ZERO_PULSE_LENGTH_MOD_CYCLES) * 10 + (RECEIVER_PULSE_LENGTH_BIAS_UPPER_BOUND_MOD_CYCLES_x10))
#define ZERO_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10 ((ZERO_PULSE_LENGTH_MOD_CYCLES) * 10 - (RECEIVER_PULSE_LENGTH_BIAS_LOWER_BOUND_MOD_CYCLES_x10))
#define ONE_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10 ((ONE_PULSE_LENGTH_MOD_CYCLES) * 10 + (RECEIVER_PULSE_LENGTH_BIAS_UPPER_BOUND_MOD_CYCLES_x10))
#define ONE_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10 ((ONE_PULSE_LENGTH_MOD_CYCLES) * 10 - (RECEIVER_PULSE_LENGTH_BIAS_LOWER_BOUND_MOD_CYCLES_x10))

#define ZERO_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES ((ZERO_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10) * (SMT1_MOD_FREQ_RATIO) / 10)
#define ZERO_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES ((ZERO_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10) * (SMT1_MOD_FREQ_RATIO) / 10)
#define ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES ((ONE_PULSE_LENGTH_UPPER_BOUND_MOD_CYCLES_x10) * (SMT1_MOD_FREQ_RATIO) / 10)
#define ONE_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES ((ONE_PULSE_LENGTH_LOWER_BOUND_MOD_CYCLES_x10) * (SMT1_MOD_FREQ_RATIO) / 10)

// We have limited the period of the SMT timer such that its value always fits
// in 8 bits
typedef uint8_t SMT1_t;

static bool g_transmission_received = false;
static uint16_t g_transmission_data = 0;

static volatile bool g_pulse_received = false;
// Set by the interrupt handler, cleared by mainline code when read
static volatile SMT1_t g_pulse_length;
static volatile SMT1_t g_gap_length;

#define RECORD_PULSE_LENGTHS
#ifdef RECORD_PULSE_LENGTHS
static SMT1_t g_pulse_lengths[TRANSMISSION_LENGTH];
static SMT1_t g_gap_lengths[TRANSMISSION_LENGTH];
#endif

void receiverInterruptHandler()
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

void receiverEventHandler(void)
{
    if (g_pulse_received)
    {
        // Data accumulator
        static uint16_t data = 0;
        static uint8_t bit_index = 0;
        static bool wait_for_silence = false;

        // Copy these to reduce the chance of them getting written to while
        // we're using them
        SMT1_t pulse_length = g_pulse_length;
        SMT1_t gap_length = g_gap_length;

        // Since we copied the values, let another pulse come in
        g_pulse_received = false;

        if (gap_length >= MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES)
        {
            // Long silence, reset
            data = 0;
            bit_index = 0;

            wait_for_silence = false;
        }

        if (!wait_for_silence)
        {
#ifdef RECORD_PULSE_LENGTHS
            g_pulse_lengths[bit_index] = pulse_length;
            g_gap_lengths[bit_index] = gap_length;
#endif

            if (pulse_length > ZERO_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES
                    && pulse_length < ZERO_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES)
            {
                // Received a 0
                data <<= 1;
                bit_index++;
            }
            else if (pulse_length > ONE_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES
                    && pulse_length < ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES)
            {
                // Received a 1
                data = (data << 1) | 1;
                bit_index++;
            }
            else
            {
                // Invalid pulse width. Something has gone wrong, so we're going to stop
                // reading in data and wait until we see a long period of silence

                wait_for_silence = true;
            }

            if (bit_index == TRANSMISSION_LENGTH)
            {
                g_transmission_data = data;
                g_transmission_received = true;

                // Wait for a reset gap before attempting to receive another
                // transmission
                wait_for_silence = true;
            }
        }
    }
}

bool tryGetTransmissionData(uint16_t* data_out)
{
    if (!g_transmission_received)
        return false;

    // Copy the data to reduce the chance of it getting overwritten while we're
    // working on it
    uint16_t data = g_transmission_data;

    // Reset transmission received flag until the next transmission
    g_transmission_received = false;

    uint16_t parity_bits_mask = (1 << NUM_PARITY_BITS) - 1;
    // Sum all except the last n bits, n being the number of
    // parity bits
    uint8_t bit_sum = sumBits(data & ~parity_bits_mask);
    bool parity_matches = ((bit_sum & parity_bits_mask) == (data & parity_bits_mask));

    if (parity_matches)
    {
        *data_out = data >> NUM_PARITY_BITS;
    }

    return parity_matches;
}

void receiverStaticAsserts(void)
{
    // Pulse lengths in terms of SMT1 cycles must fit in 8 bits with room to
    // spare
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
