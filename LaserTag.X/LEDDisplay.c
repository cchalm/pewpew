#include "LEDDisplay.h"

#include <xc.h>

// C4 - C7
#define LED_MASK_PORTC 0b11110000
// D2 - D7
#define LED_MASK_PORTD 0b11111100

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

void initializeLEDDisplay()
{
    // Set all LEDs off. The LEDs are active-low
    LATC = LATC | LED_MASK_PORTC;
    LATD = LATD | LED_MASK_PORTD;
    // Set appropriate pins as output
    TRISC &= ~LED_MASK_PORTC;
    TRISD &= ~LED_MASK_PORTD;
}

void setLEDDisplay(unsigned int bits)
{
    // Bit-to-pin mapping:
    // MSB                                 LSB
    // 9   8   7   6   5   4   3   2   1   0
    // D7  D6  D5  D4  C7  C6  C5  C4  D3  D2

    // LEDs are active-low, so invert the given bits
    bits = ~bits;

    // Map bits onto port latches
    LATD = (LATD & ~LED_MASK_PORTD) | ((bits & 0b0000000011) << 2) | ((bits & 0b1111000000) >> 2);
    LATC = (LATC & ~LED_MASK_PORTC) | ((bits & 0b0000111100) << 2);
}
