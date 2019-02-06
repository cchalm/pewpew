#include "irTransceiver.h"

#include "../LaserTagUtils.X/bitArray.h"
#include "crc.h"
#include "crcConstants.h"
#include "error.h"
#include "i2cMaster.h"

const uint8_t TRANSCEIVER_ADDRESS = 0b1010001;

void irTransceiver_transmit(uint8_t* bitarray, uint8_t bitarray_length)
{
    // The first byte of the transmission specifies the number of bits in the subsequent data
    i2cMaster_writePartial(TRANSCEIVER_ADDRESS, &bitarray_length, 1, false);
    i2cMaster_writePartial(TRANSCEIVER_ADDRESS, bitarray, NUM_BYTES(bitarray_length), true);
}

typedef enum {
    IR_RECEIVER_STATE_IDLE,
    IR_RECEIVER_STATE_AWAITING_LENGTH,
    IR_RECEIVER_STATE_AWAITING_DATA,
} irReceiverState_t;

bool irTransceiver_receive(uint8_t* bitarray_out, uint8_t* bitarray_length_out)
{
    static irReceiverState_t state = IR_RECEIVER_STATE_IDLE;
    static uint8_t num_bits_to_read = 0;

    bool data_returned = false;

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
                    i2cMaster_read(TRANSCEIVER_ADDRESS, NUM_BYTES(num_bits_to_read));

                    state = IR_RECEIVER_STATE_AWAITING_DATA;
                }
            }

            break;
        }

        case IR_RECEIVER_STATE_AWAITING_DATA:
        {
            uint8_t received_data_length;
            bool is_whole_message = i2cMaster_getReadResults(TRANSCEIVER_ADDRESS, NUM_BYTES(num_bits_to_read),
                                                             bitarray_out, &received_data_length);

            if (received_data_length != 0)
            {
                if (received_data_length != NUM_BYTES(num_bits_to_read) || !is_whole_message)
                    fatal(0);

                *bitarray_length_out = num_bits_to_read;
                data_returned = true;

                // Immediately start the next length read
                i2cMaster_read(TRANSCEIVER_ADDRESS, 1);

                state = IR_RECEIVER_STATE_AWAITING_LENGTH;
            }

            break;
        }
    }

    return data_returned;
}

void irTransceiver_transmit8WithCRC(uint8_t data)
{
    uint8_t transmission[] = {data, crc(data)};
    irTransceiver_transmit(transmission, 8 + CRC_LENGTH);
}

bool irTransceiver_receive8WithCRC(uint8_t* data_out)
{
    uint8_t transmission[2];
    uint8_t num_bits;
    if (!irTransceiver_receive(transmission, &num_bits))
        return false;

    if (num_bits != 8 + CRC_LENGTH)
        return false;

    uint8_t data = transmission[0];

    uint8_t actual_crc = transmission[1];
    uint8_t expected_crc = crc(data);

    bool crc_matches = expected_crc == actual_crc;
    if (crc_matches)
    {
        *data_out = data;
    }

    return crc_matches;
}
