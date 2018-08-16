#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

#define OUTPUT 0
#define INPUT 1
#define HIGH 1
#define LOW 0

#define PIN_MUZZLE_FLASH RB4
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

typedef uint8_t TMR0_t;

void configureSystem(void);
void delay(uint32_t d);
void delayTiny(uint32_t d);
void error(uint16_t error_code);

#endif // SYSTEM_H
