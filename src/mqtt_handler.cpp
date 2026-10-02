#include "mqtt_handler.h"
#include "hardware.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

static String pendingOtaPayload = "";

void initMQTT()
{
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
    client.setKeepAlive(15);
    client.setBufferSize(1024);
}

bool pub(const char *topic, const char *payload)
{
    bool ok = client.publish(topic, payload, true);
    Serial.printf("PUB [%s] %s %s\n", topic, payload, ok ? "OK" : "FAIL");
    return ok;
}

void pubState(const char *topic, bool on)
{
    if (pub(topic, on ? "ON" : "OFF") && subscribed)
        addEcho(topic, on);
}

void addEcho(const char *topic, bool on)
{
    if (echoCount == MAX_ECHO)
    {
        memmove(echoes, echoes + 1, sizeof(PendingEcho) * (MAX_ECHO - 1));
        echoCount--;
    }
    echoes[echoCount++] = {topic, on, millis()};
}

bool consumeEcho(const String &topic, bool on)
{
    unsigned long now = millis();
    int w = 0;
    for (int i = 0; i < echoCount; i++)
    {
        if (now - echoes[i].at < ECHO_TIMEOUT_MS)
            echoes[w++] = echoes[i];
    }
    echoCount = w;
    for (int i = 0; i < echoCount; i++)
    {
        if (topic != echoes[i].topic)
            continue;
        if (echoes[i].on != on)
            return false;
        memmove(&echoes[i], &echoes[i + 1], sizeof(PendingEcho) * (echoCount - i - 1));
        echoCount--;
        return true;
    }
    return false;
}

void reconnect()
{
    unsigned long now = millis();
    if (now - lastMqttReconnectAttempt >= MQTT_RECONNECT_INTERVAL)
    {
        lastMqttReconnectAttempt = now;
        Serial.print("Dang ket noi MQTT...");

        if (client.connect(DEVICE_ID, MQTT_USER, MQTT_PASS, TOPIC_STATUS, 1, true, "offline"))
        {
            Serial.println("OK");
            echoCount = 0;

            subscribeAll();
            subscribed = true;

            client.publish(TOPIC_STATUS, "online", true);

            setupMQTTDiscovery();

            for (int i = 0; i < RELAY_COUNT; i++)
                pubState(relays[i].stateTopic, relays[i].on);
            pub(TOPIC_DOOR_STATE, (doorState == IDLE_OPEN || doorState == MOVING_OPEN) ? "OPEN" : "CLOSED");
            pubState(TOPIC_RFID_STATE, rfidEnabled);

            Serial.println(">>> MQTT Reconnected Successfully!");
        }
        else
        {
            int rc = client.state();
            Serial.printf("LOI! Ma loi rc=%d (Se thu lai sau 5s)\n", rc);
        }
    }
}

void subscribeAll()
{
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        client.subscribe(relays[i].ctrlTopic, 1);
        client.subscribe(relays[i].stateTopic, 1);
    }
    client.subscribe(TOPIC_DOOR_CTRL, 1);
    client.subscribe(TOPIC_RFID_CTRL, 1);
    client.subscribe(TOPIC_RFID_STATE, 1);
    client.subscribe(TOPIC_OTA_UPDATE, 1);
    Serial.println("Subscribed to all topics.");
}

String normalizeCmd(String s)
{
    s.trim();
    s.toUpperCase();
    int colon = s.lastIndexOf(':');
    if (colon >= 0)
        s = s.substring(colon + 1);
    s.replace("\"", "");
    s.replace("{", "");
    s.replace("}", "");
    s.replace(" ", "");
    if (s == "1" || s == "TRUE")
        s = "ON";
    if (s == "0" || s == "FALSE")
        s = "OFF";

    return s;
}

void callback(char *topic, byte *payload, unsigned int length)
{
    String t = String(topic);
    String raw;
    raw.reserve(length);
    for (unsigned int i = 0; i < length; i++)
        raw += (char)payload[i];
    if (length == 0)
        return;

    Serial.printf(">>> RX [%s] raw='%s'\n", t.c_str(), raw.c_str());

    // OTA: chỉ lưu lệnh, KHÔNG tải ở đây (đang nằm trong client.loop())
    if (t == TOPIC_OTA_UPDATE)
    {
        pendingOtaPayload = raw;
        return;
    }

    String cmd = normalizeCmd(raw);

    for (int i = 0; i < RELAY_COUNT; i++)
    {
        RelayChannel &r = relays[i];
        if (t == r.ctrlTopic)
        {
            if (cmd == "ON")
                setRelay(r, true);
            else if (cmd == "OFF")
                setRelay(r, false);
            else if (cmd == "TOGGLE")
                setRelay(r, !r.on);
            return;
        }
        if (t == r.stateTopic)
        {
            if (cmd != "ON" && cmd != "OFF")
                return;
            bool on = (cmd == "ON");
            if (consumeEcho(t, on))
                return;
            if (on != r.on || r.timed)
                setRelay(r, on);
            return;
        }
    }

    if (t == TOPIC_DOOR_CTRL)
    {
        if (cmd == "OPEN" || cmd == "ON" || cmd == "UNLOCK")
            openDoor();
        else if (cmd == "CLOSE" || cmd == "OFF" || cmd == "LOCK")
            closeDoor();
        return;
    }

    if (t == TOPIC_RFID_CTRL)
    {
        if (cmd == "ON")
            setRfid(true);
        else if (cmd == "OFF")
            setRfid(false);
    }
    else if (t == TOPIC_RFID_STATE)
    {
        if (cmd != "ON" && cmd != "OFF")
            return;
        bool on = (cmd == "ON");
        if (consumeEcho(t, on))
            return;
        if (on != rfidEnabled)
            setRfid(on);
    }
}

void sendDiscoveryConfig(const char *component, const char *object_id, String configJson)
{
    if (!client.connected())
        return;
    char topic[192];
    snprintf(topic, sizeof(topic), "homeassistant/%s/%s/%s/config", component, DEVICE_ID, object_id);
    String finalJson = "{\"unique_id\":\"" + String(DEVICE_ID) + "_" + String(object_id) + "\"," + configJson.substring(1);

    client.publish(topic, finalJson.c_str(), true);
    Serial.printf("[DISCOVERY] Sent: %s\n", object_id);

    client.loop();
}

void setupMQTTDiscovery()
{
    Serial.println("[MQTT] Gui cau hinh Auto-Discovery...");
    String deviceInfo = "\"device\":{\"identifiers\":[\"" + String(DEVICE_ID) + "\"],\"name\":\"" + String(DEVICE_NAME) + "\",\"manufacturer\":\"Espressif\",\"model\":\"ESP32 DevKit\",\"sw_version\":\"2.3.0\"}";
    String avail = "\"availability_topic\":\"" + String(TOPIC_STATUS) + "\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\"";

    sendDiscoveryConfig("light", "den", "{\"name\":\"Den Phong Khach\",\"state_topic\":\"" + String(relayDenPK.stateTopic) + "\",\"command_topic\":\"" + String(relayDenPK.ctrlTopic) + "\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"icon\":\"mdi:sofa\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("switch", "quat", "{\"name\":\"Quat Phong\",\"state_topic\":\"" + String(relayQuat.stateTopic) + "\",\"command_topic\":\"" + String(relayQuat.ctrlTopic) + "\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"icon\":\"mdi:fan\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("lock", "door", "{\"name\":\"Cua Ra Vao (Servo)\",\"state_topic\":\"" + String(TOPIC_DOOR_STATE) + "\",\"command_topic\":\"" + String(TOPIC_DOOR_CTRL) + "\",\"payload_lock\":\"CLOSE\",\"payload_unlock\":\"OPEN\",\"state_locked\":\"CLOSED\",\"state_unlocked\":\"OPEN\",\"icon\":\"mdi:door\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("sensor", "ldr", "{\"name\":\"Anh Sang Phong Khach\",\"state_topic\":\"" + String(TOPIC_LDR_STATE) + "\",\"unit_of_measurement\":\"%\",\"icon\":\"mdi:brightness-5\",\"value_template\":\"{{ (100 - (value | int / 4095 * 100)) | round(1) }}\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("binary_sensor", "pir", "{\"name\":\"Chuyen Dong Phong Khach\",\"state_topic\":\"" + String(TOPIC_PIR_STATE) + "\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"device_class\":\"motion\",\"icon\":\"mdi:motion-sensor\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("sensor", "temp", "{\"name\":\"Nhiet Do Phong Khach\",\"state_topic\":\"" + String(TOPIC_DHT_STATE) + "\",\"unit_of_measurement\":\"°C\",\"icon\":\"mdi:thermometer\",\"value_template\":\"{{ value_json.temp }}\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("sensor", "hum", "{\"name\":\"Do Am Phong Khach\",\"state_topic\":\"" + String(TOPIC_DHT_STATE) + "\",\"unit_of_measurement\":\"%\",\"icon\":\"mdi:water-percent\",\"value_template\":\"{{ value_json.hum }}\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("sensor", "flame", "{\"name\":\"Canh Bao Lua\",\"state_topic\":\"" + String(TOPIC_FLAME_STATE) + "\",\"unit_of_measurement\":\"%\",\"icon\":\"mdi:fire\",\"value_template\":\"{{ value | int }}\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("sensor", "mq2", "{\"name\":\"Nong Do Khoi va Gas\",\"state_topic\":\"" + String(TOPIC_MQ2_STATE) + "\",\"unit_of_measurement\":\"%\",\"icon\":\"mdi:smoke-detector\",\"device_class\":\"smoke\",\"value_template\":\"{{ value | int }}\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("binary_sensor", "door_contact", "{\"name\":\"Trang Thai Cua Vat Ly\",\"state_topic\":\"" + String(TOPIC_DOOR_SENSOR_STATE) + "\",\"payload_on\":\"OPEN\",\"payload_off\":\"CLOSED\",\"device_class\":\"door\",\"icon\":\"mdi:door-open\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("sensor", "rfid_uid", "{\"name\":\"The RFID Vua Quet\",\"state_topic\":\"" + String(TOPIC_EVENT) + "\",\"value_template\":\"{{ value_json.uid }}\",\"icon\":\"mdi:card-account-details\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("sensor", "rfid_access", "{\"name\":\"Ket Qua Quet The\",\"state_topic\":\"" + String(TOPIC_EVENT) + "\",\"value_template\":\"{{ value_json.access }}\",\"icon\":\"mdi:shield-account\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("switch", "rfid_enable", "{\"name\":\"Dau Doc The RFID\",\"state_topic\":\"" + String(TOPIC_RFID_STATE) + "\",\"command_topic\":\"" + String(TOPIC_RFID_CTRL) + "\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"icon\":\"mdi:credit-card-wireless\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("light", "den_ngu", "{\"name\":\"Den Phong Ngu\",\"state_topic\":\"" + String(TOPIC_RELAY_DEN_NGU_STATE) + "\",\"command_topic\":\"" + String(TOPIC_RELAY_DEN_NGU_CTRL) + "\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"icon\":\"mdi:bed\"," + avail + "," + deviceInfo + "}");
    sendDiscoveryConfig("switch", "buzzer", "{\"name\":\"Buzzer Bao Dong\",\"state_topic\":\"" + String(TOPIC_BUZZER_STATE) + "\",\"command_topic\":\"" + String(TOPIC_BUZZER_CTRL) + "\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"icon\":\"mdi:alarm-bell\"," + avail + "," + deviceInfo + "}");

    Serial.println("[MQTT] Hoan tat Auto-Discovery!");
}

void mqttHeartbeat()
{
    static unsigned long last = 0;
    if (!client.connected())
        return;
    if (millis() - last >= MQTT_STATUS_HEARTBEAT)
    {
        last = millis();
        client.publish(TOPIC_STATUS, "online", true);
    }
}

void handleOTA()
{
    if (pendingOtaPayload.length() == 0)
        return;
    String p = pendingOtaPayload;
    pendingOtaPayload = "";
    checkForOTAUpdate(p);
}

void checkForOTAUpdate(String payload)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.println(">>> OTA: Loi parse JSON payload!");
        return;
    }

    String url = doc["url"].as<String>();
    String version = doc["version"].as<String>();

    if (url.length() > 0)
    {
        Serial.printf(">>> OTA: Nhan lenh cap nhat! Version: %s\n", version.c_str());
        Serial.println(">>> OTA: Dang tai firmware... TUYET DOI KHONG NGAT DIEN!");

        WiFiClientSecure clientSecure;
        clientSecure.setInsecure();

        // GitHub Releases trả về redirect 302 -> phải cho phép follow redirect
        httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        httpUpdate.rebootOnUpdate(true);

        t_httpUpdate_return ret = httpUpdate.update(clientSecure, url);

        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            Serial.printf(">>> OTA: Cap nhat that bai (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println(">>> OTA: Khong co ban cap nhat moi.");
            break;
        case HTTP_UPDATE_OK:
            Serial.println(">>> OTA: Cap nhat thanh cong! Dang khoi dong lai...");
            break;
        }
    }
}