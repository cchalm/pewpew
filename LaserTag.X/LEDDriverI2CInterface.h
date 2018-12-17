#ifndef LEDDRIVERI2CINTERFACE_H
#define	LEDDRIVERI2CINTERFACE_H

#include <stdbool.h>
#include <stdint.h>

// All of these functions return false if there is a transmission currently in
// progress. These functions are NOT thread safe - do not call them in ISRs.
bool LEDDriver_setShutdown(bool shutdown);
bool LEDDriver_setPWM(uint8_t startIndex, uint8_t* pwm, uint8_t pwmLen);
bool LEDDriver_flushChanges(void);
bool LEDDriver_setControl(uint8_t startIndex, uint8_t* control, uint8_t controlLen);
bool LEDDriver_setGlobalEnable(bool enable);
bool LEDDriver_reset(void);

#endif	/* LEDDRIVERI2CINTERFACE_H */
