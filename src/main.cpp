#include "globals.h"
#include "hardware.h"
#include "sensors.h"
#include "mqtt_handler.h"
#include "rfid_handler.h"

MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};
Servo doorServo;
DHTesp dht;
WiFiClient espClient;
PubSubClient client(espClient);

DoorState doorState = IDLE_CLOSED;
unsigned long doorActionStart = 0;
bool rfidEnabled = true;
unsigned long scanCount = 0;
bool subscribed = false;

unsigned long lastLdrPublish = 0;
unsigned long lastDhtPublish = 0;
unsigned long lastFlamePublish = 0;
unsigned long lastMq2Publish = 0;
bool lastPirState = false;
bool lastDoorSensorState = false;
unsigned long lastMqttReconnectAttempt = 0;
const char *ALLOWED_UIDS[] = {nullptr};

RelayChannel relays[] = {
    {"den_pk", RELAY_DEN_PK_PIN, "esp32/relay/den/state", "esp32/relay/den/control", false, false, 0},
    {"quat", RELAY_QUAT_PIN, "esp32/relay/quat/state", "esp32/relay/quat/control", false, false, 0}};
const int RELAY_COUNT = sizeof(relays) / sizeof(relays[0]);
RelayChannel &relayDenPK = relays[0];
RelayChannel &relayQuat = relays[1];

PendingEcho echoes[MAX_ECHO];
int echoCount = 0;

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32 Smart Gateway v2.2.0 ===");

  initHardware();
  initSensors();
  initRFID();

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  bool res = wm.autoConnect("ESP32_Gateway_AP", "12345678");
  if (!res)
  {
    Serial.println("Ket noi WiFi that bai. Dang khoi dong lai...");
    ESP.restart();
  }
  Serial.println("\nWiFi connected!");
  Serial.print(">>> IP: ");
  Serial.println(WiFi.localIP());

  initMQTT();
  configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");

  ArduinoOTA.setHostname("esp32_gateway_02");
  ArduinoOTA.begin();
  Serial.println(">>> System Ready!\n");
}

unsigned long lastWifiReconnectAttempt = 0;
const unsigned long WIFI_RECONNECT_INTERVAL = 10000;

void loop()
{
  ArduinoOTA.handle();
  checkCard();
  handleTimers();
  handleLDR();
  handlePIR();
  handleDHT();
  handleFlame();
  handleMQ2();
  handleDoorSensor();

  // NETWORK & MQTT (Non-blocking)
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!client.connected())
    {
      reconnect();
    }
    client.loop();
    mqttHeartbeat();
  }
  else
  {
    unsigned long now = millis();
    if (now - lastWifiReconnectAttempt >= WIFI_RECONNECT_INTERVAL)
    {
      lastWifiReconnectAttempt = now;
      Serial.println("[WARN] WiFi mat ket noi, dang thu ket noi lai...");
      WiFi.disconnect();
      WiFi.reconnect();
    }
  }
}