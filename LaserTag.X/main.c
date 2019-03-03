// clang-format off
// PIC16F1619 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable (Brown-out Reset disabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switch Over (Internal External Switch Over mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PPS1WAY = ON     // Peripheral Pin Select one-way control (The PPSLOCK bit cannot be cleared once it is set by software)
#pragma config ZCD = OFF        // Zero Cross Detect Disable Bit (ZCD disable.  ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PLLEN = ON       // PLL Enable Bit (4x PLL is always enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)

// CONFIG3
#pragma config WDTCPS = WDTCPS1F  // WDT Period Select (Software Control (WDTPS))
#pragma config WDTE = OFF         // Watchdog Timer Enable (WDT disabled)
#pragma config WDTCWS = WDTCWSSW  // WDT Window Select (Software WDT window size control (WDTWS bits))
#pragma config WDTCCS = SWC       // WDT Input Clock Selector (Software control, controlled by WDTCS bits)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
// clang-format on

#include "LEDDriver.h"
#include "LEDs.h"
#include "error.h"
#include "i2cMaster.h"
#include "irTransceiver.h"
#include "pins.h"
#include "realTimeClock.h"
#include "system.h"

#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

#define MAX_HEALTH 10
#define MAX_AMMO 10

// Shots per second
#define FIRE_RATE 2
// Delay between shots in ms
#define SHOT_DELAY_MS 1000 / (FIRE_RATE)
#define FULL_AUTO true

// Minimum delay, in ms, from trigger falling edge to rising edge for a shot to
// be registered. This is to mitigate button bounce.
#define BOUNCE_DELAY 50

#define ERROR_IF_RECEIVED_DOES_NOT_MATCH_SENT
#undef COUNT_DROPPED_TRANSMISSIONS
#undef DISPLAY_DROP_COUNT
#define DISPLAY_RECEIVED_DATA
#undef RANDOMIZE_SHOT_DELAY
#define SEND_RANDOM_DATA

#ifdef RANDOMIZE_SHOT_DELAY
#define MIN_SHOT_DELAY_MS 10
#define MAX_SHOT_DELAY_MS 500
#endif

// The time, in the form of a millisecond count corresponding to ms_counter,
// at which we can shoot again
uint32_t g_shot_enable_ms_count;
volatile bool g_can_shoot;

uint8_t g_shot_data_to_send;

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

static void testLEDDriver()
{
    // Reset all registers. No surprises
    LEDDriver_reset();

    {
        uint8_t data[36] = {
            255,  // Bar: Red
            255,  // Bar: Red
            255,  // Bar: Red
            255,  // Bar: Green
            255,  // Bar: Green
            255,  // Bar: Green
            255,  // Bar: Green
            255,  // Bar: Green
            255,  // Bar: Green
            255,  // Bar: Green
            20,   // Bar: Blue
            20,   // Bar: Blue
            20,   // Bar: Blue
            20,   // Bar: Blue
            20,   // Bar: Blue
            20,   // Bar: Blue
            20,   // Bar: Blue
            20,   // Bar: Blue
            20,   // Bar: Blue
            20,   // Bar: Blue
            255,  // RGB: Red
            213,  // RGB: Green
            213,  // RGB: Blue
            255,  // RGB: Red
            213,  // RGB: Green
            213,  // RGB: Blue
            255,  // RGB: Red
            213,  // RGB: Green
            213,  // RGB: Blue
            255,  // RGB: Red
            213,  // RGB: Green
            213,  // RGB: Blue
            255,  // RGB: Red
            213,  // RGB: Green
            213,  // RGB: Blue
            0     // NONE
        };

        LEDDriver_setPWM(0, data, 36);
    }
    {
        uint8_t data[36] = {
            0b001,  // Bar: Red
            0b001,  // Bar: Red
            0b001,  // Bar: Red
            0b011,  // Bar: Green
            0b011,  // Bar: Green
            0b011,  // Bar: Green
            0b011,  // Bar: Green
            0b011,  // Bar: Green
            0b011,  // Bar: Green
            0b011,  // Bar: Green
            0b111,  // Bar: Blue
            0b111,  // Bar: Blue
            0b111,  // Bar: Blue
            0b111,  // Bar: Blue
            0b111,  // Bar: Blue
            0b111,  // Bar: Blue
            0b111,  // Bar: Blue
            0b111,  // Bar: Blue
            0b111,  // Bar: Blue
            0b111,  // Bar: Blue
            0b110,  // RGB: Red
            0b110,  // RGB: Green
            0b111,  // RGB: Blue
            0b110,  // RGB: Red
            0b110,  // RGB: Green
            0b110,  // RGB: Blue
            0b111,  // RGB: Red
            0b110,  // RGB: Green
            0b111,  // RGB: Blue
            0b110,  // RGB: Red
            0b110,  // RGB: Green
            0b110,  // RGB: Blue
            0b110,  // RGB: Red
            0b110,  // RGB: Green
            0b110,  // RGB: Blue
            0       // NONE
        };

        LEDDriver_setControl(0, data, 36);
    }

    LEDDriver_flushChanges();
    LEDDriver_setShutdown(false);

    i2cMaster_flushQueue();
}

static uint32_t getShotDelay()
{
#ifdef RANDOMIZE_SHOT_DELAY
    // Rand is too slow, so sweep through the range instead
    static uint16_t delay = MIN_SHOT_DELAY_MS;
    static uint8_t inc = 1;
    uint16_t delay_to_return = delay;
    delay += inc;
    if (delay > MAX_SHOT_DELAY_MS)
        delay = MIN_SHOT_DELAY_MS;
    inc++;
    return delay_to_return;
#else
    return SHOT_DELAY_MS;
#endif
}

int main(void)
{
    configureSystem();

    testLEDDriver();

    for (unsigned char i = 0; i < 10; i++)
    {
        setBarDisplay1(1 << i);
        setBarDisplay2(1 << i);
        i2cMaster_flushQueue();
        delay(100);
    }

    setBarDisplay1(0);
    setBarDisplay2(0);
    i2cMaster_flushQueue();

    flashMuzzleLight();
    delay(400);
    flashHitLight();
    delay(400);

    setBarDisplay1(0b1111111111);
    setBarDisplay2(0b1111111111);
    i2cMaster_flushQueue();

    g_can_shoot = true;
    g_shot_enable_ms_count = 0;

    g_shot_data_to_send = 0b01010100;

    int16_t health = MAX_HEALTH;
    int16_t ammo = MAX_AMMO;

    // Set the "was" variables for use on the first loop iteration
    uint16_t input_state = INPUT_PORT;
    bool trigger_was_pressed = ((input_state >> PORT_INDEX_TRIGGER) & 1) == TRIGGER_PRESSED;
    bool mag_was_out = ((input_state >> PORT_INDEX_RELOAD) & 1) == MAG_OUT;

    // Enable interrupts (go, go, go!)
    GIE = 1;

    while (true)
    {
        i2cMaster_eventHandler();
        irTransceiver_eventHandler();

        uint8_t received_data;
        if (irTransceiver_receive8WithCRC(&received_data))
        {
#ifdef DISPLAY_RECEIVED_DATA
            setBarDisplay1(received_data);
#endif

            // Register a hit if we received a shot not from ourselves
            if (received_data != g_shot_data_to_send)
            {
                flashHitLight();
                health--;

                if (health == 0)
                {
                    fatal(0b0000110000);
                }

                setHealthDisplay(health);
            }

#ifdef COUNT_DROPPED_TRANSMISSIONS
            g_num_shots_received++;
#endif

#ifdef ERROR_IF_RECEIVED_DOES_NOT_MATCH_SENT
            if (received_data != g_shot_data_to_send)
                fatal(ERROR_INVALID_SHOT_DATA_RECEIVED);
#endif
        }

        if (getMillisecondCount() >= g_shot_enable_ms_count)
        {
            g_can_shoot = true;
        }

        // Snapshot input state. This way, we don't have to worry about the
        // inputs changing while we're performing logic.
        input_state = INPUT_PORT;
        bool trigger_pressed = ((input_state >> PORT_INDEX_TRIGGER) & 1) == TRIGGER_PRESSED;
        bool mag_in = ((input_state >> PORT_INDEX_RELOAD) & 1) == MAG_IN;

        if (trigger_pressed && (FULL_AUTO || !trigger_was_pressed) && g_can_shoot)
        {
            // The trigger is pressed, it wasn't pressed before, and the shot
            // timer has elapsed; try to shoot

            // Make sure we have ammo and the magazine is in
            if (mag_in && ammo)
            {
                shoot();
            }

            // Putting the following lines outside the ammo conditional means that you can only get a "not enough ammo"
            // signal, like a sound, at the rate of fire of the tagger, not as fast as you can press the trigger

            // Indicate that a shot has just occurred and store the time at which we can shoot again
            g_shot_enable_ms_count = getMillisecondCount() + getShotDelay();

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

        // if (!mag_in)
        //    setHealthDisplay(0);

        if (mag_in && mag_was_out)
        {
            // Mag has been put back in, reload
            ammo = MAX_AMMO;
            // setHealthDisplay(ammo);
        }

        // Update "was" variables
        trigger_was_pressed = trigger_pressed;
        mag_was_out = !mag_in;
    }
}

// Main Interrupt Service Routine (ISR)
void __interrupt() ISR(void)
{
    rtcTimerInterruptHandler();
}
void shoot(void)
{
#ifdef SEND_RANDOM_DATA
    g_shot_data_to_send = (g_shot_data_to_send >> 1) | ((TMR6 & 1) << 7);
#endif

    irTransceiver_transmit8WithCRC(g_shot_data_to_send);

#ifdef COUNT_DROPPED_TRANSMISSIONS
#ifdef DISPLAY_DROP_COUNT
    uint16_t num_shots_missed = g_num_shots_sent - g_num_shots_received;
    setBarDisplay1(num_shots_missed);
#endif
    g_num_shots_sent++;
#endif

    flashMuzzleLight();
    // ammo--;
    // setBarDisplay1(shot_data_to_send);
}
