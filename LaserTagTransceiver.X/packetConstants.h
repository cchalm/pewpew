#ifndef PACKETSTATS_H
#define PACKETSTATS_H

#include "crcConstants.h"

// Length of a data packet. This cannot be increased above 8 without code
// changes
#define PACKET_LENGTH 8
// Length of the entire transmission container for a packet
#define PACKET_TRANSMISSION_LENGTH ((PACKET_LENGTH) + CRC_LENGTH)

#define EVALUATE_CONSTANTS
#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const volatile uint8_t PACKET_LENGTH_eval = PACKET_LENGTH;
const volatile uint8_t PACKET_TRANSMISSION_LENGTH_eval = PACKET_TRANSMISSION_LENGTH;
#endif

#endif /* PACKETSTATS_H */
