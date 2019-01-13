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
#pragma config WDTCPS = WDTCPS1F// WDT Period Select (Software Control (WDTPS))
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config WDTCWS = WDTCWSSW// WDT Window Select (Software WDT window size control (WDTWS bits))
#pragma config WDTCCS = SWC     // WDT Input Clock Selector (Software control, controlled by WDTCS bits)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
// clang-format on

#include "error.h"
#include "i2cSlave.h"
#include "IRReceiver.h"
#include "IRTransmitter.h"
#include "realTimeClock.h"
#include "system.h"
#include "transmissionConstants.h"

#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

int main(void)
{
    configureSystem();

    // Enable interrupts (go, go, go!)
    GIE = 1;

    uint16_t loop_counter = 0;

    while (true)
    {
        irTransmitter_eventHandler();
        irReceiver_eventHandler();
        i2cSlave_eventHandler();

        uint8_t received_data_length;
        uint16_t received_data;
        if (irReceiver_tryGetTransmission(&received_data, &received_data_length))
        {
            NOP();
            // TODO send data across UART or similar
        }

        // A crude non-blocking delay system, for testing
        if (loop_counter == 0)
        {
            irTransmitter_transmitAsync(0b10001111, 8);
            loop_counter = 4000;
        }

        loop_counter--;
    }
}

// Main Interrupt Service Routine (ISR)
void __interrupt() ISR(void)
{
    irTransmitter_interruptHandler();
    irReceiver_interruptHandler();
}
