/* 
 * File:   realTimeClock.h
 * Author: chris
 *
 * Created on August 12, 2018, 3:14 AM
 */

#ifndef REALTIMECLOCK_H
#define	REALTIMECLOCK_H

typedef unsigned int count_t;

void initializeRTC(void);
void rtcTimerInterruptHandler(void);

// Counts seconds. Overflows to 0 after ~18 hours (1092.25 minutes)
count_t getSecondCount(void);
// Counts milliseconds. Overflows to 0 after ~1 minute (65.535 seconds)
count_t getMillisecondCount(void);

#endif	/* REALTIMECLOCK_H */

