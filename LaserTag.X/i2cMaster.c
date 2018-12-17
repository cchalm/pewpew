#include "i2cMaster.h"

#include "error.h"
#include "pins.h"

#include <xc.h>

void initializeI2CMaster()
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
    //SSP1STATbits.SMP = 0;
}

uint8_t g_outgoing_address;
uint8_t* g_p_outgoing_data;
uint8_t g_outgoing_data_len;
// -1 indicates the address should be sent
int8_t g_outgoing_data_index;

bool transmitI2CMaster(uint8_t address, uint8_t* data, uint8_t data_len)
{
    if (isTransmissionInProgress())
        return false;
    
    g_outgoing_address = address;
    g_p_outgoing_data = data;
    g_outgoing_data_len = data_len;
    g_outgoing_data_index = -1;
    
    SSP1CON2bits.SEN = 1;
    
    return true;
}

bool isTransmissionInProgress()
{
    return g_p_outgoing_data != NULL;
}

void I2CTransmitterEventHandler(void)
{
    // Detecting and handling the SSP1 interrupt flag in a non-interrupting way,
    // as the handling code is not urgent
    
    if (!SSP1IF)
        return;
    
    // Five possible reasons the interrupt flag is set:
    // - Start condition detected
    // - Stop condition detected
    // - Data transfer byte transmitted/received (slave only?)
    // - Acknowledge transmitted/received
    // - Repeated Start generated
    
    if (SSP1STATbits.P)
    {
        // Stop condition detected
        // Transmission is over, do nothing
    }
    else
    {
        // Start or ack detected

        if (g_outgoing_data_index == g_outgoing_data_len)
        {
            // Tranmission finished
            g_p_outgoing_data = NULL;
            SSP1CON2bits.PEN = 1;
        }
        else if (g_p_outgoing_data != NULL)
        {
            uint8_t byte_to_send;

            if (g_outgoing_data_index == -1)
            {
                byte_to_send = g_outgoing_address;
            }
            else
            {
                if (SSP1CON2bits.ACKSTAT)
                    fatal(ERROR_I2C_NO_ACK);

                byte_to_send = g_p_outgoing_data[g_outgoing_data_index];
            }

            g_outgoing_data_index++;

            SSP1BUF = byte_to_send;

            if (WCOL == 1)
            {
                fatal(ERROR_I2C_BUFFER_CONTENTION);
            }
        }
    }

    SSP1IF = 0;
}
