#include <xc.h>
#include "IRReceiver.h"

#include "bitwiseUtils.h"
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
 *    - Automatic: halt timer until reset (TODO: confirm that the automatic resets count)
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

#define SMT1_CLOCK_FREQ 32000000
// Ratio between SMT1 clock frequency and transmission carrier wave frequency.
// Assumes they divide evenly
#define SMT1_MOD_FREQ_RATIO ((SMT1_CLOCK_FREQ) / (MODULATION_FREQ))

#define RB3_PPS 0x0B

static void configureSMT1(void)
{
    // Enable SMT1
    SMT1EN = 1;
    // Set mode to High and Low Measurement
    SMT1CON1bits.MODE = 0b0011;

    // Set clock source to HFINTOSC (32MHz). TODO verify 32MHz vs 16MHz
    SMT1CLK = 0b011;

    // Set signal source to pin B3
    SMT1SIGPPS = RB3_PPS;
    // Set pin B3 to input
    TRISBbits.TRISB3 = 1;

    // Halt on period match
    SMT1STP = 1;
    // Set period register to 2x the maximum valid pulse length
    SMT1PR = 2 * (MAX_PULSE_LENGTH_MOD_CYCLES) * (SMT1_MOD_FREQ_RATIO);
    // Disable period match interrupts
    SMT1IE = 0;

    // Enable falling edge interrupts
    SMT1PWAIE = 1;
}

void initializeReceiver(void)
{
    configureSMT1();
}

// Pulse lengths in terms of SMT1 clock cycles
#define PULSE_GAP_LENGTH_SMT1_CYCLES  ((PULSE_GAP_LENGTH_MOD_CYCLES) * (SMT1_MOD_FREQ_RATIO))
#define ZERO_PULSE_LENGTH_SMT1_CYCLES ((ZERO_PULSE_LENGTH_MOD_CYCLES) * (SMT1_MOD_FREQ_RATIO))
#define ONE_PULSE_LENGTH_SMT1_CYCLES  ((ONE_PULSE_LENGTH_MOD_CYCLES) * (SMT1_MOD_FREQ_RATIO))

// Minimum gap between distinct transmissions in terms of SMT1 clock cycles
#define MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES ((MIN_TRANSMISSION_GAP_LENGTH_MOD_CYCLES) * (SMT1_MOD_FREQ_RATIO))

// Pulses come out of the receiver longer than sent, and gaps come out shorter
// than sent. Adjust lengths using this value to get accurate measurements.
// Units are microseconds. Estimated from TSOP2240 datasheet, figure 4, then
// adjusted empirically
#define PULSE_LENGTH_BIAS_US 40
// Units are SMT1 clock cycles
#define PULSE_LENGTH_BIAS_SMT1_CYCLES ((PULSE_LENGTH_BIAS_US) * (SMT1_CLOCK_FREQ) / 1000000)

// Pulse lengths within this many microseconds of each other will be considered
// equivalent
#define PULSE_LENGTH_EPSILON_US 50
// Units are SMT1 clock cycles
#define PULSE_LENGTH_EPSILON_SMT1_CYCLES ((PULSE_LENGTH_EPSILON_US) * (SMT1_CLOCK_FREQ) / 1000000)

#define PULSE_GAP_UPPER_LENGTH_SMT1_CYCLES ((PULSE_GAP_LENGTH_SMT1_CYCLES) + (PULSE_LENGTH_EPSILON_SMT1_CYCLES))
#define PULSE_GAP_LOWER_LENGTH_SMT1_CYCLES ((PULSE_GAP_LENGTH_SMT1_CYCLES) - (PULSE_LENGTH_EPSILON_SMT1_CYCLES))
#define ZERO_PULSE_UPPER_LENGTH_SMT1_CYCLES ((ZERO_PULSE_LENGTH_SMT1_CYCLES) + (PULSE_LENGTH_EPSILON_SMT1_CYCLES))
#define ZERO_PULSE_LOWER_LENGTH_SMT1_CYCLES ((ZERO_PULSE_LENGTH_SMT1_CYCLES) - (PULSE_LENGTH_EPSILON_SMT1_CYCLES))
#define ONE_PULSE_UPPER_LENGTH_SMT1_CYCLES ((ONE_PULSE_LENGTH_SMT1_CYCLES) + (PULSE_LENGTH_EPSILON_SMT1_CYCLES))
#define ONE_PULSE_LOWER_LENGTH_SMT1_CYCLES ((ONE_PULSE_LENGTH_SMT1_CYCLES) - (PULSE_LENGTH_EPSILON_SMT1_CYCLES))

#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const uint32_t SMT1_CLOCK_FREQ_eval = SMT1_CLOCK_FREQ;
const uint8_t SMT1_MOD_FREQ_RATIO_eval = SMT1_MOD_FREQ_RATIO;
const uint16_t PULSE_GAP_LENGTH_SMT1_CYCLES_eval = PULSE_GAP_LENGTH_SMT1_CYCLES;
const uint16_t ZERO_PULSE_LENGTH_SMT1_CYCLES_eval = ZERO_PULSE_LENGTH_SMT1_CYCLES;
const uint16_t ONE_PULSE_LENGTH_SMT1_CYCLES_eval = ONE_PULSE_LENGTH_SMT1_CYCLES;
const uint16_t MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES_eval = MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES;
const uint16_t PULSE_LENGTH_BIAS_US_eval = PULSE_LENGTH_BIAS_US;
const uint16_t PULSE_LENGTH_BIAS_SMT1_CYCLES_eval = PULSE_LENGTH_BIAS_SMT1_CYCLES;
const uint16_t PULSE_LENGTH_EPSILON_US_eval = PULSE_LENGTH_EPSILON_US;
const uint16_t PULSE_LENGTH_EPSILON_SMT1_CYCLES_eval = PULSE_LENGTH_EPSILON_SMT1_CYCLES;
const uint16_t PULSE_GAP_UPPER_LENGTH_SMT1_CYCLES_eval = PULSE_GAP_UPPER_LENGTH_SMT1_CYCLES;
const uint16_t PULSE_GAP_LOWER_LENGTH_SMT1_CYCLES_eval = PULSE_GAP_LOWER_LENGTH_SMT1_CYCLES;
const uint16_t ZERO_PULSE_UPPER_LENGTH_SMT1_CYCLES_eval = ZERO_PULSE_UPPER_LENGTH_SMT1_CYCLES;
const uint16_t ZERO_PULSE_LOWER_LENGTH_SMT1_CYCLES_eval = ZERO_PULSE_LOWER_LENGTH_SMT1_CYCLES;
const uint16_t ONE_PULSE_UPPER_LENGTH_SMT1_CYCLES_eval = ONE_PULSE_UPPER_LENGTH_SMT1_CYCLES;
const uint16_t ONE_PULSE_LOWER_LENGTH_SMT1_CYCLES_eval = ONE_PULSE_LOWER_LENGTH_SMT1_CYCLES;
#endif

// It's actually 24 bits, but stdint doesn't have that
typedef uint32_t SMT1_t;

static volatile bool g_transmission_received = false;
static volatile uint16_t g_transmission_data = 0;

#define RECORD_GAPS

static volatile SMT1_t g_pulses_received[TRANSMISSION_LENGTH];
#ifdef RECORD_GAPS
static volatile SMT1_t g_gaps_received[TRANSMISSION_LENGTH];
#endif

void handleSignalReceptionInterrupt()
{
    // Shot reception logic

    // Data accumulator
    static uint16_t data = 0;
    static uint8_t bit_index = 0;

    static bool wait_for_silence = false;

    if (!(SMT1PWAIE && SMT1PWAIF))
        return;

    // TODO consider picking a SMT1 frequency such that SMT1PR fits in 8 or 16
    // bits, so that we can ignore some bits here to save space and time
    SMT1_t gap_length = SMT1CPR + PULSE_LENGTH_BIAS_SMT1_CYCLES;
    SMT1_t pulse_length = SMT1CPW - PULSE_LENGTH_BIAS_SMT1_CYCLES;

    if (gap_length >= MIN_TRANSMISSION_GAP_LENGTH_SMT1_CYCLES)
    {
        // Long silence, reset
        data = 0;
        bit_index = 0;

        wait_for_silence = false;
    }

    if (wait_for_silence)
        return;

    g_pulses_received[bit_index] = pulse_length;
#ifdef RECORD_GAPS
    g_gaps_received[bit_index] = gap_length;
#endif

    if (pulse_length > ZERO_PULSE_LOWER_LENGTH_SMT1_CYCLES
            && pulse_length < ZERO_PULSE_UPPER_LENGTH_SMT1_CYCLES)
    {
        // Received a 0
        data <<= 1;
        bit_index++;
    }
    else if (pulse_length > ONE_PULSE_LOWER_LENGTH_SMT1_CYCLES
            && pulse_length < ONE_PULSE_UPPER_LENGTH_SMT1_CYCLES)
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

    SMT1PWAIF = 0;
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

