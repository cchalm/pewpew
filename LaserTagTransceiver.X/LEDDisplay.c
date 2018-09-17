#include "LEDDisplay.h"

#include <xc.h>

// B4 - B7
#define LED_MASK_PORTB 0b11110000
// C0 - C5
#define LED_MASK_PORTC 0b00111111

#define PIN_LED0 RC0
#define PIN_LED1 RC1
#define PIN_LED2 RC2
#define PIN_LED3 RC3
#define PIN_LED4 RC4
#define PIN_LED5 RC5
#define PIN_LED6 RB4
#define PIN_LED7 RB5
#define PIN_LED8 RB6
#define PIN_LED9 RB7

void initializeLEDDisplay()
{
    // Set all LEDs off. The LEDs are active-low
    LATB = LATB | LED_MASK_PORTB;
    LATC = LATC | LED_MASK_PORTC;
    // Set appropriate pins as output
    TRISB &= ~LED_MASK_PORTB;
    TRISC &= ~LED_MASK_PORTC;
}

void setLEDDisplay(uint16_t bits)
{
    // Bit-to-pin mapping:
    // MSB                                 LSB
    // 9   8   7   6   5   4   3   2   1   0
    // B7  B6  B5  B4  C5  C4  C3  C2  C1  C0

    // LEDs are active-low, so invert the given bits
    bits = ~bits;

    // Map bits onto port latches
    LATB = (LATB & ~LED_MASK_PORTB) | ((bits & 0b1111000000) >> 2);
    LATC = (LATC & ~LED_MASK_PORTC) | (bits & 0b0000111111);
}
