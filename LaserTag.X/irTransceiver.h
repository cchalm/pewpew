#ifndef IRTRANSCEIVER_H
#define IRTRANSCEIVER_H

#include <stdbool.h>
#include <stdint.h>

void irTransceiver_transmit(uint8_t* bitarray, uint8_t bitarray_length);
// Get a received transmission, if available. Returns the bits and the number of bits as two out parameters. Returns
// true if a transmission was returned, false otherwise. The data out parameter must point to an array large enough to
// store the maximum transmission length
bool irTransceiver_receive(uint8_t* bitarray_out, uint8_t* bitarray_length_out);

#endif /* IRTRANSCEIVER_H */
