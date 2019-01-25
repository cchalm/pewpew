#include "i2cMaster.h"

#include "../LaserTagUtils.X/keyedStringQueue.h"
#include "../LaserTagUtils.X/stringQueue.h"
#include "error.h"
#include "pins.h"

#include <stdbool.h>
#include <stdint.h>

#include <xc.h>

#define OUTGOING_MESSAGE_QUEUE_LENGTH 128

uint8_t g_outgoing_message_queue_storage[OUTGOING_MESSAGE_QUEUE_LENGTH];
string_queue_t g_outgoing_message_queue;
bool g_outgoing_message_in_progress = false;

#define INCOMING_MESSAGE_QUEUE_LENGTH 128

uint8_t g_incoming_message_queue_storage[INCOMING_MESSAGE_QUEUE_LENGTH];
string_queue_t g_incoming_message_queue;

void i2cMaster_initialize()
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
    // Select I2C Master mode
    SSP1CON1bits.SSPM = 0b1000;

    // Clock divider. For I2C, ADD must be 3 or greater, according to
    // http://ww1.microchip.com/downloads/en/DeviceDoc/40001770D.pdf
    // f = Fosc / ((ADD + 1) * 4)
    // With ADD == 3 and Fosc == 32MHz, f == 2MHz
    // Max clock 400kHz, according to http://www.issi.com/WW/pdf/31FL3236.pdf
    // ADD = (Fosc / (f * 4)) - 1
    // With f == 400kHz and Fosc == 32MHz, ADD == 19
    SSP1ADD = 19;

    // Enable slew rate control for 400kHz mode
    SSP1STATbits.SMP = 0;

    g_outgoing_message_queue = stringQueue_create(g_outgoing_message_queue_storage, OUTGOING_MESSAGE_QUEUE_LENGTH);
    g_incoming_message_queue = stringQueue_create(g_incoming_message_queue_storage, INCOMING_MESSAGE_QUEUE_LENGTH);
}

void i2cMaster_write(uint8_t address, uint8_t* data, uint8_t data_length)
{
    i2cMaster_writePartial(address, data, data_length, true);
}

void i2cMaster_writePartial(uint8_t address, uint8_t* data, uint8_t data_length, bool is_last_part)
{
    // Stores the address of the last partial write
    static uint8_t current_address = 0;
    static bool transmission_partially_written = false;

    if (!transmission_partially_written)
    {
        current_address = address;
        // The 7-bit address must sit in the most significant bits, with the LSB for the R/W bit
        address <<= 1;
        if (!stringQueue_pushPartial(&g_outgoing_message_queue, &address, 1, false))
            fatal(ERROR_I2C_OUTGOING_QUEUE_FULL);

        transmission_partially_written = true;
    }
    else
    {
        // Each partial write of a transmission must use the same address
        if (address != current_address)
            fatal(ERROR_I2C_PARTIAL_WRITE_ADDRESS_MISMATCH);
    }

    if (!stringQueue_pushPartial(&g_outgoing_message_queue, data, data_length, is_last_part))
        fatal(ERROR_I2C_OUTGOING_QUEUE_FULL);

    if (is_last_part)
        transmission_partially_written = false;
}

void i2cMaster_read(uint8_t address, uint8_t read_length)
{
    // The 7-bit address sits in the most significant bits, with the LSB for the R/W bit
    address <<= 1;
    // Set the Read bit
    address |= 1;
    if (!stringQueue_pushPartial(&g_outgoing_message_queue, &address, 1, false))
        fatal(ERROR_I2C_OUTGOING_QUEUE_FULL);
    // Push the length into the queue as the second byte of the message. We will use this byte to determine when to stop
    // asking for more bytes from the slave
    if (!stringQueue_pushPartial(&g_outgoing_message_queue, &read_length, 1, true))
        fatal(ERROR_I2C_OUTGOING_QUEUE_FULL);
}

bool i2cMaster_getReadResults(uint8_t address, uint8_t max_length, uint8_t* data_out, uint8_t* length_out)
{
    if (!keyedStringQueue_hasFullString(&g_incoming_message_queue, address))
    {
        *length_out = 0;
        return false;
    }

    return keyedStringQueue_pop(&g_incoming_message_queue, address, max_length, data_out, length_out);
}

static bool isOutgoingQueueEmpty()
{
    return !stringQueue_hasFullString(&g_outgoing_message_queue);
}

typedef enum {
    I2C_STATE_IDLE,
    I2C_STATE_START,            // Transmitted start or repeated start signal, ready to transmit data
    I2C_STATE_ACK_RECEIVED,     // Transmitted data and received ACK or NACK
    I2C_STATE_BYTE_RECEIVED,    // Byte clocked in from slave, ready to be read in software
    I2C_STATE_ACK_TRANSMITTED,  // ACK or NACK transmitted, ready to receive next byte
    I2C_STATE_STOP              // Transmitted stop signal
} i2cModuleState_t;

i2cModuleState_t g_i2c_module_state = I2C_STATE_IDLE;
// Non-zero when reading from slave, zero when writing to slave
uint8_t g_read_length = 0;

void i2cMaster_flushQueue()
{
    // Continue until the queue is empty AND the module state is idle. If we stop as soon as the message queue is empty,
    // we will not send the stop bit for the final byte
    while (!(g_i2c_module_state == I2C_STATE_IDLE && isOutgoingQueueEmpty()))
        i2cMaster_eventHandler();
}

static void stateChange_idle()
{
    g_i2c_module_state = I2C_STATE_IDLE;
}
static void stateChange_start()
{
    SSP1CON2bits.SEN = 1;
    g_i2c_module_state = I2C_STATE_START;
}

static void stateChange_restart()
{
    SSP1CON2bits.RSEN = 1;
    g_i2c_module_state = I2C_STATE_START;
}

static void stateChange_stop()
{
    SSP1CON2bits.PEN = 1;
    g_i2c_module_state = I2C_STATE_STOP;
}

static void stateChange_writeNextByteToBuffer(bool isAddress)
{
    uint8_t next_byte;
    uint8_t out_length;
    bool next_byte_is_last = stringQueue_pop(&g_outgoing_message_queue, 1, &next_byte, &out_length);
    SSP1BUF = next_byte;
    g_outgoing_message_in_progress = !next_byte_is_last;

    if (WCOL == 1)
    {
        fatal(ERROR_I2C_BUFFER_CONTENTION);
    }

    if (isAddress)
    {
        // Record whether we're reading from the address or writing to it, indicated by the LSB of the address
        bool read = ((next_byte & 1) != 0);

        if (read)
        {
            // Pop the read length from the outgoing message queue
            bool is_last = stringQueue_pop(&g_outgoing_message_queue, 1, &g_read_length, &out_length);
            if (!is_last)
                fatal(0);
            // Shift off the R/W bit appended to the address
            uint8_t addr = next_byte >> 1;
            // Write the address for the read to the incoming message queue
            if (!stringQueue_pushPartial(&g_incoming_message_queue, &addr, 1, false))
                fatal(ERROR_I2C_INCOMING_QUEUE_FULL);
        }
    }

    g_i2c_module_state = I2C_STATE_ACK_RECEIVED;
}

static void stateChange_prepRead()
{
    RCEN = 1;

    g_i2c_module_state = I2C_STATE_BYTE_RECEIVED;
}

static void stateChange_readByteFromBuffer()
{
    g_read_length--;
    bool is_last_byte = (g_read_length == 0);

    uint8_t byte = SSP1BUF;
    if (!stringQueue_pushPartial(&g_incoming_message_queue, &byte, 1, is_last_byte))
        fatal(ERROR_I2C_INCOMING_QUEUE_FULL);

    if (is_last_byte)
    {
        ACKDT = 1;
    }
    else
    {
        ACKDT = 0;
    }
    ACKEN = 1;

    g_i2c_module_state = I2C_STATE_ACK_TRANSMITTED;
}

void i2cMaster_eventHandler(void)
{
    // Detecting and handling the SSP1 interrupt flag in a non-interrupting way,
    // as the handling code is not urgent

    if (SSP1IF)
    {
        // Immediately clear the flag. Some of the logic in this function immediately triggers asynchronous MSSP module
        // behavior that can cause the flag to be set again. If we're too slow and we clear the flag after it has
        // already been set a second time, we will miss that event
        SSP1IF = 0;

        // Five possible reasons the interrupt flag is set:
        // - Start condition detected
        // - Stop condition detected
        // - Data transfer byte transmitted/received (slave only?)
        // - Acknowledge transmitted/received
        // - Repeated Start generated

        switch (g_i2c_module_state)
        {
            case I2C_STATE_START:
            {
                if (isOutgoingQueueEmpty())
                    fatal(ERROR_I2C_OUTGOING_QUEUE_EMPTY);

                stateChange_writeNextByteToBuffer(true);

                break;
            }

            case I2C_STATE_STOP:
            {
                stateChange_idle();

                break;
            }

            case I2C_STATE_ACK_RECEIVED:
            {
                if (SSP1CON2bits.ACKSTAT)
                    fatal(ERROR_I2C_NO_ACK);

                if (g_read_length != 0)
                {
                    // Address acknowledged by slave, prepare to receive a byte
                    stateChange_prepRead();
                }
                else
                {
                    if (isOutgoingQueueEmpty())
                        stateChange_stop();
                    else if (!g_outgoing_message_in_progress)
                        // stateChange_restart();
                        stateChange_stop();
                    else
                        stateChange_writeNextByteToBuffer(false);
                }

                break;
            }

            case I2C_STATE_BYTE_RECEIVED:
            {
                stateChange_readByteFromBuffer();

                break;
            }

            case I2C_STATE_ACK_TRANSMITTED:
            {
                if (g_read_length != 0)
                    stateChange_prepRead();
                else if (isOutgoingQueueEmpty())
                    stateChange_stop();
                else
                    // stateChange_restart();
                    stateChange_stop();

                break;
            }

            default:
                fatal(ERROR_I2C_UNEXPECTED_STATE);
        }
    }
    else if (g_i2c_module_state == I2C_STATE_IDLE && !isOutgoingQueueEmpty())
    {
        stateChange_start();
    }
}
