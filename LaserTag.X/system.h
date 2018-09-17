#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

#define OUTPUT 0
#define INPUT 1
#define HIGH 1
#define LOW 0

#define PIN_MUZZLE_FLASH RA5
#define PIN_HIT_LIGHT RA4

#define INPUT_PORT PORTC

#define PIN_TRIGGER RC7
#define TRIGGER_OFFSET 7
#define TRIGGER_MASK 0b1000000
#define TRIGGER_PRESSED HIGH
#define TRIGGER_NOT_PRESSED LOW

#define PIN_RELOAD RC6
#define RELOAD_OFFSET 6
#define RELOAD_MASK 0b100000
#define MAG_IN HIGH
#define MAG_OUT LOW

void configureSystem(void);
void delay(uint32_t d);
void delayTiny(uint32_t d);
void error(uint16_t error_code);

#endif // SYSTEM_H
