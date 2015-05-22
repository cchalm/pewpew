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

#define FULL_AUTO FALSE

#define SHOT_DATA_LENGTH 8

// Minimum delay, in ms, from trigger falling edge to rising edge for a shot to
// be registered. This is to mitigate button bounce.
#define BOUNCE_DELAY 2

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

unsigned char shot_data_to_send;
volatile bool_t shot_requested;

volatile unsigned char shot_data_received;
volatile bool_t shot_received;

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

    setLEDDisplay(0b10101010);
    delay(400);
    setLEDDisplay(0b00000000);
    delay(400);
    setLEDDisplay(0b10101010);
    delay(400);
    setLEDDisplay(0b00000000);
    delay(400);

    can_shoot = TRUE;
    shot_enable_ms_count = 0;

    shot_data_to_send = 0b01010101;
    shot_requested = FALSE;

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
            unsigned char player_id = shot_data_received;

            PIN_HIT_LIGHT = 0;
            NOP();
            PIN_HIT_LIGHT = 1;

            // Look up player ID
            if (player_id != shot_data_to_send)
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
            setHealthDisplay(0);

        if (mag_in && mag_was_out)
        {
            // Mag has been put back in, reload
            ammo = MAX_AMMO;
            setHealthDisplay(ammo);
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
    else if (CCP3IF && CCP3IE)
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
    shot_data_to_send = (shot_data_to_send << 1) | (TMR0 % 2);
    // Enable CCP3 interrupts and immediately trigger an interrupt, which begins
    // the shot transmission. Manually setting the interrupt flag may not be the
    // best practice. Another option is to pull the shooting logic out of the
    // ISR and put it into another method, and then call that method here.
    CCP3IF = 1;
    CCP3IE = 1;
    flash();
    ammo--;
    setHealthDisplay(ammo);
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
        CCPR3 = TMR1 + pulse_width;
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
            CCP3IE = 0;
        }
        else
        {
            CCPR3 = TMR1 + PULSE_GAP_WIDTH;
        }
    }

    // Reset flag
    CCP3IF = 0;
}

void HandleShotReceptionInterrupt(void)
{
    // Shot reception logic

    // Counts timer1 overflows, capped at 2
    static unsigned char overflow_count;

    // Data accumulator
    static unsigned char data = 0;
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
    else if (CCP2IF && CCP2IE)
    {
        // Falling edge detected (start of pulse, end of silence)

        TMR1IF = 0;

        TMR1_t pulse_width = getPulseWidth(CCPR1, CCPR2, overflow_count);

        if ( pulse_width > 2500 )
        {
            // Long silence. Reset
            data = 0;
            bit_count = 0;

            // Re-enable end-of-pulse interrupts, in case we disabled them
            CCP1IF = 0;
            CCP1IE = 1;
        }

        overflow_count = 0;
        TMR1IE = 1;
        CCP2IF = 0;
    }
    else if (CCP1IF && CCP1IE)
    {
        // Rising edge detected (end of pulse, start of silence)

        TMR1IF = 0;

        TMR1_t pulse_width = getPulseWidth(CCPR2, CCPR1, overflow_count);

        // The upper-bound on the pulse width is high because the first pulse
        // received is always ~500 ticks longer than the rest
        if (pulse_width > 1500 && pulse_width < 3000)
        {
            // Received a 0
            data <<= 1;
            bit_count++;
        }
        else if (pulse_width > 3500 && pulse_width < 5000)
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

            // Disable end-of-pulse interrupts to stop reading data
            CCP1IE = 0;
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

        overflow_count = 0;
        TMR1IE = 1;
        CCP1IF = 0;
    }
}
