/*
 * File:   LEDDisplay.c
 * Author: Chris
 *
 * Created on August 11, 2018, 9:45 PM
 */

#include "realTimeClock.h"

#include <stdbool.h>
#include <xc.h>

void initializeRTC()
{
    // Note that the datasheet says HFINTOSC (16MHz) on page 429. We selected
    // the HFINTOSC frequency as 32MHz with the OSCFRQ register though, and
    // observation confirms 32MHz.
    // Select HFINTOSC (32MHz) as the timer 2 clock source
    T2CLKCON = 0b0011;
    // Set prescaler to 1:128
    T2CONbits.CKPS = 0b111;
    // Set period register (the value after which the timer will overflow) to
    // achieve an overflow frequency of 1kHz (one overflow per millisecond)
    // (PR + 1) * (1/f) = t_overflow
    // PR = f * t_overflow - 1
    // PR = (32MHz / 128) * (0.001) - 1 = 249
    T2PR = 249;
    // Enable overflow interrupts
    TMR2IE = 1;
    // Clear the timer
    TMR2 = 0;
    // Start the timer
    TMR2ON = 1;
}

static volatile count_t g_s_count = 0;
static volatile count_t g_ms_count = 0;

void rtcTimerInterruptHandler(void)
{
    // Real-time timer logic

    if (!(TMR2IF && TMR2IE))
        return;

    static count_t next_s = 1000;

    g_ms_count++;

    if (g_ms_count == next_s)
    {
        g_s_count++;
        next_s = g_ms_count + 1000;
    }

    // Reset flag
    TMR2IF = 0;
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