#ifndef IRTRANSMITTER_H
#define	IRTRANSMITTER_H

#include <stdbool.h>
#include <stdint.h>

void initializeTransmitter(void);

void transmitterInterruptHandler(void);
void transmitterEventHandler(void);

// Asynchronously transmit the given bits over IR. If a transmission is in
// progress, returns false and does nothing. Otherwise returns true
bool transmitAsync(uint8_t data);

#define DEBUG_TRANSMISSION
#ifdef DEBUG_TRANSMISSION
// Transmit exactly the given bits, without any modification
bool transmitAsyncRaw(uint16_t data);
#endif

#endif	/* IRTRANSMITTER_H */
