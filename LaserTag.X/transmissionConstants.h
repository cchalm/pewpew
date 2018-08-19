#ifndef TRANSMISSIONCONSTANTS_H
#define	TRANSMISSIONCONSTANTS_H

/*
 * Our sensor is a TSOP2240 @ 40kHz. The datasheet specifies that each pulse
 * must be at least 10 cycles, and that the gap between pulses must be at least
 * 12 cycles. We'll specify that a 10-cycle pulse represents a zero, and that a
 * 16-cycle pulse represents a one. The gap between pulses will be the minimum
 * 12 cycles.
 *
 * Pulses greater than 70 cycles @ 40khz must be separated by gaps of at least
 * 4x the pulse length, so our pulse lengths must be less than 70 cycles.
 *
 * The receiver can receive a maximum of 800 bursts per second, so divide that
 * by the number of bits in a shot and that's the hardware-enforced maximum fire
 * rate. For example if there are 10 bits in a shot then the maximum fire rate
 * is 80 shots per second.
 *
 * We should aim to use the TSOP2440 in the future - it is better at filtering
 * out noise. It has the same specifications as above, except that the maximum
 * pulse length before requiring a long silence is 35 cycles instead of 70, and
 * it can receive 1300 bursts per second instead of 800.
 *
 * The frequency of the sensor is the bottleneck of our system right now.
 * TSOP2*56 sensors receive @ 56kHz, so we may want to consider that in the
 * future as well.
 */

#define TSOP2240_MODULATION_FREQ 40000 // 40kHz
#define TSOP2240_PULSE_MIN_CYCLES 10
#define TSOP2240_GAP_MIN_CYCLES 12

#define MODULATION_FREQ (TSOP2240_MODULATION_FREQ)
// Modulation frequency in microseconds. Assumes period is a whole integer
#define MODULATION_PERIOD_US (1000000 / (MODULATION_FREQ))

// Pulse lengths in terms of modulation cycles
#define ZERO_PULSE_LENGTH_MOD_CYCLES (TSOP2240_PULSE_MIN_CYCLES)
#define ONE_PULSE_LENGTH_MOD_CYCLES  ((TSOP2240_PULSE_MIN_CYCLES) + 6)
#define PULSE_GAP_LENGTH_MOD_CYCLES  (TSOP2240_GAP_MIN_CYCLES)

#define MAX_PULSE_LENGTH_MOD_CYCLES (ONE_PULSE_LENGTH_MOD_CYCLES)

// Pulse lengths in microseconds
#define ZERO_PULSE_LENGTH_US ((ZERO_PULSE_LENGTH_MOD_CYCLES) * (MODULATION_PERIOD_US))
#define ONE_PULSE_LENGTH_US  ((ONE_PULSE_LENGTH_MOD_CYCLES) * (MODULATION_PERIOD_US))
#define PULSE_GAP_LENGTH_US  ((PULSE_GAP_LENGTH_MOD_CYCLES) * (MODULATION_PERIOD_US))

// Minimum gap between distinct transmissions in microseconds. 150% of pulse
// gap, truncated to nearest integer number of cycles
#define MIN_TRANSMISSION_GAP_LENGTH_MOD_CYCLES (3 * (PULSE_GAP_LENGTH_MOD_CYCLES) / 2)

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

#define NUM_PARITY_BITS 1
#define TRANSMISSION_LENGTH ((TRANSMISSION_DATA_LENGTH) + NUM_PARITY_BITS)

#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const uint32_t TSOP2240_MODULATION_FREQ_eval = TSOP2240_MODULATION_FREQ;
const uint8_t TSOP2240_PULSE_MIN_CYCLES_eval = TSOP2240_PULSE_MIN_CYCLES;
const uint8_t TSOP2240_GAP_MIN_CYCLES_eval = TSOP2240_GAP_MIN_CYCLES;
const uint32_t MODULATION_FREQ_eval = MODULATION_FREQ;
const uint8_t MODULATION_PERIOD_US_eval = MODULATION_PERIOD_US;
const uint8_t ZERO_PULSE_LENGTH_MOD_CYCLES_eval = ZERO_PULSE_LENGTH_MOD_CYCLES;
const uint8_t ONE_PULSE_LENGTH_MOD_CYCLES_eval = ONE_PULSE_LENGTH_MOD_CYCLES;
const uint8_t PULSE_GAP_LENGTH_MOD_CYCLES_eval = PULSE_GAP_LENGTH_MOD_CYCLES;
const uint16_t ZERO_PULSE_LENGTH_US_eval = ZERO_PULSE_LENGTH_US;
const uint16_t ONE_PULSE_LENGTH_US_eval = ONE_PULSE_LENGTH_US;
const uint16_t PULSE_GAP_LENGTH_US_eval = PULSE_GAP_LENGTH_US;
const uint16_t MIN_TRANSMISSION_GAP_LENGTH_US_eval = MIN_TRANSMISSION_GAP_LENGTH_US;
const uint8_t TRANSMISSION_DATA_LENGTH_eval = TRANSMISSION_DATA_LENGTH;
const uint8_t NUM_PARITY_BITS_eval = NUM_PARITY_BITS;
const uint8_t TRANSMISSION_LENGTH_eval = TRANSMISSION_LENGTH;
#endif

#endif	/* TRANSMISSIONCONSTANTS_H */

