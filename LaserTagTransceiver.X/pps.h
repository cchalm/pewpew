#ifndef PPS_H
#define	PPS_H

#include <xc.h>

// Defines PPS constants, which are mysteriously not defined by the xc8 header
// for the chip. These values are specific to pic16f1619.

// Not all pins are available for each port. Unavailable pins are commented-out

#define PPS_SOURCE_RA0 0b00000
#define PPS_SOURCE_RA1 0b00001
#define PPS_SOURCE_RA2 0b00010
#define PPS_SOURCE_RA3 0b00011
#define PPS_SOURCE_RA4 0b00100
#define PPS_SOURCE_RA5 0b00101
//#define PPS_SOURCE_RA6 0b00110
//#define PPS_SOURCE_RA7 0b00111

//#define PPS_SOURCE_RB0 0b01000
//#define PPS_SOURCE_RB1 0b01001
//#define PPS_SOURCE_RB2 0b01010
//#define PPS_SOURCE_RB3 0b01011
#define PPS_SOURCE_RB4 0b01100
#define PPS_SOURCE_RB5 0b01101
#define PPS_SOURCE_RB6 0b01110
#define PPS_SOURCE_RB7 0b01111

#define PPS_SOURCE_RC0 0b10000
#define PPS_SOURCE_RC1 0b10001
#define PPS_SOURCE_RC2 0b10010
#define PPS_SOURCE_RC3 0b10011
#define PPS_SOURCE_RC4 0b10100
#define PPS_SOURCE_RC5 0b10101
#define PPS_SOURCE_RC6 0b10110
#define PPS_SOURCE_RC7 0b10111

#define PPS_SOURCE_LATxy        0b00000
#define PPS_SOURCE_sync_C1OUT   0b00001
#define PPS_SOURCE_sync_C2OUT   0b00010
#define PPS_SOURCE_ZCD1_out     0b00011
#define PPS_SOURCE_LC1_out      0b00100
#define PPS_SOURCE_LC2_out      0b00101
#define PPS_SOURCE_LC3_out      0b00110
#define PPS_SOURCE_LC4_out      0b00111
#define PPS_SOURCE_CWG1OUTA     0b01000
#define PPS_SOURCE_CWG1OUTB     0b01001
#define PPS_SOURCE_CWG1OUTC     0b01010
#define PPS_SOURCE_CWG1OUTD     0b01011
#define PPS_SOURCE_CCP1_out     0b01100
#define PPS_SOURCE_CCP2_out     0b01101
#define PPS_SOURCE_PWM3_out     0b01110
#define PPS_SOURCE_PWM4_out     0b01111
#define PPS_SOURCE_SCK_SCL      0b10000
#define PPS_SOURCE_SDO_SDA      0b10001
#define PPS_SOURCE_TX_CK        0b10010
#define PPS_SOURCE_DT           0b10011

#endif	/* PPS_H */

