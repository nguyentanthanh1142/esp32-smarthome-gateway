#include "sensors.h"
#include "mqtt_handler.h"

void initSensors()
{
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    pinMode(LDR_PIN, INPUT);
    pinMode(PIR_PIN, INPUT);
    pinMode(FLAME_PIN, INPUT_PULLDOWN);
    pinMode(MQ2_PIN, INPUT);
    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
    dht.setup(DHT_PIN, DHTesp::DHT11);
    Serial.println("Sensors initialized.");
}

void handleLDR()
{
    unsigned long now = millis();
    if (now - lastLdrPublish >= LDR_PUBLISH_INTERVAL)
    {
        lastLdrPublish = now;
        int rawValue = analogRead(LDR_PIN);
        char payload[10];
        snprintf(payload, sizeof(payload), "%d", rawValue);
        pub(TOPIC_LDR_STATE, payload);
    }
}

void handlePIR()
{
    bool currentPirState = digitalRead(PIR_PIN) == HIGH;
    if (currentPirState != lastPirState)
    {
        lastPirState = currentPirState;
        pub(TOPIC_PIR_STATE, currentPirState ? "ON" : "OFF");
        Serial.printf(">>> PIR: %s\n", currentPirState ? "DETECTED" : "CLEAR");
    }
}

void handleDHT()
{
    unsigned long now = millis();
    if (now - lastDhtPublish >= DHT_PUBLISH_INTERVAL)
    {
        lastDhtPublish = now;
        TempAndHumidity newValues = dht.getTempAndHumidity();
        if (dht.getStatus() != 0)
        {
            Serial.println(">>> DHT Error: " + String(dht.getStatusString()));
            return;
        }
        JsonDocument doc;
        doc["temp"] = newValues.temperature;
        doc["hum"] = newValues.humidity;
        char jsonBuffer[128];
        serializeJson(doc, jsonBuffer);
        pub(TOPIC_DHT_STATE, jsonBuffer);
    }
}

void handleFlame()
{
    unsigned long now = millis();
    if (now - lastFlamePublish >= FLAME_PUBLISH_INTERVAL)
    {
        lastFlamePublish = now;
        int raw = analogRead(FLAME_PIN); 
        if (raw < 100)
        {
            pub(TOPIC_FLAME_STATE, "0");
            return;
        }
        int flamePercent = map(raw, 4095, 0, 0, 100);
        flamePercent = constrain(flamePercent, 0, 100);
        char payload[10];
        snprintf(payload, sizeof(payload), "%d", flamePercent);
        pub(TOPIC_FLAME_STATE, payload);
    }
}

void handleMQ2()
{
    unsigned long now = millis();
    if (now - lastMq2Publish >= MQ2_PUBLISH_INTERVAL)
    {
        lastMq2Publish = now;
        int raw = analogRead(MQ2_PIN);
        int gasPercent = map(raw, 0, 3500, 0, 100);
        gasPercent = constrain(gasPercent, 0, 100);
        Serial.printf(">>> MQ2 RAW: %d | Smoke/Gas Level: %d%%\n", raw, gasPercent);
        char payload[10];
        snprintf(payload, sizeof(payload), "%d", gasPercent);
        pub(TOPIC_MQ2_STATE, payload);
    }
}

void handleDoorSensor()
{
    bool currentState = digitalRead(DOOR_SENSOR_PIN) == HIGH;
    if (currentState != lastDoorSensorState)
    {
        lastDoorSensorState = currentState;
        const char *stateStr = currentState ? "OPEN" : "CLOSED";
        pub(TOPIC_DOOR_SENSOR_STATE, stateStr);
        Serial.printf(">>> DOOR SENSOR (Vat ly): %s\n", stateStr);
    }
}