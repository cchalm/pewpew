#include "irTransceiver.h"

#include "i2cMaster.h"

const uint8_t TRANSCEIVER_ADDRESS = 0b1010001;

void irTransceiver_transmit(uint8_t* data, uint8_t data_length)
{
    i2cMaster_write(TRANSCEIVER_ADDRESS, data, data_length);
}

bool irTransceiver_receive(uint8_t max_data_length, uint8_t* data_out, uint8_t* data_out_length)
{
    // TODO
    return false;
}
