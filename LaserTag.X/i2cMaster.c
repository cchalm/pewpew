#include "i2cMaster.h"

#include "error.h"
#include "pins.h"

#include <stdbool.h>
#include <stdint.h>

#include <xc.h>

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
    
    // Clock divider. For I2C, ADD must be 3 or greater, according to http://ww1.microchip.com/downloads/en/DeviceDoc/40001770D.pdf
    // f = Fosc / ((ADD + 1) * 4)
    // With ADD == 3 and Fosc == 32MHz, f == 2MHz
    // Max clock 400kHz, according to http://www.issi.com/WW/pdf/31FL3236.pdf
    // ADD = (Fosc / (f * 4)) - 1
    // With f == 400kHz and Fosc == 32MHz, ADD == 19
    SSP1ADD = 19;

    // Enable slew rate control for 400kHz mode
    SSP1STATbits.SMP = 0;
}

#define OUTGOING_DATA_QUEUE_LENGTH 128

uint8_t g_outgoing_data_queue[OUTGOING_DATA_QUEUE_LENGTH];
uint8_t g_outgoing_data_queue_index = 0;
// Exclusive end index of the data currently in the queue
uint8_t g_outgoing_data_queue_end_index = 0;
// A bitfield with '1's at the indexes of bytes in the data queue that are
// addresses
uint8_t g_outgoing_data_address_indexes[OUTGOING_DATA_QUEUE_LENGTH / 8];

static uint8_t increment(uint8_t index)
{
    if (index == OUTGOING_DATA_QUEUE_LENGTH - 1)
        return 0;

    return index + 1;
}

static uint8_t decrement(uint8_t index)
{
    if (index == 0)
        return OUTGOING_DATA_QUEUE_LENGTH - 1;

    return index - 1;
}

static void setIsAddress(uint8_t index, bool isAddress)
{
    if (isAddress)
        g_outgoing_data_address_indexes[index / 8] |= (1 << (index % 8));
    else
        g_outgoing_data_address_indexes[index / 8] &= ~(1 << (index % 8));
}

static bool isAddress(uint8_t index)
{
    return (g_outgoing_data_address_indexes[index / 8] & (1 << (index % 8))) != 0;
}

static bool isNextByteAddress()
{
    return isAddress(g_outgoing_data_queue_index);
}

static void incrementEndIndex()
{
    g_outgoing_data_queue_end_index = increment(g_outgoing_data_queue_end_index);
    if (g_outgoing_data_queue_end_index == g_outgoing_data_queue_index)
        fatal(ERROR_I2C_OUTGOING_QUEUE_FULL);
}

static void incrementIndex()
{
    g_outgoing_data_queue_index = increment(g_outgoing_data_queue_index);
}

static void pushData(uint8_t data, bool isAddress)
{
    setIsAddress(g_outgoing_data_queue_end_index, isAddress);

    g_outgoing_data_queue[g_outgoing_data_queue_end_index] = data;
    incrementEndIndex();
}

void i2cMaster_transmit(uint8_t address, uint8_t* data, uint8_t data_len)
{
    pushData(address, true);
    for (int i = 0; i < data_len; i++)
    {
        pushData(data[i], false);
    }
}

static bool isQueueEmpty()
{
    return g_outgoing_data_queue_index == g_outgoing_data_queue_end_index;
}

typedef enum
{
    I2C_STATE_IDLE,
    I2C_STATE_START,            // Transmitted start or repeated start signal, ready to transmit data
    I2C_STATE_ACK,              // Transmitted data and received ACK or NACK
    I2C_STATE_STOP              // Transmitted stop signal
} i2cModuleState_t;

i2cModuleState_t g_i2c_module_state = I2C_STATE_IDLE;

void i2cMaster_flushQueue()
{
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
    SSP1BUF = g_outgoing_data_queue[g_outgoing_data_queue_index];
    incrementIndex();

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
            else if (isNextByteAddress())
                //stateChange_startAnotherTransmission();
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
