#ifndef LEDDISPLAY_H
#define	LEDDISPLAY_H

#include <stdint.h>

void initializeLEDDisplay(void);
// LSB is the first LED. 1 is on, 0 is off.
void setBarDisplay1(uint16_t bits);
void setBarDisplay2(uint16_t bits);

#endif	/* LEDDISPLAY_H */
