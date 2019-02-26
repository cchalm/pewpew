#ifndef REALTIMECLOCK_H
#define REALTIMECLOCK_H

#include <stdint.h>

void initializeRTC(void);
void rtcTimerInterruptHandler(void);

// Counts seconds. Overflows to 0 after ~136 years
uint32_t getSecondCount(void);
// Counts milliseconds. Overflows to 0 after ~50 days
uint32_t getMillisecondCount(void);

#endif /* REALTIMECLOCK_H */
