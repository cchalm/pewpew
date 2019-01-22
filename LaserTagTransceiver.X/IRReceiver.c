#include "IRReceiver.h"
#include <xc.h>

#include "../LaserTagUtils.X/queue.h"
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
 * falling edge interrupt enabled. The electrical signal is active-low but
 * inverted by the SMT module. When this documentation refers to a "falling
 * edge", for example, that will look like a rising edge on an oscilloscope.
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
    // Don't allow SMT1TMR to increment beyond an 8-bit value, minus one. 0xFF is reserved to mean "long gap"
    SMT1PR = 0xFE;
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

// We have limited the period of the SMT timer such that its value always fits
// in 8 bits
typedef uint8_t SMT1_t;

// Double the maximum transmission length in bits, plus one
#define INCOMING_PULSE_WIDTHS_STORAGE_SIZE 129
static uint8_t g_incoming_pulse_widths_storage[INCOMING_PULSE_WIDTHS_STORAGE_SIZE];
// Active and inactive pulse widths
static queue_t g_incoming_pulse_widths;

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

    g_incoming_pulse_widths = queue_create(g_incoming_pulse_widths_storage, INCOMING_PULSE_WIDTHS_STORAGE_SIZE);
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

static void SMT1InterruptHandler()
{
    if (!(SMT1PWAIF && SMT1PWAIE))
        return;

    SMT1PWAIF = 0;

    // We only need to grab the low (L) 8 bits because we've limited the max
    // timer value
    uint8_t gap_length = SMT1CPRL;
    uint8_t pulse_length = SMT1CPWL;

    queue_push(&g_incoming_pulse_widths, gap_length);
    queue_push(&g_incoming_pulse_widths, pulse_length);

    // Imperfect check for interrupt overlap
    if (SMT1PWAIF)
    {
        fatal(ERROR_UNHANDLED_PULSE_MEASUREMENT);
    }
}

static void TMR4InterruptHandler()
{
    if (!(TMR4IE && TMR4IF))
        return;

    TMR4IF = 0;

    // Push the reserved value 0xFF onto the queue to indicate "long gap"
    queue_push(&g_incoming_pulse_widths, 0xFF);

    // Turn the timer back on, as the period match that triggered this interrupt
    // also turned off the timer. It will resume counting on the next
    // active --> inactive transition on the transmission line
    TMR4ON = 1;
}

void irReceiver_interruptHandler()
{
    TMR4InterruptHandler();
    SMT1InterruptHandler();
}

static bool tryDecodePulseLength(uint8_t pulse_length, uint8_t* bit_out)
{
    // TODO consider loading these macros into constants to avoid evaluating them multiple times
    if (pulse_length > ZERO_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES
        && pulse_length < ZERO_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES)
    {
        *bit_out = 0;
        return true;
    }
    else if (pulse_length > ONE_PULSE_LENGTH_LOWER_BOUND_SMT1_CYCLES
             && pulse_length < ONE_PULSE_LENGTH_UPPER_BOUND_SMT1_CYCLES)
    {
        *bit_out = 1;
        return true;
    }
    else
    {
        // Invalid pulse width
        return false;
    }
}

bool irReceiver_tryGetTransmission(uint16_t* data_out, uint8_t* data_length_out)
{
    // Accumulate using a static variable, in case a client calls this before we've received all of a transmission
    static uint16_t data_accumulator = 0;
    static uint8_t data_length = 0;
    // A boolean indicating that the transmission currently being analyzed should be discarded
    static bool invalid_transmission = false;

    while (queue_size(&g_incoming_pulse_widths) != 0)
    {
        uint8_t gap_length;
        queue_pop(&g_incoming_pulse_widths, &gap_length);

        // 0xFF is a reserved value meaning "end of transmission"
        if (gap_length == 0xFF)
        {
            if (!invalid_transmission)
            {
                *data_out = data_accumulator;
                *data_length_out = data_length;
                data_accumulator = 0;
                data_length = 0;
                return true;
            }
            else
            {
                // We've reached the end of the invalid transmission, so try the next one
                invalid_transmission = false;
                data_accumulator = 0;
                data_length = 0;
                continue;
            }
        }

        uint8_t pulse_length;
        bool empty = !queue_pop(&g_incoming_pulse_widths, &pulse_length);

        // There must be another element in the queue, because gap and pulse lengths are added in pairs except for the
        // terminating gap, which we checked for above
        if (empty)
            fatal(ERROR_INCOMING_PULSE_LENGTHS_QUEUE_EMPTY);

        // Don't bother analyzing the pulse length if the transmission is invalid
        if (invalid_transmission)
            continue;

        uint8_t bit;
        bool is_valid_pulse_length = tryDecodePulseLength(pulse_length, &bit);
        if (is_valid_pulse_length)
        {
            data_accumulator = (data_accumulator << 1) | bit;
            data_length++;
        }
        else
        {
            // Invalid pulse width. Something has gone wrong, so we're going to ignore this transmission
            invalid_transmission = true;
        }
    }

    return false;
}

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
