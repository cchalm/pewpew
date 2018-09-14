#ifndef IRRECEIVER_H
#define	IRRECEIVER_H

#include <stdbool.h>
#include <stdint.h>

void initializeReceiver(void);

void receiverInterruptHandler(void);
void receiverEventHandler(void);

// Returns true and copies the transmission data to the given address if a valid
// transmission was received since the last call to tryGetTransmissionData.
// Returns false otherwise. The contents of data_out are undefined when this
// function returns false.
bool tryGetTransmissionData(uint8_t* data_out);

#define DEBUG_DATA_RECEPTION
#ifdef DEBUG_DATA_RECEPTION
bool tryGetTransmissionDataRaw(uint16_t* data_out);
#endif

#define DEBUG_RECEIVED_PULSE_LENGTHS
#ifdef DEBUG_RECEIVED_PULSE_LENGTHS
extern const size_t PULSE_LENGTH_ARRAY_SIZE;
// Returns an array of length TRANSMISSION_LENGTH containing all the pulse/gap
// lengths received in the last transmission. The array may be overwritten at
// any time
uint8_t* getPulseLengthArray();
uint8_t* getGapLengthArray();
#endif

#endif	/* IRRECEIVER_H */
