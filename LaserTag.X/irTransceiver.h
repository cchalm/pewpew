#ifndef IRTRANSCEIVER_H
#define IRTRANSCEIVER_H

#include <stdbool.h>
#include <stdint.h>

void irTransceiver_eventHandler(void);

void irTransceiver_transmit(uint8_t* bitarray, uint8_t bitarray_length);
// Get a received transmission, if available. Returns the bits and the number of bits as two out parameters. Returns
// true if a transmission was returned, false otherwise. Skips and discards transmissions that are longer than
// bitarray_max_length
bool irTransceiver_receive(uint8_t* bitarray_out, uint8_t bitarray_max_length, uint8_t* bitarray_length_out);

// Same as transmit and receive, except transmits/receives 8 bits of data with a CRC. Received transmissions for which
// the CRC doesn't match the data are discarded and not reported
void irTransceiver_transmit8WithCRC(uint8_t data);
bool irTransceiver_receive8WithCRC(uint8_t* data_out);

#endif /* IRTRANSCEIVER_H */
