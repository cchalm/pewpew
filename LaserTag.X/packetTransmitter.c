#include "packetTransmitter.h"

#include "crc.h"
#include "crcConstants.h"
#include "IRTransmitter.h"
#include "packetConstants.h"

#include <stdbool.h>
#include <stdint.h>

bool transmitPacketAsync(uint8_t packet)
{
    // Append crc bits to the transmission
    return transmitAsync(((int16_t)packet << CRC_LENGTH) | crc(packet), PACKET_TRANSMISSION_LENGTH);
}