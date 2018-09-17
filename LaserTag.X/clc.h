#ifndef CLC_H
#define	CLC_H

// Defines CLC constants, which are mysteriously not defined by the xc8 header
// for the chip. These values are specific to pic16f1619.

#define CLC_SOURCE_CLCIN0           0b000000
#define CLC_SOURCE_CLCIN1           0b000001
#define CLC_SOURCE_CLCIN2           0b000010
#define CLC_SOURCE_CLCIN3           0b000011
#define CLC_SOURCE_LC1_out          0b000100
#define CLC_SOURCE_LC2_out          0b000101
#define CLC_SOURCE_LC3_out          0b000110
#define CLC_SOURCE_LC4_out          0b000111
#define CLC_SOURCE_C1OUT_sync       0b001000
#define CLC_SOURCE_C2OUT_sync       0b001001
#define CLC_SOURCE_CWGOUTA          0b001010
#define CLC_SOURCE_CWGOUTB          0b001011
#define CLC_SOURCE_CCP1_out         0b001100
#define CLC_SOURCE_CCP2_out         0b001101
#define CLC_SOURCE_PWM3_out         0b001110
#define CLC_SOURCE_PWM4_out         0b001111
#define CLC_SOURCE_AT1_cmp1         0b010000
#define CLC_SOURCE_AT1_cmp2         0b010001
#define CLC_SOURCE_AT1_cmp3         0b010010
#define CLC_SOURCE_SMT1_match       0b010011
#define CLC_SOURCE_SMT2_match       0b010100
#define CLC_SOURCE_ZCD1_output      0b010101
#define CLC_SOURCE_TMR0_overflow    0b010110
#define CLC_SOURCE_TMR1_overflow    0b010111
#define CLC_SOURCE_TMR2_postscaled  0b011000
#define CLC_SOURCE_TMR3_overflow    0b011001
#define CLC_SOURCE_TMR4_postscaled  0b011010
#define CLC_SOURCE_TMR5_overflow    0b011011
#define CLC_SOURCE_TMR6_postscaled  0b011100
#define CLC_SOURCE_IOC_interrupt    0b011101
#define CLC_SOURCE_ADC_rc           0b011110
#define CLC_SOURCE_LFINTOSC         0b011111
#define CLC_SOURCE_HFINTOSC         0b100000
#define CLC_SOURCE_FOSC             0b100001
#define CLC_SOURCE_AT1_missedpulse  0b100010
#define CLC_SOURCE_AT1_perclk       0b100011
#define CLC_SOURCE_AT1_phsclk       0b100100
#define CLC_SOURCE_TX               0b100101
#define CLC_SOURCE_RX               0b100110
#define CLC_SOURCE_SCK              0b100111
#define CLC_SOURCE_SDO              0b101000

#define CLC_LOGIC_FUNCTION_AND_OR               0b000
#define CLC_LOGIC_FUNCTION_OR_XOR               0b001
#define CLC_LOGIC_FUNCTION_4IN_AND              0b010
#define CLC_LOGIC_FUNCTION_SR_LATCH             0b011
#define CLC_LOGIC_FUNCTION_1IN_DFLIPFLOP_S_R    0b100
#define CLC_LOGIC_FUNCTION_2IN_DFLIPFLOP_R      0b101
#define CLC_LOGIC_FUNCTION_JKFLIPFLOP_R         0b110
#define CLC_LOGIC_FUNCTION_1IN_TRANSPARENT_LATCH_S_R 0b111

#endif	/* CLC_H */

