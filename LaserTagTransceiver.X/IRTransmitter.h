#ifndef IRTRANSMITTER_H
#define IRTRANSMITTER_H

#include <stdbool.h>
#include <stdint.h>

void irTransmitter_initialize(void);
void irTransmitter_shutdown(void);

void irTransmitter_interruptHandler(void);

// Asynchronously transmit the given data over IR. Takes the transmission data as an array and its length in bits.
// Transmits bytes in little endian order, e.g. the 0th byte is transmitted first. The bits of each byte are transmitted
// in big endian order, e.g. the 0th bit is transmitted last, e.g. the 0th bit of the 0th is the last to be sent. If a
// transmission is in progress, returns false and does nothing. Otherwise returns true
bool irTransmitter_transmitAsync(uint8_t* data, uint8_t length);

#endif /* IRTRANSMITTER_H */
