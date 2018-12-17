#ifndef PINS_H
#define	PINS_H

#include "pps.h"

#include <xc.h>

// Selection constants for any external pins we're using

// IR receiver pin
#define PPS_IN_VAL_IR_RECEIVER PPS_IN_VAL_RA2
#define TRIS_IR_RECEIVER TRISA2

// IR LED pin
#define PPS_OUT_REG_IR_LED PPS_OUT_VAL_RB7
#define TRIS_IR_LED TRISB7

// Transmission carrier signal pin
#define PPS_IN_VAL_CARRIER_SIGNAL PPS_IN_VAL_RA5
#define PPS_OUT_REG_CARRIER_SIGNAL PPS_OUT_REG_RA5
#define TRIS_CARRIER_SIGNAL TRISA5

// Modulation pin
#define PPS_IN_VAL_MODULATION_SIGNAL PPS_IN_VAL_RA4
#define PPS_OUT_REG_MODULATION_SIGNAL PPS_OUT_REG_RA4
#define TRIS_MODULATION_SIGNAL TRISA4

// Error LED pin
#define PIN_ERROR_LED RC7
#define TRIS_ERROR_LED TRISC7

// SCL pin

// SDA pin

#endif	/* PINS_H */

