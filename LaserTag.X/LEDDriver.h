#ifndef LEDDRIVER_H
#define	LEDDRIVER_H

#include <stdbool.h>
#include <stdint.h>

// All of these functions return false if there is a transmission currently in
// progress. These functions are NOT thread safe - do not call them in ISRs.
void LEDDriver_setShutdown(bool shutdown);
void LEDDriver_setPWM(uint8_t start_index, uint8_t* pwm, uint8_t pwm_len);
void LEDDriver_flushChanges(void);
void LEDDriver_setControl(uint8_t start_index, uint8_t* control, uint8_t control_len);
void LEDDriver_setGlobalEnable(bool enable);
void LEDDriver_reset(void);

#endif	/* LEDDRIVER_H */
