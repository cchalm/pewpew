#ifndef PACKETTRANSMITTER_H
#define PACKETTRANSMITTER_H

#include <stdbool.h>
#include <stdint.h>

// Asynchronously transmits the given packet using IRTransmitter. If a
// transmission is in progress, returns false and does nothing. Otherwise
// returns true. Transmits with extra error checking bits
bool transmitPacketAsync(uint8_t packet);

#endif /* PACKETTRANSMITTER_H */
