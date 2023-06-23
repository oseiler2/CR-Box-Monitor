#include <neopixel.h>
#include <config.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = __FILE__;

Neopixel::Neopixel(Model* _model, uint8_t _pin, uint8_t numPixel) {
  this->model = _model;
  intStrip = new Adafruit_NeoPixel(numPixel, _pin, NEO_GRB + NEO_KHZ800);
  extStrip = new Adafruit_NeoPixel(config.neopixelExtNumber, config.neopixelExtData, NEO_GRB + NEO_KHZ800);
  toggle = false;
  ticker = new Ticker();
  wheelPos = 0;

  this->colourRed = this->intStrip->Color(255, 0, 0);
  this->colourYellow = this->intStrip->Color(255, 70, 0);
  this->colourGreen = this->intStrip->Color(0, 255, 0);
  this->colourPurple = this->intStrip->Color(255, 0, 255);
  this->colourOff = this->intStrip->Color(0, 0, 0);

  this->intStrip->begin();
  this->extStrip->begin();
  this->intStrip->setBrightness(config.brightness);
  this->extStrip->setBrightness(config.brightness);

  // https://stackoverflow.com/questions/60985496/arduino-esp8266-esp32-ticker-callback-class-member-function
  //ticker->attach(0.3, +[](Neopixel* instance) { instance->timer(); }, this);
  fill(colourPurple);
  delay(250);
  fill(colourRed);
  delay(250);
  fill(colourYellow);
  delay(250);
  fill(colourGreen);
  delay(250);
  fill(colourOff);
  if (config.colourWheel) {
    ticker->attach(0.3, +[](Neopixel* instance) { instance->wheelTimer(); }, this);
  }
}

Neopixel::~Neopixel() {
  if (this->ticker) delete ticker;
  if (this->intStrip) delete intStrip;
  if (this->extStrip) delete extStrip;
}

void Neopixel::fill(uint32_t c) {
  for (uint16_t i = 0; i < this->intStrip->numPixels(); i++) {
    this->intStrip->setPixelColor(i, c);
  }
  for (uint16_t i = 0; i < this->extStrip->numPixels(); i++) {
    this->extStrip->setPixelColor(i, c);
  }
  this->intStrip->show();
  this->extStrip->show();
}

void Neopixel::off() {
  this->intStrip->setBrightness(0);
  this->extStrip->setBrightness(0);
  for (uint16_t i = 0; i < this->intStrip->numPixels(); i++) {
    this->intStrip->setPixelColor(i, colourOff);
  }
  for (uint16_t i = 0; i < this->extStrip->numPixels(); i++) {
    this->extStrip->setPixelColor(i, colourOff);
  }
  this->intStrip->show();
  this->extStrip->show();
}

void Neopixel::prepareToSleep() {
  ticker->detach();
  if (model->getStatus() == DARK_RED) {
    fill(colourPurple); // Red
  }
}

/**
 * Returns a colour for the given PPM value, with
 * ppm <= co2GreenThreshold returning GREEN,
 * ppm = co2YellowThreshold returning YELLOW,
 * ppm = co2RedThreshold returning RED, and
 * co2RedThreshold < ppm returning PURPLE
 *
 * Values in between are interpolated.
 */
uint32_t Neopixel::ppmToColour(uint16_t ppm) {
  if (ppm <= config.co2GreenThreshold) {
    return colourGreen;
  } else if (ppm < config.co2YellowThreshold) {
    // green - yellow
    float d = float(ppm - config.co2GreenThreshold) / float(config.co2YellowThreshold - config.co2GreenThreshold);
    return this->intStrip->Color((uint8_t)(255.0 * d), 255, 0);
  } else if (ppm < config.co2RedThreshold) {
    // yellow - red
    float d = float(ppm - config.co2YellowThreshold) / float(config.co2RedThreshold - config.co2YellowThreshold);
    return this->intStrip->Color(255, (uint8_t)(255.0 * (1.0 - d)), 0);
  } else {
    // red - purple
    float d = float(min(ppm, config.co2DarkRedThreshold) - (config.co2RedThreshold)) / float(config.co2DarkRedThreshold - config.co2RedThreshold);
    return this->intStrip->Color(255, 0, (uint8_t)(255.0 * d));
  }
}

void Neopixel::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (mask & (M_CONFIG_CHANGED | M_CO2) == 0) return;
  if (mask & M_CONFIG_CHANGED) {

    ticker->detach();
    if (config.colourWheel) {
      ticker->attach(0.3, +[](Neopixel* instance) { instance->wheelTimer(); }, this);
    } else {

    }

    this->intStrip->setBrightness(config.brightness);
    this->extStrip->setBrightness(config.brightness);
  }
  if (mask & M_CO2 && !config.colourWheel) {
    fill(ppmToColour(model->getCo2()));
    if (newStatus == DARK_RED) {
      fill(colourPurple); // Purple
    } else {
      fill(ppmToColour(model->getCo2()));
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Neopixel::wheel(byte wheelPos) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    return this->intStrip->Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if (wheelPos < 170) {
    wheelPos -= 85;
    return this->intStrip->Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return this->intStrip->Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

void Neopixel::wheelTimer() {
  fill(wheel(wheelPos++));
}

void Neopixel::timer() {
  this->toggle = !(this->toggle);
  if (model->getStatus() == DARK_RED) {
    if (toggle)
      fill(colourPurple);
    else
      fill(colourOff);
  }
}