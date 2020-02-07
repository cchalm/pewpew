#include "i2cSlave.h"

#include "../LaserTagUtils.X/queue.h"
#include "../LaserTagUtils.X/stringQueue.h"
#include "error.h"
#include "pins.h"

#include <stdbool.h>
#include <stdint.h>

#include <xc.h>

#define INCOMING_MESSAGE_QUEUE_LENGTH 128

uint8_t g_incoming_message_queue_storage[INCOMING_MESSAGE_QUEUE_LENGTH];
string_queue_t g_incoming_message_queue;

#define OUTGOING_MESSAGE_QUEUE_LENGTH 128

uint8_t g_outgoing_message_queue_storage[OUTGOING_MESSAGE_QUEUE_LENGTH];
queue_t g_outgoing_message_queue;

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
    // Select I2C Slave mode, with 7-bit address, and start/stop interrupts determined by PCIE and SCIE (off by default)
    SSP1CON1bits.SSPM = 0b0110;
    // Enable clock stretching, which allows the slave to slow down data reception/transmission if needed
    SSP1CON2bits.SEN = 1;

    // Set our address to a randomly picked number. Shift left one because the
    // address goes in bits 1-7 of the register
    SSP1ADD = 0b1010001 << 1;

    g_outgoing_message_queue = queue_create(g_outgoing_message_queue_storage, OUTGOING_MESSAGE_QUEUE_LENGTH);
    g_incoming_message_queue = stringQueue_create(g_incoming_message_queue_storage, INCOMING_MESSAGE_QUEUE_LENGTH);
}

void i2cSlave_shutdown()
{
    SSP1CON1bits.SSPEN = 0;
}

bool i2cSlave_read(uint8_t max_data_length, uint8_t* data_out, uint8_t* data_length_out)
{
    if (stringQueue_hasFullString(&g_incoming_message_queue))
    {
        return stringQueue_pop(&g_incoming_message_queue, max_data_length, data_out, data_length_out);
    }

    *data_length_out = 0;
    return false;
}

void i2cSlave_write(uint8_t* data, uint8_t data_length)
{
    for (uint8_t i = 0; i < data_length; i++)
    {
        if (!queue_push(&g_outgoing_message_queue, data[i]))
            fatal(ERROR_I2C_OUTGOING_QUEUE_FULL);
    }
}

static uint8_t getNextByteToWrite()
{
    uint8_t byte;
    if (!queue_pop(&g_outgoing_message_queue, &byte))
        // If the master asks for data and we have none, return zero
        byte = 0;

    return byte;
}

static void setReceivedByte(uint8_t data)
{
    if (!stringQueue_pushPartial(&g_incoming_message_queue, &data, 1, false))
        fatal(ERROR_I2C_INCOMING_QUEUE_FULL);
}

static void endRead()
{
    // Mark the last byte added as the end of the string
    stringQueue_pushPartial(&g_incoming_message_queue, 0, 0, true);
}

void i2cSlave_eventHandler(void)
{
    // Detecting and handling the SSP1 interrupt flag in a non-interrupting way,
    // as the handling code is not urgent
    if (!SSP1IF)
        return;

    // Immediately clear the flag. Some of the logic in this function immediately triggers asynchronous MSSP module
    // behavior that can cause the flag to be set again. If we're too slow and we clear the flag after it has
    // already been set a second time, we will miss that event
    SSP1IF = 0;

    // Bit of a hack - we need to know when each distinct transmission from the master ends. The master sends a stop
    // condition to indicate the end of a write. We could enable a stop condition interrupt, but such an interrupt would
    // give no opportunity for clock stretching, so there would be no guarantee that we would handle the stop condition
    // interrupt prior to another interrupt coming in for reception of a matching address. Instead, we remember that we
    // were receiving data. If we were receiving data and then we receive an address, we know that the last byte
    // received was the last byte in the transmission.
    static bool wasReceiving = false;

    // Poll the stop condition so that long gaps between messages don't delay ending the read.
    if (wasReceiving && SSP1STATbits.P)
    {
        endRead();
        wasReceiving = false;
    }

    if (SSP1STATbits.D_nA)
    {
        // Received or sent data

        if (SSP1STATbits.R_nW)
        {
            // Sent data

            if (SSP1CON2bits.ACKSTAT)
            {
                // Sent final byte of message
                // TODO use stringQueue for output, and verify that the last byte sent was last byte in string?
            }
            else
            {
                SSP1BUF = getNextByteToWrite();
                // Release the clock
                CKP = 1;
            }
        }
        else
        {
            // Received data

            uint8_t data = SSP1BUF;
            // Release the clock
            CKP = 1;

            setReceivedByte(data);

            wasReceiving = true;
        }
    }
    else
    {
        // Received address

        if (wasReceiving)
        {
            endRead();
            wasReceiving = false;
        }

        if (SSP1STATbits.R_nW)
        {
            // Master reading data from us
            SSP1BUF = getNextByteToWrite();
        }
        else
        {
            // Master writing data to us
            // Address is in SSP1BUF; read it to clear SSP1STATbits.BF
            SSP1BUF;
        }

        // Release the clock
        CKP = 1;
    }
}
