#ifndef LEDDISPLAY_H
#define LEDDISPLAY_H

#include <stdint.h>

void initializeLEDs(void);

void flashMuzzleLight(void);
void flashHitLight(void);

// LSB is the first LED. 1 is on, 0 is off.
void setBarDisplay1(uint16_t bits);
void setBarDisplay2(uint16_t bits);

void setHealthDisplay(uint8_t value);
void setAmmoDisplay(uint8_t value);

void setRGB1(uint8_t r, uint8_t g, uint8_t b);
void setRGB2(uint8_t r, uint8_t g, uint8_t b);
void setRGB3(uint8_t r, uint8_t g, uint8_t b);
void setRGB4(uint8_t r, uint8_t g, uint8_t b);
void setRGB5(uint8_t r, uint8_t g, uint8_t b);

#endif /* LEDDISPLAY_H */
