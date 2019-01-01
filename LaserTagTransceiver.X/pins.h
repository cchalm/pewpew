#ifndef PINS_H
#define PINS_H

#include "pps.h"

#include <xc.h>

// Selection constants for any external pins we're using

// IR receiver pin
#define PPS_IN_VAL_IR_RECEIVER PPS_IN_VAL_RC3
#define TRIS_IR_RECEIVER TRISC3

// IR LED pin
#define PPS_OUT_REG_IR_LED PPS_OUT_REG_RC5
#define TRIS_IR_LED TRISC5

// Transmission carrier signal pin
#define PPS_IN_VAL_CARRIER_SIGNAL PPS_IN_VAL_RC1
#define PPS_OUT_REG_CARRIER_SIGNAL PPS_OUT_REG_RC1
#define TRIS_CARRIER_SIGNAL TRISC1

// Modulation pin
#define PPS_IN_VAL_MODULATION_SIGNAL PPS_IN_VAL_RC2
#define PPS_OUT_REG_MODULATION_SIGNAL PPS_OUT_REG_RC2
#define TRIS_MODULATION_SIGNAL TRISC2

// Error LED pin
#define PIN_ERROR_LED RA2
#define TRIS_ERROR_LED TRISA2

// SCL pin
#define PPS_IN_REG_SCL PPS_IN_REG_SSPCLK
#define PPS_IN_VAL_SCL PPS_IN_VAL_RA4
#define PPS_OUT_REG_SCL PPS_OUT_REG_RA4
#define PPS_OUT_VAL_SCL PPS_OUT_VAL_SCK_SCL
#define TRIS_SCL TRISA4

// SDA pin
#define PPS_IN_REG_SDA PPS_IN_REG_SSPDAT
#define PPS_IN_VAL_SDA PPS_IN_VAL_RA5
#define PPS_OUT_REG_SDA PPS_OUT_REG_RA5
#define PPS_OUT_VAL_SDA PPS_OUT_VAL_SDO_SDA
#define TRIS_SDA TRISA5

#endif /* PINS_H */
