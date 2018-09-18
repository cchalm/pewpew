#ifndef PINCONFIG_H
#define	PINCONFIG_H

#include "clc.h"
#include "pps.h"

#include <xc.h>

// Selection constants for any external pins we're using

// IR receiver pin
#define PPS_SOURCE_IR_RECEIVER PPS_SOURCE_RA2
#define PPS_TARGET_IR_RECEIVER RA2PPS
#define TRIS_IR_RECEIVER TRISA2

// IR LED pin
#define PPS_SOURCE_IR_LED PPS_SOURCE_RB7
#define PPS_TARGET_IR_LED RB7PPS
#define TRIS_IR_LED TRISB7

// Transmission carrier signal pin
#define PPS_SOURCE_CARRIER_SIGNAL PPS_SOURCE_RA5
#define PPS_TARGET_CARRIER_SIGNAL RA5PPS
#define TRIS_CARRIER_SIGNAL TRISA5

// Modulation pin
#define PPS_SOURCE_MODULATION_SIGNAL PPS_SOURCE_RA4
#define PPS_TARGET_MODULATION_SIGNAL RA4PPS
#define TRIS_MODULATION_SIGNAL TRISA4

// Error LED pin
#define PIN_ERROR_LED RB6
#define TRIS_ERROR_LED TRISB6

#endif	/* PINCONFIG_H */

