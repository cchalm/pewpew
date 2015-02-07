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

#define PIN_LED0 PORTDbits.RD3
#define PIN_LED1 PORTDbits.RD2
#define PIN_LED2 PORTDbits.RD1
#define PIN_LED3 PORTDbits.RD0
#define PIN_LED4 PORTAbits.RA5
#define PIN_LED5 PORTAbits.RA4
#define PIN_LED6 PORTAbits.RA3
#define PIN_LED7 PORTAbits.RA2
#define PIN_LED8 PORTAbits.RA1
#define PIN_LED9 PORTAbits.RA0
#define PIN_MUZZLE_FLASH PORTBbits.RB4
#define PIN_SHOT_LIGHT P1MDLBIT
#define PIN_HIT_LIGHT PORTBbits.RB5
#define LED_ON LOW
#define LED_OFF HIGH

#define INPUT_PORT PORTD;

#define PIN_TRIGGER PORTDbits.RD7
#define TRIGGER_OFFSET 7
#define TRIGGER_MASK 0b10000000
#define TRIGGER_PRESSED HIGH
#define TRIGGER_NOT_PRESSED LOW

#define PIN_RELOAD PORTDbits.RD6
#define RELOAD_OFFSET 6
#define RELOAD_MASK 0b01000000
#define MAG_IN HIGH
#define MAG_OUT LOW

#define PIN_IR_SENSOR PORTDbits.RD5

typedef unsigned char  TMR0_t;
typedef unsigned short TMR1_t;

#define TMR0_PRELOAD 5 // 255 - 250

// These values must be less than half of the max value of the TMR1 register.
// These numbers could probably be a little lower, but these values are robust.
#define PULSE_GAP_WIDTH 2000
#define ZERO_PULSE_WIDTH 2000
#define ONE_PULSE_WIDTH 4000

void configureSystem(void);
// MSB is first LED
void setLEDDisplay(unsigned int bits);
void delay(unsigned long d);
void delayTiny(unsigned long d);
void error(unsigned int error_code);

#endif // SYSTEM_H
