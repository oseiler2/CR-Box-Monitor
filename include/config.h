#ifndef _CONFIG_H
#define _CONFIG_H

#include <logging.h>
#include <sdkconfig.h>

#define OTA_URL               "https://otahost/crbox/firmware.json"
#define OTA_APP               "crbox"
//#define OTA_POLL


#if CONFIG_IDF_TARGET_ESP32

#define LED_PIN                2
#define BUZZER_PIN             4
#define BTN_1                  0
#define NEO_DATA_INT          16
#define NEO_DATA_EXT          17
#define FAN_HALL              25
#define FAN_PWM               26
#define FAN_EN                27
#define SDA_PIN              SDA
#define SCL_PIN              SCL
#define SCD30_RDY_PIN         35

#elif CONFIG_IDF_TARGET_ESP32S3

#define LED_PIN                2
#define BUZZER_PIN             1
#define BTN_1                  0
#define NEO_DATA_INT          17
#define NEO_DATA_EXT           6
#define XTAL_32k_1            15
#define XTAL_32k_2            16
#define FAN_HALL               4
#define FAN_PWM                5
#define FAN_EN                 9
#define SDA_PIN               14
#define SCL_PIN               21
#define SCD30_RDY_PIN          3

#endif

#define I2C_CLK 100000UL
#define SCD30_I2C_CLK 50000UL   // SCD30 recommendation of 50kHz

static const char* CONFIG_FILENAME = "/config.json";
static const char* MQTT_ROOT_CA_FILENAME = "/mqtt_root_ca.pem";
static const char* MQTT_CLIENT_CERT_FILENAME = "/mqtt_client_cert.pem";
static const char* MQTT_CLIENT_KEY_FILENAME = "/mqtt_client_key.pem";
static const char* TEMP_MQTT_ROOT_CA_FILENAME = "/temp_mqtt_root_ca.pem";
static const char* ROOT_CA_FILENAME = "/root_ca.pem";

#define MQTT_QUEUE_LENGTH      25

#define PWM_CHANNEL_FAN         0
#define PWM_CHANNEL_BUZZER      2

// ----------------------------  Config struct ------------------------------------- 
#define CONFIG_SIZE 1280

#define MQTT_USERNAME_LEN 20
#define MQTT_PASSWORD_LEN 20
#define MQTT_HOSTNAME_LEN 30
#define MQTT_TOPIC_LEN 30
#define SSID_LEN 32
#define WIFI_PASSWORD_LEN 64

typedef enum : uint8_t {
  BUZ_OFF = 0,
  BUZ_LVL_CHANGE,
  BUZ_ALWAYS
} BuzzerMode;

struct Config {
  uint16_t deviceId;
  char mqttTopic[MQTT_TOPIC_LEN + 1];
  char mqttUsername[MQTT_USERNAME_LEN + 1];
  char mqttPassword[MQTT_PASSWORD_LEN + 1];
  char mqttHost[MQTT_HOSTNAME_LEN + 1];
  bool mqttUseTls;
  bool mqttInsecure;
  bool mqttMirrorDevice;
  uint16_t mqttMirrordeviceId;
  char mqttMirrorTopic[MQTT_TOPIC_LEN + 1];
  uint16_t mqttServerPort;
  uint16_t altitude;
  uint16_t co2GreenThreshold;
  uint16_t co2YellowThreshold;
  uint16_t co2RedThreshold;
  uint16_t co2DarkRedThreshold;
  uint8_t brightness;
  bool colourWheel;
  uint8_t neopixelIntData;
  uint8_t neopixelIntNumber;
  uint8_t neopixelExtData;
  uint8_t neopixelExtNumber;
  uint8_t minPwm;
  uint8_t buzzerPin = BUZZER_PIN;
  bool fanHasPwm;
  BuzzerMode buzzerMode = BUZ_LVL_CHANGE;
};

#endif
