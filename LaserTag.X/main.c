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
#define SHOT_DELAY 1000/FIRE_RATE

#define SHOT_DATA_LENGTH 8

// Minimum delay, in ms, from trigger falling edge to rising edge for a shot to
// be registered. This is to prevent unintended shots from button bounce
#define BOUNCE_DELAY 2

int health;
int ammo;

// Counts seconds. Overflows to 0 after ~18 hours (1092.25 minutes)
volatile count_t s_counter;
// Counts milliseconds. Overflows to 0 after ~1 minute (65.535 seconds)
volatile count_t ms_counter;

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

void setHealthDisplay(int value);
void shoot(void);
void flash(void);

volatile count_t TEST_mess_up_shot;

int main(void) {
    configureSystem();

    LATA = 0b111111;
    LATD = 0b1111;
    LATB = 0b111111;

    can_shoot = TRUE;
    shot_enable_ms_count = 0;

    shot_data_to_send = 0b01010101;
    shot_requested = FALSE;

    health = MAX_HEALTH;
    ammo = MAX_AMMO;
    setHealthDisplay(ammo);

    input_state = INPUT_PORT;
    trigger_was_pressed = PIN_TRIGGER == TRIGGER_PRESSED;
    mag_was_out = PIN_RELOAD == MAG_OUT;
    
    PIN_SHOT_LIGHT = LOW;

    ms_counter = 0;
    s_counter = 0;
    
    TEST_mess_up_shot = 1;

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

            shot_received = FALSE;
        }
        
        // Snapshot input state. This way, we don't have to worry about the
        // inputs changing while we're performing logic.
        input_state = INPUT_PORT;
        trigger_pressed = (input_state >> TRIGGER_OFFSET) & 1 == TRIGGER_PRESSED;
        mag_in = (input_state >> RELOAD_OFFSET) & 1 == MAG_IN;

        // If the trigger is pressed, it wasn't pressed before, and the shot
        // timer has elapsed, try to shoot
        if (/*trigger_pressed && !trigger_was_pressed &&*/ can_shoot)
        {
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
            shot_enable_ms_count = ms_counter + SHOT_DELAY*5;
            can_shoot = FALSE;
        }
        else // Trigger not pressed
        {/*
            // Only enable shot a short time after releasing the trigger. This
            // is to prevent button bounces from causing shots when releasing
            // the trigger
            if (trigger_was_pressed && (shot_enable_ms_count - ms_counter < BOUNCE_DELAY))
            {
                // Trigger was pressed and now isn't - the trigger was released
                can_shoot = FALSE;
                shot_enable_ms_count = ms_counter + BOUNCE_DELAY;
            }*/
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
    if (TMR0IF)
    {
        // Real-time timer logic

        static count_t next_s = 1000;
        
        ms_counter++;

        if (ms_counter == next_s)
        {
            s_counter++;
            next_s = ms_counter + 1000;
        }

        if (ms_counter == shot_enable_ms_count)
        {
            can_shoot = TRUE;
        }

        // Preload timer
        TMR0 = TMR0_PRELOAD;
        // Reset flag
        TMR0IF = 0;
    }
    else if (CCP3IF && CCP3IE)
    {
        // Shooting logic

        static unsigned char shot_data_index = 0;
        static bool_t next_edge_rising = TRUE;

        if (next_edge_rising)
        {
            PIN_SHOT_LIGHT = HIGH;
            TMR1_t pulse_width = ((shot_data_to_send >> shot_data_index) & 1) ?
                ONE_PULSE_WIDTH : ZERO_PULSE_WIDTH;
            CCPR3 = TMR1 + pulse_width;
            shot_data_index++;
            next_edge_rising = FALSE;
        }
        else // !next_edge_rising
        {
            PIN_SHOT_LIGHT = LOW;
            next_edge_rising = TRUE;

            if (shot_data_index == SHOT_DATA_LENGTH)
            {
                // This is the final falling edge of the shot, reset the data
                // index and disable CCP3 interrupts
                shot_data_index = 0;
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
    else
    {
        // Shot reception logic

        static bool_t wait_for_silence = FALSE;
        // Counts timer1 overflows, capped at 2
        static unsigned char overflow_counter;

        if (TMR1IF && TMR1IE)
        {
            if (overflow_counter == 2)
            {
                // We're going to stop counting at 2, so we might as well turn
                // off timer1 interrupts.
                TMR1IE = 0;
            }
            else
            {
                overflow_counter++;
            }

            // Reset flag
            TMR1IF = 0;
        }
        else if (CCP2IF && CCP2IE)
        {
            // Falling edge detected (start of pulse, end of silence)

            /*
             * A period of silence just ended. We need to see if the length of
             * the silence was at least double the length of the gap between
             * pulses in a transmission. There are three cases to check (four if
             * you consider case 3 to be two cases).
             *
             * Case 1: overflow_counter == 2
             *  Timer1 has overflowed at least twice during this period of
             *  silence. This means at least one full timer cycle has elapsed
             *  (one overflow for the beginning of the cycle, one for the end).
             *  The pulse gap is guaranteed to be less than half of the time it
             *  takes timer1 to go through a full cycle, so this period of
             *  silence is guaranteed to be at least double the pulse gap.
             *
             * Case 2: overflow_counter == 1 && CCPR2 >= CCPR1
             *  Timer1 has overflowed exactly once during this period of
             *  silence, and the time (in terms of timer1 ticks) of the end of
             *  the silence is greater than or equal to the time of the start.
             *  The silence started in some timer cycle, and ended at the same
             *  time or later in the next timer cycle, so more than one timer
             *  cycle has elapsed. For the reasons mentioned in case 1, this
             *  period of silence is guaranteed to be at least double the pulse
             *  gap.
             *
             * Case 3: (TMR1_t)(CCPR2 - CCPR1) >= PULSE_GAP_WIDTH * 2
             *  If overflow_counter == 1, we know CCPR2 < CCPR1 because
             *  otherwise we would have gone with case 2. The result of the
             *  subtraction will underflow when cast and produce the correct
             *  value for elapsed ticks (you can verify this with a simple
             *  number line). If overflow_counter == 0, CCPR2 will be greater
             *  than CCPR1 and this is a simple subtraction to get elapsed
             *  ticks.
             */

            if ( (overflow_counter == 2) ||
                 (overflow_counter == 1 && CCPR2 >= CCPR1) ||
                 ((TMR1_t)(CCPR2 - CCPR1) >= PULSE_GAP_WIDTH * 2) )
            {
                wait_for_silence = FALSE;
                // Disable interrupts on rising edges
                CCP2IE = 0;
                // Disable timer1 overflow interrupts
                TMR1IE = 0;
            }
            
            // Reset flag
            CCP2IF = 0;
        }
        else if (CCP1IF)
        {
            // Rising edge detected (end of pulse, start of silence)

            if (wait_for_silence)
            {
                overflow_counter = 0;
            }
            else
            {
                static unsigned char bit_count = 0;
                static unsigned char data = 0;

                TMR1_t pulse_width = CCPR1 - CCPR2;

                if (pulse_width > 2500 && pulse_width < 4000)
                {
                    // Received a 0
                    data <<= 1;
                    bit_count++;
                }
                else if (pulse_width > 4500 && pulse_width < 6000)
                {
                    // Received a 1
                    data = (data << 1) | 1;
                    bit_count++;
                }
                else
                {
                    // Invalid pulse width. Something has gone wrong, so we're
                    // going to stop reading in data and wait until we see a
                    // period of silence at least twice the length of a pulse
                    // gap before starting again.

                    overflow_counter = 0;
                    wait_for_silence = TRUE;

                    // Enable interrupts on rising edges
                    CCP2IF = 0;
                    CCP2IE = 1;
                    // Enable timer1 overflow interrupts
                    TMR1IF = 0;
                    TMR1IE = 1;

                    data = 0;
                    bit_count = 0;
                }

                if (bit_count == SHOT_DATA_LENGTH)
                {
                    // We've received an entire shot
                    shot_data_received = data;
                    shot_received = TRUE;

                    bit_count = 0;
                    data = 0;
                }
            }
            
            // Reset flag
            CCP1IF = 0;
        }
    }
}

void setHealthDisplay(int value)
{
    // Shift in zeros from the right
    setLEDDisplay( 0b1111111111 >> value );
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
    //ammo--;
    setHealthDisplay(ammo);
}

void flash(void)
{
    PIN_MUZZLE_FLASH = 0;
    NOP();
    PIN_MUZZLE_FLASH = 1;
}
