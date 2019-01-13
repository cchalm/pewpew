#ifndef IRTRANSCEIVER_H
#define IRTRANSCEIVER_H

void irTransceiver_transmit(uint8_t* data, uint8_t data_length);
// Get a received transmission, if available. Takes the maximum number of bytes to return. Returns the data and the
// actual number of bytes returned as two out parameters. Returns true if the returned data is the end of a distinct
// transmission, false otherwise. A return value of false with a returned data length of zero indicates that no
// transmissions have been received.
bool irTransceiver_receive(uint8_t max_data_length, uint8_t* data_out, uint8_t* data_out_length);

#endif /* IRTRANSCEIVER_H */
