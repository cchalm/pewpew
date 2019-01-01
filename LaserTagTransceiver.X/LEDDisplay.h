#ifndef LEDDISPLAY_H
#define LEDDISPLAY_H

#include <stdint.h>

void initializeLEDDisplay(void);
// LSB is the first LED. 1 is on, 0 is off.
void setLEDDisplay(uint16_t bits);

#endif /* LEDDISPLAY_H */
