/*
 * File:   LEDDisplay.c
 * Author: Chris
 *
 * Created on August 11, 2018, 9:45 PM
 */

#include "realTimeClock.h"

#include <stdbool.h>
#include <xc.h>

#define TMR0_PRELOAD 5 // 255 - 250

void initializeRTC()
{
    INTCONbits.TMR0IE = 1; // Enable Timer0 interrupt
    OPTION_REGbits.TMR0CS = 0; // Select Timer0 clock source: Internal instruction cycle clock
    OPTION_REGbits.PSA = 0; // Enable prescaler
    OPTION_REGbits.PS = 0b100; // Set prescaler 1:32
    TMR0 = TMR0_PRELOAD;
}

static volatile count_t g_s_count = 0;
static volatile count_t g_ms_count = 0;

void handleRTCTimerInterrupt(void)
{
    // Real-time timer logic

    if (!(TMR0IF && TMR0IE))
        return;

    static count_t next_s = 1000;

    g_ms_count++;

    if (g_ms_count == next_s)
    {
        g_s_count++;
        next_s = g_ms_count + 1000;
    }

    // Preload timer
    TMR0 = TMR0_PRELOAD;
    // Reset flag
    TMR0IF = 0;
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