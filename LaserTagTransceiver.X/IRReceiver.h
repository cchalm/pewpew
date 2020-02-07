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

#endif /* IRRECEIVER_H */
