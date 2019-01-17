#ifndef I2CMASTER_H
#define I2CMASTER_H

#include <stdbool.h>
#include <stdint.h>

void i2cMaster_initialize(void);
void i2cMaster_eventHandler(void);

// Queues the given data to transmit to the device with the given address.
void i2cMaster_write(uint8_t address, uint8_t* data, uint8_t data_length);
// Queues a read of the given number of bytes from the device with the given address.
void i2cMaster_read(uint8_t address, uint8_t read_length);
// Get the results of a prior read, if available. Takes the maximum number of bytes to return. Returns the data and the
// actual length of the data as out parameters. Returns true if the returned data includes the last byte of a distinct
// read, false otherwise. A return value of false with a returned data length of zero indicates that there is no data
// available
bool i2cMaster_getReadResults(uint8_t address, uint8_t max_length, uint8_t* data_out, uint8_t* length_out);

// Blocks and pumps the event handler until all queued messages have been sent. This includes reads and writes.
void i2cMaster_flushQueue(void);

#endif /* I2CMASTER_H */
