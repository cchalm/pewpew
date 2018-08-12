/* 
 * File:   LEDDisplay.h
 * Author: chris
 *
 * Created on August 11, 2018, 9:49 PM
 */

#ifndef LEDDISPLAY_H
#define	LEDDISPLAY_H

void initializeLEDDisplay(void);
// LSB is the first LED. 1 is on, 0 is off.
void setLEDDisplay(unsigned int bits);

#endif	/* LEDDISPLAY_H */
