#include "i2cMaster.h"

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
}

void i2cMaster_transmit(uint8_t address, uint8_t* data, uint8_t data_len)
{
    if (!stringQueue_pushPartial(&g_outgoing_message_queue, &address, 1, false))
        fatal(ERROR_I2C_OUTGOING_QUEUE_FULL);
    if (!stringQueue_pushPartial(&g_outgoing_message_queue, data, data_len, true))
        fatal(ERROR_I2C_OUTGOING_QUEUE_FULL);
}

static bool isQueueEmpty()
{
    return !stringQueue_hasFullString(&g_outgoing_message_queue);
}

typedef enum {
    I2C_STATE_IDLE,
    I2C_STATE_START,  // Transmitted start or repeated start signal, ready to transmit data
    I2C_STATE_ACK,    // Transmitted data and received ACK or NACK
    I2C_STATE_STOP    // Transmitted stop signal
} i2cModuleState_t;

i2cModuleState_t g_i2c_module_state = I2C_STATE_IDLE;

void i2cMaster_flushQueue()
{
    // Continue until the queue is empty AND the module state is idle. If we stop as soon as the message queue is empty,
    // we will not send the stop bit for the final byte
    while (!(g_i2c_module_state == I2C_STATE_IDLE && isQueueEmpty()))
        i2cMaster_eventHandler();
}

static void stateChange_idle()
{
    g_i2c_module_state = I2C_STATE_IDLE;
}
static void stateChange_startTransmission()
{
    SSP1CON2bits.SEN = 1;
    g_i2c_module_state = I2C_STATE_START;
}

static void stateChange_startAnotherTransmission()
{
    SSP1CON2bits.RSEN = 1;
    g_i2c_module_state = I2C_STATE_START;
}

static void stateChange_stopTransmission()
{
    SSP1CON2bits.PEN = 1;
    g_i2c_module_state = I2C_STATE_STOP;
}

static void stateChange_writeNextByteToBuffer()
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

    g_i2c_module_state = I2C_STATE_ACK;
}

void i2cMaster_eventHandler(void)
{
    // Detecting and handling the SSP1 interrupt flag in a non-interrupting way,
    // as the handling code is not urgent

    if (SSP1IF)
    {
        // Five possible reasons the interrupt flag is set:
        // - Start condition detected
        // - Stop condition detected
        // - Data transfer byte transmitted/received (slave only?)
        // - Acknowledge transmitted/received
        // - Repeated Start generated

        if (g_i2c_module_state == I2C_STATE_STOP)
        {
            stateChange_idle();
        }
        else if (g_i2c_module_state == I2C_STATE_ACK)
        {
            if (SSP1CON2bits.ACKSTAT)
                fatal(ERROR_I2C_NO_ACK);

            if (isQueueEmpty())
                stateChange_stopTransmission();
            else if (!g_outgoing_message_in_progress)
                // stateChange_startAnotherTransmission();
                stateChange_stopTransmission();
            else
                stateChange_writeNextByteToBuffer();
        }
        else if (g_i2c_module_state == I2C_STATE_START)
        {
            if (isQueueEmpty())
                fatal(ERROR_I2C_OUTGOING_QUEUE_EMPTY);

            stateChange_writeNextByteToBuffer();
        }
        else
        {
            fatal(ERROR_I2C_UNEXPECTED_STATE);
        }

        SSP1IF = 0;
    }
    else if (g_i2c_module_state == I2C_STATE_IDLE && !isQueueEmpty())
    {
        stateChange_startTransmission();
    }
}
