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

// Returns true if a transmission was received since the last call to
// getTransmissionData
bool transmissionReceived(void);
unsigned int getTransmissionData(void);

#endif	/* IRRECEIVER_H */
