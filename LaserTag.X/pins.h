#ifndef PINCONFIG_H
#define	PINCONFIG_H

#include "clc.h"
#include "pps.h"

// Selection constants for any external pins we're using

// IR receiver pin
#define PPS_SOURCE_IR_RECEIVER PPS_SOURCE_RA2
#define PPS_TARGET_IR_RECEIVER RA2PPS
#define TRIS_IR_RECEIVER TRISA2

// IR LED pin
#define PPS_SOURCE_IR_LED PPS_SOURCE_RA4
#define PPS_TARGET_IR_LED RA4PPS
#define TRIS_IR_LED TRISA4

// Transmission carrier signal pin
#define PPS_SOURCE_CARRIER_SIGNAL PPS_SOURCE_RC7
#define PPS_TARGET_CARRIER_SIGNAL RC7PPS
#define TRIS_CARRIER_SIGNAL TRISC7

// Modulation pin
#define PPS_SOURCE_MODULATION_SIGNAL PPS_SOURCE_RC6
#define PPS_TARGET_MODULATION_SIGNAL RC6PPS
#define TRIS_MODULATION_SIGNAL TRISC6

#endif	/* PINCONFIG_H */

