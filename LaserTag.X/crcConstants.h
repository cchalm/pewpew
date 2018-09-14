#ifndef CRCCONSTANTS_H
#define	CRCCONSTANTS_H

#define CRC_LENGTH 5

// CRC width in bits and polynomial recommended by https://users.ece.cmu.edu/~koopman/crc/
// to achieve a Hamming Distance of 4 with a maximum data word legnth of 10.
// Note the "implicit +1" notation used by Koopman. We will always use data
// words of length 8, for ease of use, creating a 13-bit codeword (data + crc)
#define POLYNOMIAL 0b101011
// The polynomial, minus the most significant "on" bit, and shifted to the most
// significant bits of the byte. Easier to use in the CLC algorithm like this
#define POLYNOMIAL_FOR_ALGORITHM 0b01011000

#define EVALUATE_CONSTANTS
#ifdef EVALUATE_CONSTANTS
#include <stdint.h>
const volatile uint8_t CRC_LENGTH_eval = CRC_LENGTH;
#endif

#endif	/* CRCCONSTANTS_H */

