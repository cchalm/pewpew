#ifndef INPUTS_H
#define INPUTS_H

#include <stdbool.h>

// This function must be called regularly to update the return values of the functions below
void sampleInputs(void);

// Debounced edge events that occurred on the last call to sampleInputs
bool inputEvent_triggerPressed(void);
bool inputEvent_triggerReleased(void);
bool inputEvent_magazineEjected(void);
bool inputEvent_magazineInserted(void);

// Debounced input states as of the last call to sampleInputs
bool inputState_triggerPressed(void);
bool inputState_magazineIn(void);

#endif /* INPUTS_H */
