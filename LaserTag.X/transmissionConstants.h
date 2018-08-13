/* 
 * File:   transmissionConstants.h
 * Author: chris
 *
 * Created on August 11, 2018, 11:49 PM
 */

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

// Pulse lengths in microseconds
#define ZERO_PULSE_LENGTH_US ((ZERO_PULSE_LENGTH_MOD_CYCLES) * (MODULATION_PERIOD_US))
#define ONE_PULSE_LENGTH_US  ((ONE_PULSE_LENGTH_MOD_CYCLES) * (MODULATION_PERIOD_US))
#define PULSE_GAP_LENGTH_US  ((PULSE_GAP_LENGTH_MOD_CYCLES) * (MODULATION_PERIOD_US))

// Minimum gap between distinct transmissions in microseconds. 150% of pulse gap
#define MIN_TRANSMISSION_GAP_LENGTH_US ((PULSE_GAP_LENGTH_US) + ((PULSE_GAP_LENGTH_US) / 2))

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

// TMR1 clocks at (Fosc / 4) and is prescaled 8x. 32 / 4 / 8 = 1MHz. At 1MHz, a
// cycle is 1us.
#define PULSE_GAP_TMR1_WIDTH  300 //          300us * (1 cycle / 1us) = 300 cycles
#define ZERO_PULSE_TMR1_WIDTH 230 // (250us - 20us) * (1 cycle / 1us) = 230 cycles
#define ONE_PULSE_TMR1_WIDTH  380 // (400us - 20us) * (1 cycle / 1us) = 380 cycles

// TMR2 clocks at (Fosc / 4) and is prescaled 16x. 32 / 4 / 16 = 0.5MHz. At
// 0.5MHz, a cycle is 2us.
#define PULSE_GAP_TMR2_WIDTH  150 //          300us * (1 cycle / 2us) = 150 cycles
#define ZERO_PULSE_TMR2_WIDTH 115 // (250us - 20us) * (1 cycle / 2us) = 115 cycles
#define ONE_PULSE_TMR2_WIDTH  190 // (400us - 20us) * (1 cycle / 2us) = 190 cycles

#define TRANSMISSION_DATA_LENGTH 10

#endif	/* TRANSMISSIONCONSTANTS_H */

