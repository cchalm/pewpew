#ifndef I2CMASTER_H
#define I2CMASTER_H

#include <stdbool.h>
#include <stdint.h>

void i2cSlave_initialize(void);
void i2cSlave_shutdown(void);

void i2cSlave_eventHandler(void);

// Read one message from the master. Takes the maximum number of bytes to return. Returns the data and the number of
// returned bytes via two out parameters, and returns true if the data includes the final byte of the message. A return
// value of false with a returned data length of zero indicates that there is no data to read.
bool i2cSlave_read(uint8_t max_data_length, uint8_t* data_out, uint8_t* data_length_out);
// Queue data to be sent to the master. This data is not organized into distinct messages. The master determines how
// many bytes to read at a time. Delimiting messages, if desired, must be done by the caller and coordinated between the
// master and slave software.
void i2cSlave_write(uint8_t* data, uint8_t data_length);

#endif /* I2CMASTER_H */
