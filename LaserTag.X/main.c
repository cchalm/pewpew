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

#include "system.h"

typedef unsigned char bool_t;
typedef unsigned int count_t;

#define TRUE 1
#define FALSE 0

#define MAX_HEALTH 10
#define MAX_AMMO 10

// Shots per second
#define FIRE_RATE 10
// Delay between shots in ms
#define SHOT_DELAY 1000/(FIRE_RATE)
#define FULL_AUTO TRUE
#define SHOT_DATA_LENGTH 10

// Minimum delay, in ms, from trigger falling edge to rising edge for a shot to
// be registered. This is to mitigate button bounce.
#define BOUNCE_DELAY 2

#define DEBUG FALSE

enum
{
    INVALID_SHOT_DATA_RECEIVED = 0b1001
};

int health;
int ammo;

// Counts seconds. Overflows to 0 after ~18 hours (1092.25 minutes)
volatile count_t s_count;
// Counts milliseconds. Overflows to 0 after ~1 minute (65.535 seconds)
volatile count_t ms_count;

// The time, in the form of a millisecond count corresponding to ms_counter,
// at which we can shoot again
count_t shot_enable_ms_count;
volatile bool_t can_shoot;

bool_t mag_in;
bool_t mag_was_out;

bool_t trigger_pressed;
bool_t trigger_was_pressed;

unsigned int shot_data_to_send;

volatile unsigned int shot_data_received;
volatile bool_t shot_received;

volatile TMR1_t pulses_received[SHOT_DATA_LENGTH];
volatile TMR1_t gaps_received[SHOT_DATA_LENGTH];

unsigned char input_state;

void setHealthDisplay(unsigned char value);
void shoot(void);
void flash(void);
// Given two TMR1 measurements, with "first" being measured before "second", and
// the number of overflows that occured between the two measurements, capped at
// two, returns the number of TMR1 ticks that elapsed between the two
// measurements, capped at the maximum value of TMR1_t.
TMR1_t getPulseWidth(TMR1_t first, TMR1_t second, count_t overflow_count);

void HandleTimingInterrupt(void);
void HandleShootingInterrupt(void);
void HandleShotReceptionInterrupt(void);

int main(void) {
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

    can_shoot = TRUE;
    shot_enable_ms_count = 0;

    shot_data_to_send = 0b0101010101;

    health = MAX_HEALTH;
    ammo = MAX_AMMO;
    setHealthDisplay(ammo);

    // Set the "was" variables for use on the first loop iteration
    input_state = INPUT_PORT;
    trigger_was_pressed = (input_state >> TRIGGER_OFFSET) & 1 == TRIGGER_PRESSED;
    mag_was_out = (input_state >> RELOAD_OFFSET) & 1 == MAG_OUT;
    
    PIN_SHOT_LIGHT = LOW;

    ms_count = 0;
    s_count = 0;

    // Start timer1
    //TMR1 = 0;
    TMR1IF = 0;
    TMR1IE = 1;

    TMR0 = TMR0_PRELOAD;
    // Enable interrupts (go, go, go!)
    GIE = 1;

    while(1)
    {
        if (shot_received)
        {
            // Make a copy so it doesn't get overwritten
            unsigned int player_id = shot_data_received;
            setLEDDisplay(player_id);

            PIN_HIT_LIGHT = 0;
            NOP();
            PIN_HIT_LIGHT = 1;

            // Look up player ID
            if (player_id != shot_data_to_send && !DEBUG)
                error(INVALID_SHOT_DATA_RECEIVED);

            shot_received = FALSE;
        }
        
        // Snapshot input state. This way, we don't have to worry about the
        // inputs changing while we're performing logic.
        input_state = INPUT_PORT;
        trigger_pressed = (input_state >> TRIGGER_OFFSET) & 1 == TRIGGER_PRESSED;
        mag_in = (input_state >> RELOAD_OFFSET) & 1 == MAG_IN;

        if (trigger_pressed && (FULL_AUTO || !trigger_was_pressed) && can_shoot)
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
            shot_enable_ms_count = ms_count + SHOT_DELAY;
            can_shoot = FALSE;
        }
        else if (!FULL_AUTO)
        {
            // When in semi-auto mode, only enable shot a short time after
            // releasing the trigger. This is to prevent button bounces from
            // causing shots when releasing the trigger
            if (trigger_was_pressed && (shot_enable_ms_count - ms_count < BOUNCE_DELAY))
            {
                // Trigger was pressed and now isn't - the trigger was released
                can_shoot = FALSE;
                shot_enable_ms_count = ms_count + BOUNCE_DELAY;
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
void interrupt ISR(void)
{
    if (TMR0IF && TMR0IE)
    {
        HandleTimingInterrupt();
    }
    else if (CCP1IF && CCP1IE)
    {
        HandleShootingInterrupt();
    }
    else
    {
        HandleShotReceptionInterrupt();
    }
}

void setHealthDisplay(unsigned char value)
{
    // Shift in zeros from the right and invert
    setLEDDisplay( ~(0b1111111111 << value) );
}

void shoot(void)
{
    shot_data_to_send = (shot_data_to_send >> 1) | ((TMR0 % 2) << (SHOT_DATA_LENGTH - 1));
    TMR1ON = 0;
    CCPR1 = TMR1;
    TMR1ON = 1;
    // Enable CCP1 interrupts and immediately trigger an interrupt, which begins
    // the shot transmission. Manually setting the interrupt flag may not be the
    // best practice. Another option is to pull the shooting logic out of the
    // ISR and put it into another method, and then call that method here.
    CCP1IF = 1;
    CCP1IE = 1;
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

TMR1_t getPulseWidth(TMR1_t first, TMR1_t second, count_t overflow_count)
{
    /*
     * When measuring the length of a pulse using two TMR1 measurements, there
     * are four cases to consider, covering every possible outcome. Below,
     * "first" refers to the first measurement, and "second" refers to the
     * second measurement.
     *
     * Case 1: overflow_counter == 2
     *  time = (TMR1_t)(-1)
     *  Timer1 has overflowed at least twice during this pulse. This means at
     *  least one full timer cycle has elapsed (one overflow for the beginning
     *  of the cycle, one for the end).
     *
     * Case 2: overflow_counter == 1 && second >= first
     *  time = (TMR1_t)(-1)
     *  Timer1 has overflowed exactly once during this pulse, and the time (in
     *  terms of timer1 ticks) of the end of the pulse is greater than or equal
     *  to the time of the start. The pulse started in some timer cycle, and
     *  ended at the same time or later in the next timer cycle, so more than
     *  one timer cycle has elapsed.
     *
     * Case 3: overflow_counter == 1 && second < first
     *  time = (TMR1_t)(second - first)
     *  The pulse started in some timer cycle, and ended at an earlier count in
     *  the next timer cycle. second - first will be negative, but it will
     *  underflow when cast and produce the correct value for elapsed ticks (you
     *  can verify this with a simple number line).
     *
     * Case 4: overflow_counter == 0
     *  time = (TMR1_t)(second - first)
     *  Timer hasn't overflowed, simple subtraction to get elapsed time. second
     *  is guaranteed to be greater than or equal to first.
     */

    TMR1_t pulse_width;
    if ( (overflow_count == 2) ||
         (overflow_count == 1 && second >= first) )
    {
        // Cases 1 and 2
        pulse_width = (TMR1_t)(-1);
    }
    else
    {
        // Cases 3 and 4
        pulse_width = (TMR1_t)(second - first);
    }

    return pulse_width;
}

void HandleTimingInterrupt(void)
{
    // Real-time timer logic

    static count_t next_s = 1000;

    ms_count++;

    if (ms_count == next_s)
    {
        s_count++;
        next_s = ms_count + 1000;
    }

    if (ms_count == shot_enable_ms_count)
    {
        can_shoot = TRUE;
    }

    // Preload timer
    TMR0 = TMR0_PRELOAD;
    // Reset flag
    TMR0IF = 0;
}

void HandleShootingInterrupt(void)
{
    // Shooting logic

    // Send MSB first
    static unsigned char shot_data_index = SHOT_DATA_LENGTH;
    static bool_t next_edge_rising = TRUE;

    if (next_edge_rising)
    {
        PIN_SHOT_LIGHT = HIGH;
        shot_data_index--;
        TMR1_t pulse_width = ((shot_data_to_send >> shot_data_index) & 1) ?
            ONE_PULSE_WIDTH : ZERO_PULSE_WIDTH;

        CCPR1 += pulse_width;
        
        next_edge_rising = FALSE;
    }
    else // !next_edge_rising
    {
        PIN_SHOT_LIGHT = LOW;
        next_edge_rising = TRUE;

        if (shot_data_index == 0)
        {
            // This is the final falling edge of the shot, reset the data
            // index and disable CCP3 interrupts
            shot_data_index = SHOT_DATA_LENGTH;
            // Disable CCP3 interrupts
            CCP1IE = 0;
        }
        else
        {
            CCPR1 += PULSE_GAP_WIDTH;
        }
    }

    // Reset flag
    CCP1IF = 0;
}

void HandleShotReceptionInterrupt(void)
{
    // Shot reception logic

    /*
     * HOW IT WORKS
     *
     * A shot transmission contains some number of bits. A long pulse is a 1,
     * and a short pulse is a 0, and these pulses are separated by a
     * known-length silence.
     *
     * CCP modules can "capture" pin events like rising and falling edges. When
     * the event occurs, the Timer1 register gets copied into the CCP register
     * by the hardware. By examining the CCP register in software, we can know
     * when that event occured.
     *
     * We have CCP2 set up to capture rising edges and CCP3 set up to capture
     * falling edges. A rising edge represents the END of a pulse and the START
     * of silence because TSOP2240 sensors are active low. To keep it simple,
     * we'll say CCP3 captures the START of a pulse, and CCP2 captures the END
     * of a pulse. We've also configured the modules to trigger interrupts when
     * captures occur.
     *
     * When we get a CCP2 interrupt, we know a pulse has just ended. The CCP2
     * register contains the time (the Timer1 count) that the pulse ended, and,
     * critically, the CCP3 register contains the time that the pulse started.
     * Subtract the end time from the start time and that's the pulse length.
     * The comments in the getPulseWidth method describe in detail how we
     * account for Timer1 overflows.
     *
     * If the pulse is an expected length, we handle the bit and continue. If it
     * is an unexpected length, we disable end-of-pulse interrupts. We handle
     * data when we see the ends of pulses, so disabling those interrupts stops
     * data reception. We also stop accepting data after receiving the expected
     * number of bits from a transmission.
     *
     * Data reception resets upon observing a long silence. Disabling
     * end-of-pulse interrupts does NOT disable capture events, so pulses are
     * still being timed.
     */

    // Counts timer1 overflows, capped at 2
    static unsigned char overflow_count;

    // Data accumulator
    static unsigned int data = 0;
    static unsigned char bit_count = 0;

    if (TMR1IF && TMR1IE)
    {
        overflow_count++;

        if (overflow_count == 2)
        {
            // Once we hit two, turn off interrupts. We don't want to
            // increment further.
            TMR1IE = 0;
        }

        // Reset flag
        TMR1IF = 0;
    }
    else if (CCP3IF && CCP3IE)
    {
        // Falling edge detected (start of pulse, end of silence)

        // CCP2 is the start time of the silence
        // CCP3 is the end time of the silence

        TMR1_t pulse_width = getPulseWidth(CCPR2, CCPR3, overflow_count);
        gaps_received[bit_count] = pulse_width;

        if ( (pulse_width > 3000 && !DEBUG) ||
             (bit_count == SHOT_DATA_LENGTH && DEBUG) )
        {
            // Long silence. Reset
            data = 0;
            bit_count = 0;

            // Re-enable end-of-pulse interrupts, in case we disabled them
            CCP2IF = 0;
            CCP2IE = 1;
        }

        // Pause timer 1, so that we don't have to worry about it overflowing
        // between the next several instructions.
        TMR1ON = 0;
        /* 
         * We need to reset the overflow count for the next cycle. Timing for
         * the next cycle is happening asynchronously right now, so if an
         * overflow has occurred since we started handling this interrupt, we
         * need to record it for the next cycle.
         *
         * We can assume the timer has not gone through a full cycle in this
         * time because a full cycle even at 8MHz takes 8ms. Given this
         * assumption, if the current timer value (which is frozen) is less than
         * the timer value when we started this interrupt handler, the timer has
         * overflowed exactly once. Otherwise, it hasn't overflowed.
         */
        overflow_count = TMR1 < CCPR3 ? 1 : 0;
        // We handled the possible overflow above, so reset the timer interrupt
        // to avoid handling the same overflow twice.
        TMR1IF = 0;
        // Enable timer 1 interrupts, which may have been disabled because we
        // were already at two timer overflows.
        TMR1IE = 1;
        // Resume timer 1
        TMR1ON = 1;
        
        CCP3IF = 0;
    }
    else if (CCP2IF && CCP2IE)
    {
        // Rising edge detected (end of pulse, start of silence)

        // CCP3 is the start time of the pulse
        // CCP2 is the end time of the pulse

        TMR1_t pulse_width = getPulseWidth(CCPR3, CCPR2, overflow_count);
        pulses_received[bit_count] = pulse_width;

        if (pulse_width > 2000 && pulse_width < 3000)
        {
            // Received a 0
            data <<= 1;
            bit_count++;
        }
        else if (pulse_width > 4000 && pulse_width < 5000)
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

            if (DEBUG)
                bit_count++;
            else
                // Disable end-of-pulse interrupts to stop reading data
                CCP2IE = 0;
        }

        if (bit_count == SHOT_DATA_LENGTH)
        {
            shot_received = TRUE;
            shot_data_received = data;

            // We could just reset the data accumulator here and let more
            // shot data come through immediately, on the off chance that
            // the time between the end of one shot and the start of the
            // next is less than the silence reset, but instead we're just
            // going to wait for silence. This is more robust and has a
            // negligible impact on shot throughput.
            CCP1IE = 0;
        }

        // See comments on similar lines above
        TMR1ON = 0;
        overflow_count = TMR1 < CCPR2 ? 1 : 0;
        TMR1IF = 0;
        TMR1IE = 1;
        TMR1ON = 1;

        CCP2IF = 0;
    }
}
