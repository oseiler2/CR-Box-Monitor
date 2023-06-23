#include <logging.h>
#include <globals.h>
#include <Arduino.h>
#include <config.h>

#include <WiFi.h>
#include <Wire.h>
#include <i2c.h>
#include <esp_event.h>
#include <esp_err.h>
#include <esp_task_wdt.h>
#include <rom/rtc.h>

#include <configManager.h>
#include <mqtt.h>
#include <sensors.h>
#include <scd30.h>
#include <scd40.h>
#include <sps_30.h>
#include <housekeeping.h>
#include <neopixel.h>
#include <buzzer.h>
#include <wifiManager.h>
#include <ota.h>
#include <model.h>
#include <fan.h>

// Local logging tag
static const char TAG[] = __FILE__;

Model* model;
Neopixel* neopixel;
Buzzer* buzzer;
SCD30* scd30;
SCD40* scd40;
SPS_30* sps30;
Fan* fan;
TaskHandle_t sensorsTask;
TaskHandle_t wifiManagerTask;

bool hasLEDs = false;
bool hasNeoPixel = false;
bool hasBuzzer = false;
bool hasBattery = false;


const uint32_t debounceDelay = 50;
volatile uint32_t lastBtn1DebounceTime = 0;
volatile uint8_t button1State = 0;
uint8_t oldConfirmedButton1State = 0;
uint32_t lastConfirmedBtn1PressedTime = 0;

void ICACHE_RAM_ATTR button1Handler() {
  button1State = (digitalRead(BTN_1) ? 0 : 1);
  lastBtn1DebounceTime = millis();
}

void prepareOta() {}

void updateMessage(char const* msg) {}

void setPriorityMessage(char const* msg) {}

void clearPriorityMessage() {}

void modelUpdatedEvt(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (hasNeoPixel && neopixel) neopixel->update(mask, oldStatus, newStatus);
  if (hasBuzzer && buzzer) buzzer->update(mask, oldStatus, newStatus);
  if (fan) fan->update(mask, oldStatus, newStatus);
  if ((mask & M_PRESSURE) && I2C::scd40Present() && scd40) scd40->setAmbientPressure(model->getPressure());
  if ((mask & M_PRESSURE) && I2C::scd30Present() && scd30) scd30->setAmbientPressure(model->getPressure());
  if ((mask & ~M_CONFIG_CHANGED) != M_NONE) {
    char buf[8];
    DynamicJsonDocument* doc = new DynamicJsonDocument(512);
    if (mask & M_CO2) (*doc)["co2"] = model->getCo2();
    if (mask & M_TEMPERATURE) {
      sprintf(buf, "%.1f", model->getTemperature());
      (*doc)["temperature"] = buf;
    }
    if (mask & M_HUMIDITY) {
      sprintf(buf, "%.1f", model->getHumidity());
      (*doc)["humidity"] = buf;
    }
    if (mask & M_PRESSURE) (*doc)["pressure"] = model->getPressure();
    if (mask & M_IAQ) (*doc)["iaq"] = model->getIAQ();
    if (mask & M_PM0_5) (*doc)["pm0.5"] = model->getPM0_5();
    if (mask & M_PM1_0) (*doc)["pm1"] = model->getPM1();
    if (mask & M_PM2_5) (*doc)["pm2.5"] = model->getPM2_5();
    if (mask & M_PM4) (*doc)["pm4"] = model->getPM4();
    if (mask & M_PM10) (*doc)["pm10"] = model->getPM10();
    mqtt::publishSensors(doc);
  }
}

void configChanged() {
  model->configurationChanged();
}

void calibrateCo2SensorCallback(uint16_t co2Reference) {
  ESP_LOGI(TAG, "Starting calibration");
  if (I2C::scd30Present() && scd30) scd30->calibrateScd30ToReference(co2Reference);
  if (I2C::scd40Present() && scd40) scd40->calibrateScd40ToReference(co2Reference);
  vTaskDelay(pdMS_TO_TICKS(200));
}

void setTemperatureOffsetCallback(float temperatureOffset) {
  if (I2C::scd30Present() && scd30) scd30->setTemperatureOffset(temperatureOffset);
  if (I2C::scd40Present() && scd40) scd40->setTemperatureOffset(temperatureOffset);
}

float getTemperatureOffsetCallback() {
  if (I2C::scd30Present() && scd30) return scd30->getTemperatureOffset();
  if (I2C::scd40Present() && scd40) return scd40->getTemperatureOffset();
  return NaN;
}

uint32_t getSPS30AutoCleanInterval() {
  if (I2C::sps30Present && sps30) return sps30->getAutoCleanInterval();
  return 0;
}

boolean setSPS30AutoCleanInterval(uint32_t intervalInSeconds) {
  if (I2C::sps30Present && sps30) return sps30->setAutoCleanInterval(intervalInSeconds);
  return false;
}

boolean cleanSPS30() {
  if (I2C::sps30Present && sps30) return sps30->clean();
  return false;
}

uint8_t getSPS30Status() {
  if (I2C::sps30Present && sps30) return sps30->getStatus();
  return false;
}

void logCoreInfo() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  ESP_LOGI(TAG,
    "This is ESP32 chip with %d CPU cores, WiFi%s%s, silicon revision "
    "%d, %dMB %s Flash",
    chip_info.cores,
    (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
    (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
    chip_info.revision, spi_flash_get_chip_size() / (1024 * 1024),
    (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded"
    : "external");
  ESP_LOGI(TAG, "Internal Total heap %d, internal Free Heap %d",
    ESP.getHeapSize(), ESP.getFreeHeap());
#ifdef BOARD_HAS_PSRAM
  ESP_LOGI(TAG, "SPIRam Total heap %d, SPIRam Free Heap %d",
    ESP.getPsramSize(), ESP.getFreePsram());
#endif
  ESP_LOGI(TAG, "ChipRevision %d, Cpu Freq %d, SDK Version %s",
    ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  ESP_LOGI(TAG, "Flash Size %d, Flash Speed %d", ESP.getFlashChipSize(),
    ESP.getFlashChipSpeed());
}

void setup() {
  esp_task_wdt_init(20, true);

  if (LED_PIN >= 0) {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
  }
  pinMode(BTN_1, INPUT_PULLUP);
  Serial.begin(115200);
  esp_log_set_vprintf(logging::logger);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  ESP_LOGI(TAG, "CO2 Monitor v%s. Built from %s @ %s", APP_VERSION, SRC_REVISION, BUILD_TIMESTAMP);

  model = new Model(modelUpdatedEvt);

  logCoreInfo();

  RESET_REASON resetReason = rtc_get_reset_reason(0);

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, WifiManager::eventHandler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, WifiManager::eventHandler, NULL, NULL));

  setupConfigManager();
  if (!loadConfiguration(config)) {
    getDefaultConfiguration(config);
    saveConfiguration(config);
  }
  logConfiguration(config);

  WifiManager::setupWifiManager("CR-Box", getConfigParameters(), false, true,
    updateMessage, setPriorityMessage, clearPriorityMessage, configChanged);

  hasNeoPixel = (config.neopixelIntData != 0 && config.neopixelIntNumber != 0);
  hasBuzzer = config.buzzerPin != 0;

  if (hasNeoPixel) {
    pinMode(config.neopixelIntData, OUTPUT);
    digitalWrite(config.neopixelIntData, LOW);
  }
  if (hasBuzzer) {
    pinMode(config.buzzerPin, OUTPUT);
    digitalWrite(config.buzzerPin, LOW);
  }

  Wire.begin((int)SDA_PIN, (int)SCL_PIN, (uint32_t)I2C_CLK);

  I2C::initI2C();

  if (I2C::scd30Present()) scd30 = new SCD30(&Wire, model, updateMessage);
  if (I2C::scd40Present()) scd40 = new SCD40(&Wire, model, updateMessage);
  if (I2C::sps30Present()) sps30 = new SPS_30(&Wire, model, updateMessage);

  if (hasNeoPixel) neopixel = new Neopixel(model, config.neopixelIntData, config.neopixelIntNumber);
  if (hasBuzzer) buzzer = new Buzzer(model, config.buzzerPin);
  fan = new Fan(model);

  mqtt::setupMqtt(
    "CrBox",
    model,
    calibrateCo2SensorCallback,
    setTemperatureOffsetCallback,
    getTemperatureOffsetCallback,
    getSPS30AutoCleanInterval,
    setSPS30AutoCleanInterval,
    cleanSPS30,
    getSPS30Status,
    configChanged);

  char msg[128];
  sprintf(msg, "Reset reason: %u", resetReason);
  mqtt::publishStatusMsg(msg);

  xTaskCreatePinnedToCore(mqtt::mqttLoop,  // task function
    "mqttLoop",         // name of task
    8192,               // stack size of task
    (void*)1,           // parameter of the task
    2,                  // priority of the task
    &mqtt::mqttTask,    // task handle
    0);                 // CPU core

  xTaskCreatePinnedToCore(OTA::otaLoop,  // task function
    "otaLoop",          // name of task
    8192,               // stack size of task
    (void*)1,           // parameter of the task
    2,                  // priority of the task
    &OTA::otaTask,      // task handle
    1);                 // CPU core

  if (scd30 || scd40 || sps30) {
    Sensors::setupSensorsLoop(scd30, scd40, sps30);
    sensorsTask = Sensors::start(
      "sensorsLoop",      // name of task
      4096,               // stack size of task
      2,                  // priority of the task
      1);                 // CPU core
  }

  wifiManagerTask = WifiManager::start(
    "wifiManagerLoop",  // name of task
    8192,               // stack size of task
    2,                  // priority of the task
    1);                 // CPU core

  housekeeping::cyclicTimer.attach(30, housekeeping::doHousekeeping);

  OTA::setupOta(prepareOta, setPriorityMessage, clearPriorityMessage);

  attachInterrupt(BTN_1, button1Handler, CHANGE);

  ESP_LOGI(TAG, "Setup done.");
}

void loop() {
  if (button1State != oldConfirmedButton1State && (millis() - lastBtn1DebounceTime) > debounceDelay) {
    oldConfirmedButton1State = button1State;
    if (oldConfirmedButton1State == 1) {
      lastConfirmedBtn1PressedTime = millis();
    } else if (oldConfirmedButton1State == 0) {
      uint32_t btnPressTime = millis() - lastConfirmedBtn1PressedTime;
      ESP_LOGD(TAG, "lastConfirmedBtnPressedTime - millis() %u", btnPressTime);
      if (btnPressTime < 2000) {
        if (LED_PIN >= 0) digitalWrite(LED_PIN, LOW);
        prepareOta();
        WifiManager::startCaptivePortal();
      } else if (btnPressTime > 5000) {
        calibrateCo2SensorCallback(420);
      }
    }
  }
  vTaskDelay(pdMS_TO_TICKS(50));
}