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

#include "IRReceiver.h"
#include "IRTransmitter.h"
#include "LEDDisplay.h"
#include "realTimeClock.h"
#include "system.h"
#include "transmissionConstants.h"

#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

#define MAX_HEALTH 10
#define MAX_AMMO 10

// Shots per second
#define FIRE_RATE 70
// Delay between shots in ms
#define SHOT_DELAY_MS 1000/(FIRE_RATE)
#define FULL_AUTO true

// Minimum delay, in ms, from trigger falling edge to rising edge for a shot to
// be registered. This is to mitigate button bounce.
#define BOUNCE_DELAY 2

#define ERROR_IF_RECEIVED_DOES_NOT_MATCH_SENT
#define COUNT_DROPPED_TRANSMISSIONS
#define DISPLAY_DROP_COUNT
#define RANDOMIZE_SHOT_DELAY
#define EVALUATE_CONSTANTS

#ifdef RANDOMIZE_SHOT_DELAY
#define MIN_SHOT_DELAY_MS 50
#define MAX_SHOT_DELAY_MS 500
#endif

#undef DISPLAY_RECEIVED_DATA

enum
{
    INVALID_SHOT_DATA_RECEIVED = 0b1001,
    TRANSMISSION_OVERLAP = 0b100101
};

// The time, in the form of a millisecond count corresponding to ms_counter,
// at which we can shoot again
count_t g_shot_enable_ms_count;
volatile bool g_can_shoot;

uint16_t g_shot_data_to_send;

void setHealthDisplay(uint8_t value);
void shoot(void);
void flashMuzzleLight(void);
void flashHitLight(void);

void handleTimingInterrupt(void);
void handleShotReceptionInterrupt(void);

void handleShotReceived(uint16_t shot_data_received);

#ifdef COUNT_DROPPED_TRANSMISSIONS
uint32_t g_num_shots_sent = 0;
uint32_t g_num_shots_received = 0;
#endif

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

    int16_t health = MAX_HEALTH;
    int16_t ammo = MAX_AMMO;
    setHealthDisplay(ammo);

    // Set the "was" variables for use on the first loop iteration
    uint16_t input_state = INPUT_PORT;
    bool trigger_was_pressed = ((input_state >> TRIGGER_OFFSET) & 1) == TRIGGER_PRESSED;
    bool mag_was_out = ((input_state >> RELOAD_OFFSET) & 1) == MAG_OUT;

    // Enable interrupts (go, go, go!)
    GIE = 1;

    while(true)
    {
        uint16_t received_data;
        if (tryGetTransmissionData(&received_data))
        {
#ifdef DISPLAY_RECEIVED_DATA
            setLEDDisplay(received_data);
#endif

            flashHitLight();

#ifdef COUNT_DROPPED_TRANSMISSIONS
            g_num_shots_received++;
#endif

#ifdef ERROR_IF_RECEIVED_DOES_NOT_MATCH_SENT
            if (received_data != g_shot_data_to_send)
                error(INVALID_SHOT_DATA_RECEIVED);
#endif
        }

        // Assumes that we hit this function at least once per millisecond
        if (getMillisecondCount() == g_shot_enable_ms_count)
        {
            g_can_shoot = true;
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
#ifdef RANDOMIZE_SHOT_DELAY
            int16_t randDelay = (abs(rand()) % (MAX_SHOT_DELAY_MS - MIN_SHOT_DELAY_MS + 1)) + MIN_SHOT_DELAY_MS;
            g_shot_enable_ms_count = getMillisecondCount() + randDelay;
#else
            g_shot_enable_ms_count = getMillisecondCount() + SHOT_DELAY_MS;
#endif

            g_can_shoot = false;
        }
        else if (!FULL_AUTO)
        {
            // When in semi-auto mode, only enable shot a short time after
            // releasing the trigger. This is to prevent button bounces from
            // causing shots when releasing the trigger
            if (trigger_was_pressed && (g_shot_enable_ms_count - getMillisecondCount() < BOUNCE_DELAY))
            {
                // Trigger was pressed and now isn't - the trigger was released
                g_can_shoot = false;
                g_shot_enable_ms_count = getMillisecondCount() + BOUNCE_DELAY;
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
    handleRTCTimerInterrupt();
    handleTransmissionTimingInterrupt();
    handleSignalReceptionInterrupt();
}

void setHealthDisplay(uint8_t value)
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
    
#ifdef COUNT_DROPPED_TRANSMISSIONS
#ifdef DISPLAY_DROP_COUNT
    uint16_t num_shots_missed = g_num_shots_sent - g_num_shots_received;
    setLEDDisplay(num_shots_missed);
#endif
    g_num_shots_sent++;
#endif

    flashMuzzleLight();
    //ammo--;
    //setLEDDisplay(shot_data_to_send);
}

void flashMuzzleLight(void)
{
    PIN_MUZZLE_FLASH = 0;
    NOP();
    PIN_MUZZLE_FLASH = 1;
}

void flashHitLight(void)
{
    PIN_HIT_LIGHT = 0;
    NOP();
    PIN_HIT_LIGHT = 1;
}
