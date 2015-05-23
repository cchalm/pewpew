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

#define PIN_LED0 RD2
#define PIN_LED1 RD3
#define PIN_LED2 RC4
#define PIN_LED3 RC5
#define PIN_LED4 RC6
#define PIN_LED5 RC7
#define PIN_LED6 RD4
#define PIN_LED7 RD5
#define PIN_LED8 RD6
#define PIN_LED9 RD7
#define PIN_MUZZLE_FLASH RB4
#define PIN_SHOT_LIGHT P1MDLBIT
#define PIN_HIT_LIGHT RB5
#define LED_ON LOW
#define LED_OFF HIGH

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

#define TMR0_PRELOAD 5 // 255 - 250

/*
 * These values must be less than half of the max value of the TMR1 register.
 *
 * Our sensor is a TSOP2240 @ 40kHz. The datasheet specifies that each pulse
 * must be at least 10 cycles, and that the gap between pulses must be at least
 * 12 cycles. TMR1 clocks at 8MHz. 10 cycles @ 40kHz == 2000 cycles @ 8MHz, and
 * 12 cycles @ 40kHz == 2400 cycles @ 8MHz.
 *
 * Pulses greater than 70 cycles must be separated by gaps of at least 4x the
 * pulse length, so make sure these values are less than 14k.
 *
 * We should aim to use the TSOP2440 in the future - it is better at filtering
 * out noise. It has the same specifications as above, except that the maximum
 * pulse length before requiring a long silence is 35 cycles instead of 70.
 *
 * The frequency of the sensor is the bottleneck of our system right now.
 * TSOP2*56 sensors receive @ 56kHz, so we may want to consider that in the
 * future as well.
 *
 * With the TSOP2*40, a zero pulse 250us, a one pulse is 500us, and the pulse
 * gap is 300us. The final gap is at least 3000 TMR1 cycles, or 375us. An
 * eight-bit transmission is thus at least 250*8+300*7+375 = 4475 (4.475ms) and
 * at most 500*8+300*7+375 = 6475 (6.475ms) long.
 */
#define PULSE_GAP_WIDTH 2400
#define ZERO_PULSE_WIDTH 2000
#define ONE_PULSE_WIDTH 4000

void configureSystem(void);
// LSB is the first LED. 1 is on, 0 is off.
void setLEDDisplay(unsigned int bits);
void delay(unsigned long d);
void delayTiny(unsigned long d);
void error(unsigned int error_code);

#endif // SYSTEM_H
