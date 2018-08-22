#ifndef IRRECEIVERSTATS_H
#define	IRRECEIVERSTATS_H

/*
 * Our sensor is a TSOP13556 @ 56kHz. The datasheet specifies that each pulse
 * must be at least 6 cycles, and that the gap between pulses must be at least
 * 10 cycles.
 *
 * Pulses greater than 24 cycles @ 56khz must be separated by gaps of at least
 * 25ms, so our pulse lengths must be less than 24 cycles.
 *
 * The receiver can receive a maximum of 1800 bursts per second, so divide that
 * by the number of bits in a shot and that's the hardware-enforced maximum shot
 * reception rate. For example if there are 10 bits in a shot then the maximum
 * fire rate is 180 shots per second.
 */

#define RECEIVER_MODULATION_FREQ 56000 // 56kHz
#define RECEIVER_PULSE_MIN_CYCLES 6
#define RECEIVER_GAP_MIN_CYCLES 10

// If the receiver receives an optical signal pulse consisting of x modulation
// cycles, it will output an electrical signal up to
// RECEIVER_PULSE_LENGTH_BIAS_UPPER_BOUND_MOD_CYCLES cycles longer than the
// optical signal, and down to RECEIVER_PULSE_LENGTH_BIAS_LOWER_BOUND_MOD_CYCLES
// cycles shorter than the optical signal. Both values are rounded up, if
// fractional
#define RECEIVER_PULSE_LENGTH_BIAS_UPPER_BOUND_MOD_CYCLES 4
#define RECEIVER_PULSE_LENGTH_BIAS_LOWER_BOUND_MOD_CYCLES 3

#define EVALUATE_CONSTANTS
#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const uint32_t RECEIVER_MODULATION_FREQ_eval = RECEIVER_MODULATION_FREQ;
const uint8_t RECEIVER_PULSE_MIN_CYCLES_eval = RECEIVER_PULSE_MIN_CYCLES;
const uint8_t RECEIVER_GAP_MIN_CYCLES_eval = RECEIVER_GAP_MIN_CYCLES;
const uint8_t RECEIVER_PULSE_LENGTH_BIAS_UPPER_BOUND_MOD_CYCLES_eval = RECEIVER_PULSE_LENGTH_BIAS_UPPER_BOUND_MOD_CYCLES;
const uint8_t RECEIVER_PULSE_LENGTH_BIAS_LOWER_BOUND_MOD_CYCLES_eval = RECEIVER_PULSE_LENGTH_BIAS_LOWER_BOUND_MOD_CYCLES;
#endif

#endif	/* IRRECEIVERSTATS_H */

