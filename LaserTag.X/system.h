/* 
 * File:   system.h
 * Author: Chris
 *
 * Created on January 6, 2015, 9:31 PM
 */

#ifndef SYSTEM_H
#define SYSTEM_H

#define OUTPUT 0
#define INPUT 1
#define HIGH 1
#define LOW 0

#define PIN_MUZZLE_FLASH RB4
#define PIN_SHOT_LIGHT P1MDLBIT
#define PIN_HIT_LIGHT RB5

#define INPUT_PORT PORTD;

#define PIN_TRIGGER RD0
#define TRIGGER_OFFSET 0
#define TRIGGER_MASK 0b1
#define TRIGGER_PRESSED HIGH
#define TRIGGER_NOT_PRESSED LOW

#define PIN_RELOAD RD1
#define RELOAD_OFFSET 1
#define RELOAD_MASK 0b10
#define MAG_IN HIGH
#define MAG_OUT LOW

typedef unsigned char  TMR0_t;
typedef unsigned short TMR1_t;
typedef unsigned char  TMR2_t;

#define TMR0_PRELOAD 5 // 255 - 250

/*
 * These values must be less than half of the max value of the TMR1 register.
 *
 * Our sensor is a TSOP2240 @ 40kHz. The datasheet specifies that each pulse
 * must be at least 10 cycles, and that the gap between pulses must be at least
 * 12 cycles. We'll specify that a 10-cycle pulse represents a zero, and that a
 * 16-cycle pulse represents a one. The gap between pulses will be the minimum
 * 12 cycles.
 * 
 * At 40kHz each cycle is 25us. 10 cycles therefore takes 25us, so we should
 * output a 250us pulse right? Not quite. Each cycle is 25us, so we can only
 * output in multiples of that cycle. If we output a 250us pulse, the 11th cycle
 * will begin right as we try to stop the pulse. This will result in an 11-cycle
 * pulse. Instead, we should output for slightly longer than 9 pulses would
 * take. The 10th cycle will start before we turn off the pulse. So let's output
 * pulses 20us shorter than the full time. In that case, a "zero" pulse is 230us
 * and a "one" pulse is (16 * 25) - 20 = 380us. 
 *
 * Remember: pulses greater than 70 cycles @ 40khz must be separated by gaps of
 * at least 4x the pulse length, so make sure our pulse lengths for zeroes and
 * ones do not exceed 1750 cycles @ 1MHz. Also, the receiver can receive a
 * maximum of 800 bursts per second, so divide that by the number of bits in a
 * shot and that's the hardware-enforced maximum fire rate. For example if there
 * are 10 bits in a shot then the maximum fire rate is 80 shots per second.
 *
 * We should aim to use the TSOP2440 in the future - it is better at filtering
 * out noise. It has the same specifications as above, except that the maximum
 * pulse length before requiring a long silence is 35 cycles instead of 70, and
 * it can receive 1300 bursts per second instead of 800.
 *
 * With the TSOP2*40, a zero pulse is 250us, a one pulse is 500us, and the pulse
 * gap is 300us. The final gap is at least 375 TMR1 cycles, or 375us. An
 * eight-bit transmission is thus at least 250*8+300*7+375 = 4475 (4.475ms) and
 * at most 500*8+300*7+375 = 6475 (6.475ms) long.
 *
 * The frequency of the sensor is the bottleneck of our system right now.
 * TSOP2*56 sensors receive @ 56kHz, so we may want to consider that in the
 * future as well.
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

void configureSystem(void);
void delay(unsigned long d);
void delayTiny(unsigned long d);
void error(unsigned int error_code);

#endif // SYSTEM_H
