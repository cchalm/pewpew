#ifndef PACKETRECEIVER_H
#define	PACKETRECEIVER_H

#include <stdbool.h>
#include <stdint.h>

// Returns true and copies the packet to the given address if a valid
// packet was received since the last call to tryGetPacket [or
// tryGetTransmission]. Returns false otherwise. The contents of packet_out are
// undefined when this function returns false.
bool tryGetPacket(uint8_t* packet_out);

#endif	/* PACKETRECEIVER_H */

