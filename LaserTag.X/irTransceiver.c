#include "irTransceiver.h"

#include "../LaserTagUtils.X/bitArray.h"
#include "crc.h"
#include "crcConstants.h"
#include "error.h"
#include "i2cMaster.h"

#include <xc.h>

const uint8_t TRANSCEIVER_ADDRESS = 0b1010001;

typedef enum {
    IR_RECEIVER_STATE_IDLE,
    IR_RECEIVER_STATE_AWAITING_LENGTH,
    IR_RECEIVER_STATE_AWAITING_DATA,
} irReceiverState_t;

// TODO share this between LaserTag and LaserTagTransceiver
#define MAX_TRANSMISSION_LENGTH 120

uint8_t g_transmission_buffer[NUM_BYTES(MAX_TRANSMISSION_LENGTH)];
// The length of the transmission currently in the buffer, in bits. length == 0 means there is no transmission available
// at this time
uint8_t g_transmission_length;

void irTransceiver_eventHandler()
{
    static irReceiverState_t state = IR_RECEIVER_STATE_IDLE;
    static uint8_t num_bits_to_read = 0;

    switch (state)
    {
        case IR_RECEIVER_STATE_IDLE:
        {
            // Read a length, so that we know how many bytes to ask for to get the real data
            i2cMaster_read(TRANSCEIVER_ADDRESS, 1);

            state = IR_RECEIVER_STATE_AWAITING_LENGTH;
            break;
        }

        case IR_RECEIVER_STATE_AWAITING_LENGTH:
        {
            uint8_t received_data_length;
            bool is_whole_message
                = i2cMaster_getReadResults(TRANSCEIVER_ADDRESS, 1, &num_bits_to_read, &received_data_length);

            if (received_data_length != 0)
            {
                if (received_data_length != 1 || !is_whole_message)
                    fatal(0);

                if (num_bits_to_read == 0)
                {
                    // Immediately start the next length read
                    i2cMaster_read(TRANSCEIVER_ADDRESS, 1);

                    state = IR_RECEIVER_STATE_AWAITING_LENGTH;
                }
                else
                {
                    if (num_bits_to_read > MAX_TRANSMISSION_LENGTH)
                        fatal(ERROR_RECEIVED_TRANSMISSION_TOO_LONG);

                    i2cMaster_read(TRANSCEIVER_ADDRESS, NUM_BYTES(num_bits_to_read));

                    state = IR_RECEIVER_STATE_AWAITING_DATA;
                }
            }

            break;
        }

        case IR_RECEIVER_STATE_AWAITING_DATA:
        {
            // If the transmission buffer is full, don't read new data yet
            if (g_transmission_length != 0)
                break;

            uint8_t received_data_length;
            bool is_whole_message = i2cMaster_getReadResults(TRANSCEIVER_ADDRESS, NUM_BYTES(num_bits_to_read),
                                                             g_transmission_buffer, &received_data_length);

            if (received_data_length != 0)
            {
                if (received_data_length != NUM_BYTES(num_bits_to_read) || !is_whole_message)
                    fatal(0);

                g_transmission_length = num_bits_to_read;

                // Immediately start the next length read
                i2cMaster_read(TRANSCEIVER_ADDRESS, 1);

                state = IR_RECEIVER_STATE_AWAITING_LENGTH;
            }

            break;
        }
    }
}

void irTransceiver_transmit(uint8_t* bitarray, uint8_t bitarray_length)
{
    // The first byte of the transmission specifies the number of bits in the subsequent data
    i2cMaster_writePartial(TRANSCEIVER_ADDRESS, &bitarray_length, 1, false);
    i2cMaster_writePartial(TRANSCEIVER_ADDRESS, bitarray, NUM_BYTES(bitarray_length), true);
}

bool irTransceiver_receive(uint8_t* bitarray_out, uint8_t bitarray_max_length, uint8_t* bitarray_length_out)
{
    if (g_transmission_length == 0)
        return false;

    if (g_transmission_length > bitarray_max_length)
    {
        // Discard the current transmission by setting its length to zero
        g_transmission_length = 0;
        return false;
    }

    // Copy the buffer into the out parameter and zero out the buffer
    for (uint8_t i = 0; i < NUM_BYTES(g_transmission_length); i++)
    {
        bitarray_out[i] = g_transmission_buffer[i];
        g_transmission_buffer[i] = 0;
    }
    *bitarray_length_out = g_transmission_length;

    // Set the transmission length to zero to indicate the transmission buffer can be overwritten
    g_transmission_length = 0;

    return true;
}

void irTransceiver_transmit8WithCRC(uint8_t data)
{
    uint8_t transmission[] = {data, crc(data) << (8 - CRC_LENGTH)};
    irTransceiver_transmit(transmission, 8 + CRC_LENGTH);
}

bool irTransceiver_receive8WithCRC(uint8_t* data_out)
{
    uint8_t transmission[2];
    uint8_t num_bits;
    if (!irTransceiver_receive(transmission, 8 + CRC_LENGTH, &num_bits))
        return false;

    if (num_bits != 8 + CRC_LENGTH)
        return false;

    uint8_t data = transmission[0];

    uint8_t actual_crc = transmission[1] >> (8 - CRC_LENGTH);
    uint8_t expected_crc = crc(data);

    bool crc_matches = expected_crc == actual_crc;
    if (crc_matches)
    {
        *data_out = data;
    }

    return crc_matches;
}
