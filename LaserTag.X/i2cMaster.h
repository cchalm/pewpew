#ifndef I2CMASTER_H
#define	I2CMASTER_H

#include <stdbool.h>
#include <stdint.h>

void i2cMaster_initialize(void);

// Queues the given data to transmit over I2C
void i2cMaster_transmit(uint8_t address, uint8_t* data, uint8_t data_len);
void i2cMaster_flushQueue(void);

void i2cMaster_eventHandler(void);

#endif	/* I2CMASTER_H */

