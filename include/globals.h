#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <time.h>
#include <SPI.h>
#include <cstring>
#include <cstdio>
#include "driver/gpio.h"
#include <ArduinoOTA.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <ESP32Servo.h>
#include <DHTesp.h>
#include <ArduinoJson.h>

#include "config.h"

// ===== Enums & Structs =====
enum DoorState { IDLE_CLOSED, MOVING_OPEN, IDLE_OPEN, MOVING_CLOSE };

struct RelayChannel {
    const char* name;
    uint8_t pin;
    const char* stateTopic;
    const char* ctrlTopic;
    bool on;
    bool timed;
    unsigned long offAt;
};

struct PendingEcho {
    const char* topic;
    bool on;
    unsigned long at;
};

// ===== Extern Global Objects =====
extern MFRC522DriverPinSimple ss_pin;
extern MFRC522DriverSPI driver;
extern MFRC522 mfrc522;
extern Servo doorServo;
extern DHTesp dht;
extern WiFiClient espClient;
extern PubSubClient client;

// ===== Extern Global Variables =====
extern DoorState doorState;
extern unsigned long doorActionStart;
extern bool rfidEnabled;
extern unsigned long scanCount;
extern bool subscribed;

extern unsigned long lastLdrPublish;
extern unsigned long lastDhtPublish;
extern unsigned long lastFlamePublish;
extern unsigned long lastMq2Publish;
extern bool lastPirState;
extern bool lastDoorSensorState;
extern unsigned long lastMqttReconnectAttempt;

extern RelayChannel relays[];
extern const int RELAY_COUNT;
extern RelayChannel& relayDenPK;
extern RelayChannel& relayQuat;

extern PendingEcho echoes[];
extern int echoCount;
// ===== RFID =====
extern const char* ALLOWED_UIDS[];
#endif