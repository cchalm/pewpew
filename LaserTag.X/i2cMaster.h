#ifndef I2CMASTER_H
#define	I2CMASTER_H

#include <stdbool.h>
#include <stdint.h>

void initializeI2CMaster(void);

// This is not thread safe, it must not be called from ISRs
bool transmitI2CMaster(uint8_t address, uint8_t* data, uint8_t data_len);
bool isTransmissionInProgress(void);

void I2CTransmitterEventHandler(void);

#endif	/* I2CMASTER_H */

