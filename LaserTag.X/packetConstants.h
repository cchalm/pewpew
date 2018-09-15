#ifndef PACKETSTATS_H
#define	PACKETSTATS_H

#include "crcConstants.h"

#define PACKET_LENGTH 8
#define TRANSMISSION_LENGTH ((PACKET_LENGTH) + CRC_LENGTH)

#define EVALUATE_CONSTANTS
#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const volatile uint8_t PACKET_LENGTH_eval = PACKET_LENGTH;
const volatile uint8_t TRANSMISSION_LENGTH_eval = TRANSMISSION_LENGTH;
#endif

#endif	/* PACKETSTATS_H */

