#ifndef LEDDISPLAY_H
#define	LEDDISPLAY_H

void initializeLEDDisplay(void);
// LSB is the first LED. 1 is on, 0 is off.
void setLEDDisplay(unsigned int bits);

#endif	/* LEDDISPLAY_H */
