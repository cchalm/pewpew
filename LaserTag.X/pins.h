#ifndef PINS_H
#define PINS_H

#include "pps.h"

#include <xc.h>

// Selection constants for any external pins we're using

// SCL pin
#define PPS_IN_REG_SCL PPS_IN_REG_SSPCLK
#define PPS_IN_VAL_SCL PPS_IN_VAL_RC3
#define PPS_OUT_REG_SCL PPS_OUT_REG_RC3
#define PPS_OUT_VAL_SCL PPS_OUT_VAL_SCK_SCL
#define TRIS_SCL TRISC3

// SDA pin
#define PPS_IN_REG_SDA PPS_IN_REG_SSPDAT
#define PPS_IN_VAL_SDA PPS_IN_VAL_RC4
#define PPS_OUT_REG_SDA PPS_OUT_REG_RC4
#define PPS_OUT_VAL_SDA PPS_OUT_VAL_SDO_SDA
#define TRIS_SDA TRISC4

// Muzzle LED
#define PIN_MUZZLE_LED RA5
#define TRIS_MUZZLE_LED TRISA5
#define LATCH_MUZZLE_LED LATAbits.LATA5

// Hit LED
#define PIN_HIT_LED RA4
#define TRIS_HIT_LED TRISA4
#define LATCH_HIT_LED LATAbits.LATA4

// Port where all input pins reside
#define INPUT_PORT PORTC

// Trigger input
#define PIN_TRIGGER RC2
#define PORT_INDEX_TRIGGER 2
#define PORT_MASK_TRIGGER 0b100
#define TRIGGER_PRESSED HIGH
#define TRIGGER_NOT_PRESSED LOW

// Reload input
#define PIN_RELOAD RC1
#define PORT_INDEX_RELOAD 1
#define PORT_MASK_RELOAD 0b10
#define MAG_IN HIGH
#define MAG_OUT LOW

#endif /* PINS_H */
