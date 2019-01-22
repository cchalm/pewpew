#ifndef IRTRANSMITTER_H
#define IRTRANSMITTER_H

#include <stdbool.h>
#include <stdint.h>

void irTransmitter_initialize(void);
void irTransmitter_shutdown(void);

void irTransmitter_interruptHandler(void);

// Asynchronously transmit the given bits over IR. If a transmission is in
// progress, returns false and does nothing. Otherwise returns true
bool irTransmitter_transmitAsync(uint16_t data, uint8_t length);

#endif /* IRTRANSMITTER_H */
