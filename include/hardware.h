#ifndef HARDWARE_H
#define HARDWARE_H

#include "globals.h"

void initHardware();
void setRelay(RelayChannel& relay, bool on, unsigned long ms = 0);
void setRfid(bool on);
void openDoor();
void closeDoor();
void handleTimers();

#endif