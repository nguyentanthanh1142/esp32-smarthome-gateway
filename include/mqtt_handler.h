#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include "globals.h"

void initMQTT();
void reconnect();
bool pub(const char* topic, const char* payload);
void pubState(const char* topic, bool on);
void setupMQTTDiscovery();
void subscribeAll();
void callback(char* topic, byte* payload, unsigned int length);
void addEcho(const char* topic, bool on);
bool consumeEcho(const String& topic, bool on);

#endif