#ifndef IRRECEIVER_H
#define IRRECEIVER_H

#include <stdbool.h>
#include <stdint.h>

void irReceiver_initialize(void);
void irReceiver_shutdown(void);

void irReceiver_interruptHandler(void);

// Returns true and copies the transmission data and length, in bits, into the out parameters if a transmission was
// received since the last call to tryGetTransmissionData. Returns false otherwise. data_out must point to a static
// buffer of at least ((MAX_TRANSMISSION_LENGTH + 7) / 8 bytes. The contents of data_out are
// undefined when this function returns false.
bool irReceiver_tryGetTransmission(uint8_t* data_out, uint8_t* data_length_out);

#define DEBUG_RECEIVED_PULSE_LENGTHS
#ifdef DEBUG_RECEIVED_PULSE_LENGTHS
#include <stddef.h>
extern const size_t PULSE_LENGTH_ARRAY_SIZE;
// Returns an array of length MAX_TRANSMISSION_LENGTH containing all the pulse/gap lengths received in the last
// transmission. The array may be overwritten at any time
uint8_t* irReceiver_getPulseLengthArray();
uint8_t* irReceiver_getGapLengthArray();
#endif

#endif /* IRRECEIVER_H */
