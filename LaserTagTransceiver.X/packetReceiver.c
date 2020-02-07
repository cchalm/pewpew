#include "packetReceiver.h"

#include "IRReceiver.h"
#include "crc.h"

#include "crcConstants.h"
#include "packetConstants.h"

#include <stdbool.h>
#include <stdint.h>

bool tryGetPacket(uint8_t* packet_out)
{
    uint16_t transmission;
    uint8_t transmission_length;
    if (!tryGetTransmission(&transmission, &transmission_length))
        return false;

    if (transmission_length != PACKET_TRANSMISSION_LENGTH)
        return false;

    uint8_t packet = transmission >> CRC_LENGTH;

    uint8_t actual_crc = transmission & ((1 << CRC_LENGTH) - 1);
    uint8_t expected_crc = crc(packet);

    bool crc_matches = expected_crc == actual_crc;

    if (crc_matches)
    {
        *packet_out = packet;
    }

    return crc_matches;
}