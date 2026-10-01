#ifndef RFID_HANDLER_H
#define RFID_HANDLER_H

#include "globals.h"

void initRFID();
void checkCard();
void activate(const char* source, const String& uid);
bool isCardAllowed(const String& uid);
void mqttHeartbeat();

#endif