#include "i2cSlave.h"

#include "../LaserTagUtils.X/stringQueue.h"
#include "error.h"
#include "pins.h"

#include <stdbool.h>
#include <stdint.h>

#include <xc.h>

void i2cSlave_initialize()
{
    // Assign pins
    PPS_IN_REG_SCL = PPS_IN_VAL_SCL;
    PPS_IN_REG_SDA = PPS_IN_VAL_SDA;

    PPS_OUT_REG_SCL = PPS_OUT_VAL_SCL;
    PPS_OUT_REG_SDA = PPS_OUT_VAL_SDA;

    // Set SCL and SDA pins to input
    TRIS_SCL = 1;
    TRIS_SDA = 1;

    SSP1CON1bits.SSPEN = 1;
    // Select I2C Slave mode, with 7-bit address and no start/stop signal interrupts
    SSP1CON1bits.SSPM = 0b0110;  // TODO start/stop interrupts? 0b1110

    // Set our address to a randomly picked number. Shift left one because the
    // address goes in bits 1-7 of the register
    SSP1ADD = 0b1010001 << 1;
}

#define INCOMING_MESSAGE_QUEUE_LENGTH 128

uint8_t g_incoming_message_queue_storage[INCOMING_MESSAGE_QUEUE_LENGTH];
string_queue_t g_incoming_message_queue;

#define OUTGOING_MESSAGE_QUEUE_LENGTH 128

uint8_t g_outgoing_message_queue_storage[OUTGOING_MESSAGE_QUEUE_LENGTH];
string_queue_t g_outgoing_message_queue;

void i2cSlave_eventHandler(void)
{
    // Detecting and handling the SSP1 interrupt flag in a non-interrupting way,
    // as the handling code is not urgent
}
