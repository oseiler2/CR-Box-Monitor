
#include <configManager.h>

#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Local logging tag
static const char TAG[] = __FILE__;

Config config;

// Allocate a temporary JsonDocument
// Don't forget to change the capacity to match your requirements.
// Use arduinojson.org/v6/assistant to compute the capacity.
/*
{
  "deviceId": 65535,
  "mqttTopic": "123456789112345678921",
  "mqttUsername": "123456789112345678921",
  "mqttPassword": "123456789112345678921",
  "mqttHost": "1234567891123456789212345678931",
  "mqttUseTls": false,
  "mqttInsecure": false,
  "mqttServerPort": 65535,
  "mqttMirrorDevice" false,
  "mqttMirrordeviceId": 65535,
  "mqttMirrorTopic": "123456789112345678921",
  "altitude": 12345,
  "co2GreenThreshold": 0,
  "co2YellowThreshold": 800,
  "co2RedThreshold": 1000,
  "co2DarkRedThreshold": 2000,
  "brightness": 255,
  "buzzerMode": 0,
  "ssd1306Rows": 64,
  "neopixelIntData": 17,
  "neopixelIntNumber": 6,
  "neopixelExtData": 32,
  "neopixelIExNumber": 9,
  "sleepModeOledLed": 0,
  "fanHasPwm": false,
  "minPwm": 30
}
*/

#define DEFAULT_DEVICE_ID                          0
#define DEFAULT_MQTT_TOPIC                   "crbox"
#define DEFAULT_MQTT_HOST                "127.0.0.1"
#define DEFAULT_MQTT_PORT                       1883
#define DEFAULT_MQTT_USERNAME                "crbox"
#define DEFAULT_MQTT_PASSWORD                "crbox"
#define DEFAULT_MQTT_USE_TLS                   false
#define DEFAULT_MQTT_INSECURE                  false
#define DEFAULT_MQTT_MIRROR                    false
#define DEFAULT_MQTT_MIRROR_DEVICE_ID              0
#define DEFAULT_MQTT_MIRROR_TOPIC       "co2monitor"
#define DEFAULT_ALTITUDE                           5
#define DEFAULT_CO2_GREEN_THRESHOLD              420
#define DEFAULT_CO2_YELLOW_THRESHOLD             700
#define DEFAULT_CO2_RED_THRESHOLD                900
#define DEFAULT_CO2_DARK_RED_THRESHOLD          1200
#define DEFAULT_BRIGHTNESS                       255
#define DEFAULT_COLOURWHEEL                    false
#define DEFAULT_NEOPIXEL_INT_DATA       NEO_DATA_INT
#define DEFAULT_NEOPIXEL_INT_NUMBER                9
#define DEFAULT_NEOPIXEL_EXT_DATA       NEO_DATA_EXT
#define DEFAULT_NEOPIXEL_EXT_NUMBER               32
#define DEFAULT_FAN_HAS_PWM                    false
#define DEFAULT_MIN_PWM                           30
#define DEFAULT_BUZZER_MODE                  BUZ_OFF

std::vector<ConfigParameterBase<Config>*> configParameterVector;

const char* BUZZER_MODE_STRINGS[] = {
  "Buzzer off",
  "Buzzer when level changes",
  "Buzzer always on",
};


void setupConfigManager() {
  if (!LittleFS.begin(true)) {
    ESP_LOGW(TAG, "LittleFS failed! Already tried formatting.");
    if (!LittleFS.begin()) {
      delay(100);
      ESP_LOGW(TAG, "LittleFS failed second time!");
    }
  }
  //  configParameterVector.clear();
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("deviceId", "Device ID", &Config::deviceId, DEFAULT_DEVICE_ID));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttTopic", "MQTT topic", (char Config::*) & Config::mqttTopic, DEFAULT_MQTT_TOPIC, MQTT_TOPIC_LEN));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttUsername", "MQTT username", (char Config::*) & Config::mqttUsername, DEFAULT_MQTT_USERNAME, MQTT_USERNAME_LEN));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttPassword", "MQTT password", (char Config::*) & Config::mqttPassword, DEFAULT_MQTT_PASSWORD, MQTT_PASSWORD_LEN));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttHost", "MQTT host", (char Config::*) & Config::mqttHost, DEFAULT_MQTT_HOST, MQTT_HOSTNAME_LEN));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("mqttServerPort", "MQTT port", &Config::mqttServerPort, DEFAULT_MQTT_PORT));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("mqttUseTls", "MQTT use TLS", &Config::mqttUseTls, DEFAULT_MQTT_USE_TLS));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("mqttInsecure", "MQTT ignore certificate errors", &Config::mqttInsecure, DEFAULT_MQTT_INSECURE));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("mqttMirror", "Mirror other device's measurements", &Config::mqttMirrorDevice, DEFAULT_MQTT_MIRROR, true));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("mqttMirrordeviceId", "Id of device to mirror", &Config::mqttMirrordeviceId, DEFAULT_MQTT_MIRROR_DEVICE_ID, true));
  configParameterVector.push_back(new CharArrayConfigParameter<Config>("mqttMirrorTopic", "MQTT topic of device to mirror", (char Config::*) & Config::mqttMirrorTopic, DEFAULT_MQTT_MIRROR_TOPIC, MQTT_TOPIC_LEN, true));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("altitude", "Altitude", &Config::altitude, DEFAULT_ALTITUDE, 0, 8000));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("co2GreenThreshold", "CO2 Green threshold ", &Config::co2GreenThreshold, DEFAULT_CO2_GREEN_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("co2YellowThreshold", "CO2 Yellow threshold ", &Config::co2YellowThreshold, DEFAULT_CO2_YELLOW_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("co2RedThreshold", "CO2 Red threshold", &Config::co2RedThreshold, DEFAULT_CO2_RED_THRESHOLD));
  configParameterVector.push_back(new Uint16ConfigParameter<Config>("co2DarkRedThreshold", "CO2 Dark red threshold", &Config::co2DarkRedThreshold, DEFAULT_CO2_DARK_RED_THRESHOLD));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("brightness", "LED brightness pwm", &Config::brightness, DEFAULT_BRIGHTNESS));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("colourWheel", "Display colourwheel", &Config::colourWheel, DEFAULT_COLOURWHEEL));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("neopixelIntData", "Neopixel internel data pin", &Config::neopixelIntData, DEFAULT_NEOPIXEL_INT_DATA, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("neopixelIntNumber", "Number of internal Neopixels", &Config::neopixelIntNumber, DEFAULT_NEOPIXEL_INT_NUMBER, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("neopixelExtData", "Neopixel external data pin", &Config::neopixelExtData, DEFAULT_NEOPIXEL_EXT_DATA, true));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("neopixelExtNumber", "Number of external Neopixels", &Config::neopixelExtNumber, DEFAULT_NEOPIXEL_EXT_NUMBER, true));
  configParameterVector.push_back(new BooleanConfigParameter<Config>("fanHasPwm", "Fans use 4 pin conn with PWM", &Config::fanHasPwm, DEFAULT_FAN_HAS_PWM));
  configParameterVector.push_back(new Uint8ConfigParameter<Config>("minPwm", "PWM when CO2 is low", &Config::minPwm, DEFAULT_MIN_PWM, 20, 255));
  configParameterVector.push_back(new EnumConfigParameter<Config, uint8_t, BuzzerMode>("buzzerMode", "Buzzer mode", &Config::buzzerMode, DEFAULT_BUZZER_MODE, BUZZER_MODE_STRINGS, BUZ_OFF, BUZ_ALWAYS));
}

std::vector<ConfigParameterBase<Config>*> getConfigParameters() {
  return configParameterVector;
}

void getDefaultConfiguration(Config& _config) {
  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    configParameter->setToDefault(_config);
  }
}

void logConfiguration(const Config _config) {
  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    ESP_LOGD(TAG, "%s: %s", configParameter->getId(), configParameter->toString(_config).c_str());
  }
}

boolean loadConfiguration(Config& _config) {
  File file = LittleFS.open(CONFIG_FILENAME, FILE_READ);
  if (!file) {
    ESP_LOGW(TAG, "Could not open config file");
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  DynamicJsonDocument* doc = new DynamicJsonDocument(CONFIG_SIZE);

  DeserializationError error = deserializeJson(*doc, file);
  if (error) {
    ESP_LOGW(TAG, "Failed to parse config file: %s", error.f_str());
    file.close();
    return false;
  }

  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    configParameter->fromJson(_config, doc, true);
  }

  file.close();
  return true;
}

boolean saveConfiguration(const Config _config) {
  ESP_LOGD(TAG, "###################### saveConfiguration");
  logConfiguration(_config);
  // Delete existing file, otherwise the configuration is appended to the file
  if (LittleFS.exists(CONFIG_FILENAME)) {
    LittleFS.remove(CONFIG_FILENAME);
  }

  // Open file for writing
  File file = LittleFS.open(CONFIG_FILENAME, FILE_WRITE);
  if (!file) {
    ESP_LOGW(TAG, "Could not create config file for writing");
    return false;
  }

  DynamicJsonDocument* doc = new DynamicJsonDocument(CONFIG_SIZE);
  for (ConfigParameterBase<Config>* configParameter : configParameterVector) {
    configParameter->toJson(_config, doc);
  }

  // Serialize JSON to file
  if (serializeJson(*doc, file) == 0) {
    ESP_LOGW(TAG, "Failed to write to file");
    file.close();
    return false;
  }

  // Close the file
  file.close();
  ESP_LOGD(TAG, "Stored configuration successfully");
  return true;
}

// Prints the content of a file to the Serial
void printFile() {
  // Open file for reading
  File file = LittleFS.open(CONFIG_FILENAME, FILE_READ);
  if (!file) {
    ESP_LOGW(TAG, "Could not open config file");
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}

BuzzerMode getBuzzerModeFromUint(uint8_t buzzerMode) {
  switch (buzzerMode) {
    case BUZ_LVL_CHANGE: return BUZ_LVL_CHANGE;
    case BUZ_ALWAYS: return BUZ_ALWAYS;
    default: return BUZ_OFF;
  }
}

