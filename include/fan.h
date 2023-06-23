#ifndef _FAN_H
#define _FAN_H

#include <globals.h>
#include <Arduino.h>
#include <config.h>
#include <model.h>

#include <Ticker.h>

class Fan {
public:
  Fan(Model* model);
  ~Fan();

  void update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus);

  uint8_t getRpm();
  void setFanPwm(uint8_t pwm);
  uint8_t getFanPwm(void);

private:
  Model* model;
  Ticker* pcntTicker;
  int16_t counter;

  void timer();

};

#endif