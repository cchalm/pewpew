/* 
 * File:   IRReceiver.h
 * Author: chris
 *
 * Created on August 11, 2018, 9:45 PM
 */

#ifndef IRRECEIVER_H
#define	IRRECEIVER_H

#include <stdbool.h>

void initializeReceiver(void);
void handleSignalReceptionInterrupt(void);

// Returns true and copies the transmission data to the given address if a valid
// transmission was received since the last call to tryGetTransmissionData.
// Returns false otherwise. The contents of data_out are undefined when this
// function returns false.
bool tryGetTransmissionData(unsigned int* data_out);

#endif	/* IRRECEIVER_H */
