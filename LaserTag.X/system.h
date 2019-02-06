#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

#define HIGH 1
#define LOW 0

void configureSystem(void);
void delay(uint32_t d);
void delayTiny(uint32_t d);

#endif  // SYSTEM_H
