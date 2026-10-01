#include "hardware.h"
#include "mqtt_handler.h"

void initHardware() {
    for (int i = 0; i < RELAY_COUNT; i++) {
        pinMode(relays[i].pin, OUTPUT);
        digitalWrite(relays[i].pin, RELAY_OFF_LEVEL);
    }
    delay(500);
    doorServo.attach(SERVO_DOOR_PIN);
    doorServo.writeMicroseconds(SERVO_STOP_US);
    doorState = IDLE_CLOSED;
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, HIGH);
}

void setRelay(RelayChannel& relay, bool on, unsigned long ms) {
    relay.on = on;
    digitalWrite(relay.pin, on ? RELAY_ON_LEVEL : RELAY_OFF_LEVEL);
    relay.timed = on && ms > 0;
    if (relay.timed) relay.offAt = millis() + ms;
    pubState(relay.stateTopic, on);
    Serial.printf(">>> HARDWARE: Relay %s turned %s\n", relay.name, on ? "ON" : "OFF");
}

void setRfid(bool on) {
    rfidEnabled = on;
    pubState(TOPIC_RFID_STATE, on);
}

void openDoor() {
    if (doorState == MOVING_OPEN || doorState == IDLE_OPEN) return;
    doorState = MOVING_OPEN;
    doorServo.writeMicroseconds(SERVO_OPEN_US);
    doorActionStart = millis();
    pub(TOPIC_DOOR_STATE, "OPEN");
    Serial.printf(">>> HARDWARE: Cua bat dau MO (%lu us), thoi gian: %lums\n", SERVO_OPEN_US, DOOR_SPIN_MS);
}

void closeDoor() {
    if (doorState == MOVING_CLOSE || doorState == IDLE_CLOSED) return;
    doorState = MOVING_CLOSE;
    doorServo.writeMicroseconds(SERVO_CLOSE_US);
    doorActionStart = millis();
    pub(TOPIC_DOOR_STATE, "CLOSED");
    Serial.printf(">>> HARDWARE: Cua bat dau DONG (%lu us), thoi gian: %lums\n", SERVO_CLOSE_US, DOOR_SPIN_MS);
}

void handleTimers() {
    unsigned long now = millis();
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (relays[i].timed && (long)(now - relays[i].offAt) >= 0) setRelay(relays[i], false);
    }
    if ((doorState == MOVING_OPEN || doorState == MOVING_CLOSE) && (long)(now - doorActionStart) >= DOOR_SPIN_MS) {
        doorServo.writeMicroseconds(SERVO_STOP_US);
        if (doorState == MOVING_OPEN) { doorState = IDLE_OPEN; Serial.println(">>> HARDWARE: Cua da MO hoan toan."); }
        else if (doorState == MOVING_CLOSE) { doorState = IDLE_CLOSED; Serial.println(">>> HARDWARE: Cua da DONG hoan toan."); }
    }
}