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

#endif /* PINS_H */
