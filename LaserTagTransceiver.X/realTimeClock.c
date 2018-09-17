/*
 * File:   LEDDisplay.c
 * Author: Chris
 *
 * Created on August 11, 2018, 9:45 PM
 */

#include "realTimeClock.h"

#include <stdbool.h>
#include <xc.h>

// TEMPORARY CHANGES - we don't have enough period-adjustable timers on one chip
// to do the RTC properly, so we're approximating with a 16-bit timer for now.
// Revert to the old implementation when we move the shot timing logic to
// another chip

void initializeRTC()
{
    // Select Fosc (32MHz) as the timer 2 clock source
    TMR1CS = 0b01;
    // No prescaler
    // Enable overflow interrupts
    TMR1IE = 1;
    // Clear the timer
    TMR1 = 0;
    // Start the timer
    TMR1ON = 1;
}

static volatile count_t g_s_count = 0;
static volatile count_t g_ms_count = 0;

void rtcTimerInterruptHandler(void)
{
    // Real-time timer logic

    if (!(TMR1IF && TMR1IE))
        return;

    static count_t next_s = 1000;

    g_ms_count++;

    if (g_ms_count == next_s)
    {
        g_s_count++;
        next_s = g_ms_count + 1000;
    }

    // Reset flag
    TMR1IF = 0;
}

count_t getSecondCount()
{
    // two-byte read race?
    return g_s_count;
}

count_t getMillisecondCount()
{
    // two-byte read race?
    return g_ms_count;
}