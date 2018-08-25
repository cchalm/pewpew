#ifndef IRTRANSMITTER_H
#define	IRTRANSMITTER_H

#include <stdbool.h>
#include <stdint.h>

void initializeTransmitter(void);

void transmitterInterruptHandler(void);
void transmitterEventHandler(void);

// Asynchronously transmit the given bits over IR. If a transmission is in
// progress, returns false and does nothing. Otherwise returns true
bool transmitAsync(uint16_t data);

#endif	/* IRTRANSMITTER_H */
