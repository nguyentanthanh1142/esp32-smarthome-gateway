#ifndef CONFIG_H
#define CONFIG_H
#include "secret.h" 

// ===== Cấu hình phần cứng =====
#define RELAY_DEN_PK_PIN 25
#define RELAY_QUAT_PIN 27
#define SERVO_DOOR_PIN 32
#define RST_PIN 21
#define LDR_PIN 34
#define PIR_PIN 33
#define DHT_PIN 26
#define FLAME_PIN 35
#define MQ2_PIN 39
#define DOOR_SENSOR_PIN 14

#define RELAY_ACTIVE_LOW 0
#define RELAY_ON_LEVEL (RELAY_ACTIVE_LOW ? LOW : HIGH)
#define RELAY_OFF_LEVEL (RELAY_ACTIVE_LOW ? HIGH : LOW)

// ===== Servo 360 =====
#define SERVO_STOP_US 1500
#define SERVO_OPEN_US 1000
#define SERVO_CLOSE_US 2000
#define DOOR_SPIN_MS 400

// ===== Interval =====
#define LDR_PUBLISH_INTERVAL 5000
#define DHT_PUBLISH_INTERVAL 5000
#define FLAME_PUBLISH_INTERVAL 1000
#define MQ2_PUBLISH_INTERVAL 2000
#define MQTT_RECONNECT_INTERVAL 5000
#define ECHO_TIMEOUT_MS 3000
#define MAX_ECHO 8


// ===== Topics =====
#define TOPIC_STATUS "esp32/gateway_02/status"
#define MQTT_STATUS_HEARTBEAT 15000
#define TOPIC_RFID_STATE "esp32/rfid/state"
#define TOPIC_RFID_CTRL "esp32/rfid/control"
#define TOPIC_EVENT "esp32/rfid/event"
#define TOPIC_DOOR_STATE "esp32/door/state"
#define TOPIC_DOOR_CTRL "esp32/door/control"
#define TOPIC_LDR_STATE "esp32/sensor/ldr/state"
#define TOPIC_PIR_STATE "esp32/sensor/pir/state"
#define TOPIC_DHT_STATE "esp32/sensor/dht/state"
#define TOPIC_FLAME_STATE "esp32/sensor/flame/state"
#define TOPIC_MQ2_STATE "esp32/sensor/mq2/state"
#define TOPIC_DOOR_SENSOR_STATE "esp32/sensor/door_contact/state"
#define TOPIC_OTA_UPDATE "esp32/gateway_02/ota/update"
#define CURRENT_FW_VERSION "v2.3.0"

// ===== Device Info =====
#define DEVICE_ID "esp32_gateway_02"
#define DEVICE_NAME "ESP32 Gateway 02"

// ===== Configuration Servo 360 =====
#define SERVO_STOP_US 1500UL  
#define SERVO_OPEN_US 1000UL  
#define SERVO_CLOSE_US 2000UL 
#define DOOR_SPIN_MS 400UL    
#endif