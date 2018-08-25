#ifndef TRANSMISSIONCONSTANTS_H
#define	TRANSMISSIONCONSTANTS_H

#include "IRReceiverStats.h"

// The minimum difference between two pulse lengths to guarantee that they can
// be unambiguously distinguished by the receiver
#define PULSE_LENGTH_MIN_DIFF_MOD_CYCLES \
        ((((RECEIVER_PULSE_LENGTH_BIAS_LOWER_BOUND_MOD_CYCLES_x10) \
       + (RECEIVER_PULSE_LENGTH_BIAS_UPPER_BOUND_MOD_CYCLES_x10)) / 10) + 1)

// Pulse lengths in terms of modulation cycles
#define ZERO_PULSE_LENGTH_MOD_CYCLES (RECEIVER_PULSE_MIN_CYCLES)
#define ONE_PULSE_LENGTH_MOD_CYCLES \
        ((ZERO_PULSE_LENGTH_MOD_CYCLES) + (PULSE_LENGTH_MIN_DIFF_MOD_CYCLES))
#define PULSE_GAP_LENGTH_MOD_CYCLES  (RECEIVER_GAP_MIN_CYCLES)

// Minimum gap between distinct transmissions in modulation cycles. 150% of
// pulse gap, truncated to nearest integer number of cycles
#define MIN_TRANSMISSION_GAP_LENGTH_MOD_CYCLES (3 * (PULSE_GAP_LENGTH_MOD_CYCLES) / 2)

#define MODULATION_FREQ (RECEIVER_MODULATION_FREQ)

/*
 * A zero pulse is 10 modulation cycles
 * A one pulse is 16 modulation cycles
 * A pulse gap is 12 modulation cycles
 * A transmission gap is (150% of 12 = 18) modulation cycles
 *
 * A 10-bit transmission contains:
 * - 9 gaps
 * - Minimally, 10 zero pulses. Maximally, 10 one pulses
 * - Transmission gap
 *
 * Minimum time for 10-bit transmission:
 *     9*12+10*10+18 = 226 modulation cycles
 *     222 modulation cycles @ 25us per cycle = 5.65ms
 * Maximum time for a 10-bit transmission:
 *     9*12+10*16+18 = 286 modulation cycles
 *     282 modulation cycles @ 25us per cycle = 7.15ms
 */

#define TRANSMISSION_DATA_LENGTH 10

#define NUM_PARITY_BITS 3
#define TRANSMISSION_LENGTH ((TRANSMISSION_DATA_LENGTH) + NUM_PARITY_BITS)

#define EVALUATE_CONSTANTS
#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const volatile uint8_t PULSE_LENGTH_MIN_DIFF_MOD_CYCLES_eval = PULSE_LENGTH_MIN_DIFF_MOD_CYCLES;
const volatile uint8_t ZERO_PULSE_LENGTH_MOD_CYCLES_eval = ZERO_PULSE_LENGTH_MOD_CYCLES;
const volatile uint8_t ONE_PULSE_LENGTH_MOD_CYCLES_eval = ONE_PULSE_LENGTH_MOD_CYCLES;
const volatile uint8_t PULSE_GAP_LENGTH_MOD_CYCLES_eval = PULSE_GAP_LENGTH_MOD_CYCLES;
const volatile uint8_t MIN_TRANSMISSION_GAP_LENGTH_MOD_CYCLES_eval = MIN_TRANSMISSION_GAP_LENGTH_MOD_CYCLES;
const volatile uint32_t MODULATION_FREQ_eval = MODULATION_FREQ;
const volatile uint8_t TRANSMISSION_DATA_LENGTH_eval = TRANSMISSION_DATA_LENGTH;
const volatile uint8_t NUM_PARITY_BITS_eval = NUM_PARITY_BITS;
const volatile uint8_t TRANSMISSION_LENGTH_eval = TRANSMISSION_LENGTH;
#endif

#endif	/* TRANSMISSIONCONSTANTS_H */

