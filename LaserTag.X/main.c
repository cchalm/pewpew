/*
 * File:   main.c
 * Author: Chris
 *
 * Created on January 4, 2015, 3:36 PM
 */

#include <xc.h>

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config VCAPEN = OFF     // Voltage Regulator Capacitor Enable bit (Vcap functionality is disabled on RA6.)
#pragma config PLLEN = ON       // PLL Enable (4x PLL enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low Power Brown-Out Reset Enable Bit (Low power brown-out is disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

#include "IRTransmitter.h"
#include "LEDDisplay.h"
#include "system.h"
#include "transmissionConstants.h"

#include <stdbool.h>

typedef unsigned int count_t;

#define MAX_HEALTH 10
#define MAX_AMMO 10

// Shots per second
#define FIRE_RATE 10
// Delay between shots in ms
#define SHOT_DELAY 1000/(FIRE_RATE)
#define FULL_AUTO true

// Minimum delay, in ms, from trigger falling edge to rising edge for a shot to
// be registered. This is to mitigate button bounce.
#define BOUNCE_DELAY 2

//#define DEBUG

enum
{
    INVALID_SHOT_DATA_RECEIVED = 0b1001,
    TRANSMISSION_OVERLAP = 0b100101
};

// Counts seconds. Overflows to 0 after ~18 hours (1092.25 minutes)
volatile count_t g_s_count;
// Counts milliseconds. Overflows to 0 after ~1 minute (65.535 seconds)
volatile count_t g_ms_count;

// The time, in the form of a millisecond count corresponding to ms_counter,
// at which we can shoot again
count_t g_shot_enable_ms_count;
volatile bool g_can_shoot;

unsigned int g_shot_data_to_send;

volatile unsigned int g_shot_data_received;
volatile bool g_shot_received;

#define TRANSMISSION_DATA_LENGTH 10

volatile TMR1_t g_pulses_received[TRANSMISSION_DATA_LENGTH];
#ifdef DEBUG
volatile TMR1_t g_gaps_received[TRANSMISSION_DATA_LENGTH];
#endif

void setHealthDisplay(unsigned char value);
void shoot(void);
void flash(void);
// Given two TMR1 measurements, with "first" being measured before "second", and
// the number of overflows that occurred between the two measurements, capped at
// two, returns the number of TMR1 ticks that elapsed between the two
// measurements, capped at the maximum value of TMR1_t.
TMR1_t getPulseWidth(TMR1_t first, TMR1_t second, count_t overflow_count);

void handleTimingInterrupt(void);
void handleShotReceptionInterrupt(void);

void handleShotReceived(unsigned int shot_data_received);

int main(void)
{
    configureSystem();

    LATA = 0b111111;
    LATD = 0b1111;
    LATB = 0b111111;

    setLEDDisplay(0b1010101010);
    delay(400);
    setLEDDisplay(0b0000000000);
    delay(400);
    setLEDDisplay(0b1010101010);
    delay(400);
    setLEDDisplay(0b0000000000);
    delay(400);

    g_can_shoot = true;
    g_shot_enable_ms_count = 0;

    g_shot_data_to_send = 0b0101010101;

    int health = MAX_HEALTH;
    int ammo = MAX_AMMO;
    setHealthDisplay(ammo);

    // Set the "was" variables for use on the first loop iteration
    unsigned int input_state = INPUT_PORT;
    bool trigger_was_pressed = ((input_state >> TRIGGER_OFFSET) & 1) == TRIGGER_PRESSED;
    bool mag_was_out = ((input_state >> RELOAD_OFFSET) & 1) == MAG_OUT;

    g_ms_count = 0;
    g_s_count = 0;

    TMR0 = TMR0_PRELOAD;
    // Enable interrupts (go, go, go!)
    GIE = 1;

    while(true)
    {
        if (g_shot_received)
        {
            // Make a copy so it doesn't get overwritten
            unsigned int player_id = g_shot_data_received;
            setLEDDisplay(player_id);

            PIN_HIT_LIGHT = 0;
            NOP();
            PIN_HIT_LIGHT = 1;

            // Look up player ID
#ifndef DEBUG
            if (player_id != g_shot_data_to_send)
                error(INVALID_SHOT_DATA_RECEIVED);
#endif

            g_shot_received = false;
        }

        // Snapshot input state. This way, we don't have to worry about the
        // inputs changing while we're performing logic.
        input_state = INPUT_PORT;
        bool trigger_pressed = ((input_state >> TRIGGER_OFFSET) & 1) == TRIGGER_PRESSED;
        bool mag_in = ((input_state >> RELOAD_OFFSET) & 1) == MAG_IN;

        if (trigger_pressed && (FULL_AUTO || !trigger_was_pressed) && g_can_shoot)
        {
            // The trigger is pressed, it wasn't pressed before, and the shot
            // timer has elapsed; try to shoot

            // Make sure we have ammo and the magazine is in
            if (mag_in && ammo)
            {
                shoot();
            }

            // Putting the following lines outside the ammo conditional means
            // that you can only get a "not enough ammo" signal, like a sound,
            // at the rate of fire of the tagger, not as fast as you can
            // press the trigger

            // Indicate that a shot has just occurred and store the time at
            // which we can shoot again
            g_shot_enable_ms_count = g_ms_count + SHOT_DELAY;
            g_can_shoot = false;
        }
        else if (!FULL_AUTO)
        {
            // When in semi-auto mode, only enable shot a short time after
            // releasing the trigger. This is to prevent button bounces from
            // causing shots when releasing the trigger
            if (trigger_was_pressed && (g_shot_enable_ms_count - g_ms_count < BOUNCE_DELAY))
            {
                // Trigger was pressed and now isn't - the trigger was released
                g_can_shoot = false;
                g_shot_enable_ms_count = g_ms_count + BOUNCE_DELAY;
            }
        }

        if (!mag_in)
            //setHealthDisplay(0);

        if (mag_in && mag_was_out)
        {
            // Mag has been put back in, reload
            ammo = MAX_AMMO;
            //setHealthDisplay(ammo);
        }

        // Update "was" variables
        trigger_was_pressed = trigger_pressed;
        mag_was_out = !mag_in;
    }
}

// Main Interrupt Service Routine (ISR)
void __interrupt () ISR(void)
{
    handleTimingInterrupt();
    handleTransmissionTimingInterrupt();
    handleShotReceptionInterrupt();
}

void setHealthDisplay(unsigned char value)
{
    // Shift in zeros from the right and invert
    setLEDDisplay( ~(0b1111111111 << value) );
}

void shoot(void)
{
    g_shot_data_to_send = (g_shot_data_to_send >> 1) |
            ((TMR0 % 2) << (TRANSMISSION_DATA_LENGTH - 1));

    bool transmissionInProgress = !transmitAsync(g_shot_data_to_send);
    if (transmissionInProgress)
        error(TRANSMISSION_OVERLAP);
    
    flash();
    //ammo--;
    //setLEDDisplay(shot_data_to_send);
}

void flash(void)
{
    PIN_MUZZLE_FLASH = 0;
    NOP();
    PIN_MUZZLE_FLASH = 1;
}

void handleTimingInterrupt(void)
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

    if (g_ms_count == g_shot_enable_ms_count)
    {
        g_can_shoot = true;
    }

    // Preload timer
    TMR0 = TMR0_PRELOAD;
    // Reset flag
    TMR0IF = 0;
}

void handleShotReceptionInterrupt(void)
{
    // Shot reception logic

    /*
     * HOW IT WORKS
     *
     * A shot transmission contains some number of bits. A long pulse is a 1,
     * and a short pulse is a 0, and these pulses are separated by a
     * known-length silence.
     * 
     * The current implementation uses the INT pin, a pin that can be set up to
     * trigger interrupts on rising or falling edges. The interrupt handler
     * reverses the edge on each call, allowing us to handle both rising and
     * falling edges.
     * 
     * When we see any edge, we save the value of timer 1 and reset it to 0.
     * Because we always reset it to 0, the value of the timer is the elapsed
     * time since the last edge. We time both pulses and gaps this way.
     *
     * If a pulse is an expected length, we handle the bit and continue. If it
     * is an unexpected length, we disable end-of-pulse interrupts. We handle
     * data when we see the ends of pulses, so disabling those interrupts stops
     * data reception. We also stop accepting data after receiving the expected
     * number of bits from a transmission.
     *
     * Data reception resets upon observing a long silence.
     */

    // Data accumulator
    static unsigned int data = 0;
    static unsigned char bit_count = 0;

    static bool wait_for_silence = false;
    
    static bool next_edge_rising = false;

    if (INTE && INTF)
    {
        // Investigate using the INT pin instead of the CCP module
        
        TMR1ON = 0;
        TMR1_t pulse_width = TMR1;
        TMR1 = 0;
        TMR1ON = 1;
   
        if (next_edge_rising)
        {
            // Rising edge detected (end of pulse)
            
            // Only process the pulse if we're not waiting for a long silence to
            // reset.
            if (!wait_for_silence)
            {
                g_pulses_received[bit_count] = pulse_width;

                if (pulse_width > 250 && pulse_width < 350)
                {
                    // Received a 0
                    data <<= 1;
                    bit_count++;
                }
                else if (pulse_width > 400 && pulse_width < 500)
                {
                    // Received a 1
                    data = (data << 1) | 1;
                    bit_count++;
                }
                else
                {
                    // Invalid pulse width. Something has gone wrong, so we're
                    // going to stop reading in data and wait until we see a
                    // long period of silence

#ifndef DEBUG
                    wait_for_silence = true;
#else
                    bit_count++;
#endif
                }

                if (bit_count == TRANSMISSION_DATA_LENGTH)
                {
                    g_shot_received = true;
                    g_shot_data_received = data;

                    // We could just reset the data accumulator here and let
                    // more shot data come through immediately, on the off
                    // chance that the time between the end of one shot and the
                    // start of the next is less than the silence reset, but
                    // instead we're just going to wait for silence. This is
                    // more robust and has a negligible impact on shot
                    // throughput.
                    wait_for_silence = true;
                }
            }
        }
        else // !next_edge_rising
        {
            // Falling edge detected (end of silence)
#ifdef DEBUG
            g_gaps_received[bit_count] = pulse_width;
#endif
            
#ifndef DEBUG
            if (pulse_width > 300)
#else
            if (bit_count == SHOT_DATA_LENGTH)
#endif
            {
                // Long silence. Reset
                data = 0;
                bit_count = 0;

                wait_for_silence = false;
            }
        }
        
        next_edge_rising = !next_edge_rising;

        // Toggle the edge we're looking for. This can cause spurious
        // interrupts, so we disable them during the change.
        INTE = 0;
        // 1 to capture rising edges, 0 to capture falling edges
        INTEDG = next_edge_rising;
        INTF = 0;
        INTE = 1;
    }
}
