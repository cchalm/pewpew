#include "packetReceiver.h"

#include "crc.h"
#include "IRReceiver.h"

#include "crcConstants.h"

#include <stdbool.h>
#include <stdint.h>

bool tryGetPacket(uint8_t* packet_out)
{
    uint16_t transmission;
    if (!tryGetTransmission(&transmission))
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