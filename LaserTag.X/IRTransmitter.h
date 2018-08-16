#ifndef IRTRANSMITTER_H
#define	IRTRANSMITTER_H

#include <stdbool.h>

void initializeTransmitter(void);
void handleTransmissionTimingInterrupt(void);

// Asynchronously transmit the given bits over IR. If a transmission is in
// progress, returns false and does nothing. Otherwise returns true
bool transmitAsync(unsigned int data);

#endif	/* IRTRANSMITTER_H */
