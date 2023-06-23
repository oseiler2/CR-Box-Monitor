#include <fan.h>

#include <configManager.h>

#include <driver/pcnt.h>

// Local logging tag
static const char TAG[] = __FILE__;


/*
CLK src         max freq  max res
80 MHz APB_CLK    1 KHz   16 bit
80 MHz APB_CLK    5 KHz   14 bit
80 MHz APB_CLK   10 KHz   13 bit
8 MHz RTC8M_CLK   1 KHz   13 bit
8 MHz RTC8M_CLK   8 KHz   10 bit
1 MHz REF_TICK    1 KHz   10 bit
*/

#define PWM_FREQ            10000
#define PWM_RESOLUTION      8

/*
#define FAN_HALL               4
#define FAN_PWM                5
#define FAN_EN                 9
PWM_CHANNEL_FAN
*/


static uint8_t duty = 255;
const pcnt_channel_t PCNT_CHANNEL = PCNT_CHANNEL_0;
const pcnt_unit_t PCNT_UNIT = PCNT_UNIT_0;

Fan::Fan(Model* _model) {
  this->model = _model;

  pinMode(FAN_EN, OUTPUT);
  if (config.fanHasPwm) digitalWrite(FAN_EN, HIGH); // fan is controlled via PWM pin,give it full power

  pinMode(FAN_HALL, INPUT);

  pinMode(FAN_PWM, OUTPUT);
  ledcSetup(PWM_CHANNEL_FAN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(PWM_CHANNEL_FAN, 0);

  if (config.fanHasPwm) {
    // fan is controlled via PWM pin
    ledcAttachPin(FAN_PWM, PWM_CHANNEL_FAN);
  } else {
    // fan is controlled via PWM on FAN_EN
    ledcAttachPin(FAN_EN, PWM_CHANNEL_FAN);
    digitalWrite(FAN_PWM, HIGH);  // set PWM pin to high, in case a PWM enabled fan is connected but not configured.
  }

  ledcWrite(PWM_CHANNEL_FAN, duty);

  pcnt_config_t pcnt_Config = {
  .pulse_gpio_num = FAN_HALL,
  .ctrl_gpio_num = -1,
  .pos_mode = PCNT_CHANNEL_EDGE_ACTION_INCREASE,
  .neg_mode = PCNT_CHANNEL_EDGE_ACTION_HOLD,
  .counter_h_lim = 1000,
  .counter_l_lim = 0,
  .unit = PCNT_UNIT,
  .channel = PCNT_CHANNEL,
  };

  ESP_ERROR_CHECK(pcnt_unit_config(&pcnt_Config));
  ESP_ERROR_CHECK(pcnt_counter_pause(PCNT_UNIT));
  ESP_ERROR_CHECK(pcnt_counter_clear(PCNT_UNIT));
  /*
  pcnt_event_enable(PCNT_UNIT, PCNT_EVT_H_LIM);  // Interrupt on high limit.
  pcnt_isr_handle_t isrHandle;
  pcnt_isr_register(onHLim, (void*)backupCounter, 0, &isrHandle);
  pcnt_intr_enable(PCNT_UNIT);
  pcnt_counter_resume(PCNT_UNIT);*/

  // https://arduino.stackexchange.com/questions/81123/using-lambdas-as-callback-functions
  //  cyclicTimer->attach<typeof this>(1, [](typeof this p) { p->timer(); },
  //  this);

  // https://stackoverflow.com/questions/60985496/arduino-esp8266-esp32-ticker-callback-class-member-function
  pcntTicker = new Ticker();
  pcntTicker->attach(1, +[](Fan* instance) { instance->timer(); }, this);

  ESP_ERROR_CHECK(pcnt_counter_resume(PCNT_UNIT));
}

Fan::~Fan() {}

uint8_t Fan::getRpm(void) {
  return counter;
}

void Fan::setFanPwm(uint8_t pwm) {
  duty = pwm;
  ledcWrite(PWM_CHANNEL_FAN, pwm);
}

uint8_t Fan::getFanPwm(void) {
  return duty;
}

void Fan::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (!(mask & (M_CO2 | M_CONFIG_CHANGED))) return;
  uint16_t ppm = model->getCo2();
  if (ppm <= config.co2GreenThreshold) {
    setFanPwm(config.minPwm);
  } else if (ppm < config.co2YellowThreshold) {
    // green - yellow
    float d = float(ppm - config.co2GreenThreshold) / float(config.co2YellowThreshold - config.co2GreenThreshold);
    uint8_t offset = (uint8_t)(d * (127 - config.minPwm));
    setFanPwm(config.minPwm + offset);
  } else if (ppm < config.co2RedThreshold) {
    // yellow - red
    float d = float(ppm - config.co2YellowThreshold) / float(config.co2RedThreshold - config.co2YellowThreshold);
    uint8_t offset = (uint8_t)(d * 128);
    setFanPwm(127 + offset);
  } else {
    setFanPwm(255);
  }
}

void Fan::timer() {
  ESP_ERROR_CHECK(pcnt_get_counter_value(PCNT_UNIT, &counter));
  ESP_ERROR_CHECK(pcnt_counter_clear(PCNT_UNIT));
  ESP_LOGD(TAG, "counter: %i, duty: %u => %.0f%% / %.0f%%", counter, duty, (float)counter / 60 * 100, (float)duty / 255 * 100);
}
