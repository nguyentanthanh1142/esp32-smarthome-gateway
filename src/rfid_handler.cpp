#include "rfid_handler.h"
#include "hardware.h"
#include "mqtt_handler.h"

void initRFID() {
    SPI.begin();
    mfrc522.PCD_Init();
    MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
}

bool isCardAllowed(const String& uid) {
    if (ALLOWED_UIDS[0] == nullptr) return true;
    for (int i = 0; ALLOWED_UIDS[i] != nullptr; i++) { if (uid == ALLOWED_UIDS[i]) return true; }
    return false;
}

void activate(const char* source, const String& uid) {
    scanCount++;
    bool granted = (strcmp(source, "mqtt") == 0) || isCardAllowed(uid);
    if (granted) openDoor();
    Serial.printf(">>> THE %s: %s\n", uid.c_str(), granted ? "HOP LE - mo cua" : "KHONG HOP LE - tu choi");

    JsonDocument doc;
    doc["source"] = source;
    doc["uid"] = uid;
    struct tm t;
    char timeBuf[25];
    if (getLocalTime(&t, 100)) {
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &t);
        doc["time"] = timeBuf;
    } else {
        doc["time"] = "unknown";
    }
    doc["count"] = scanCount;
    doc["access"] = granted ? "granted" : "denied";
    doc["door"] = (doorState == IDLE_OPEN || doorState == MOVING_OPEN) ? "OPEN" : "CLOSED";
    doc["den_pk"] = relayDenPK.on ? "ON" : "OFF";
    doc["quat"] = relayQuat.on ? "ON" : "OFF";
    doc["pir"] = lastPirState ? "ON" : "OFF";

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    pub(TOPIC_EVENT, jsonBuffer);
}

void checkCard() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;
    String uid;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    mfrc522.PICC_HaltA();
    if (!rfidEnabled) return;
    activate("rfid", uid);
}