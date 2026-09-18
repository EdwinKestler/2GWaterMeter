#include "WaterboxFsm.h"
#include "Watchdog.h"
#include "settings.h"
#include <Arduino.h>

WaterboxFsm* WaterboxFsm::instance_ = 0;

void WaterboxFsm::yieldTrampoline() {
  if (instance_) {
    instance_->service();
  }
}

void WaterboxFsm::commandTrampoline(GsmMqttClient::Command cmd) {
  if (instance_) {
    instance_->onCommand(cmd);
  }
}

WaterboxFsm::State WaterboxFsm::state() const {
  return state_;
}

uint16_t WaterboxFsm::pulsesPerLiter() const {
  return FLOW_SENSOR_REED ? PULSES_PER_LITER_REED : PULSES_PER_LITER_HALL;
}

void WaterboxFsm::service() {
  Watchdog::kick();
  flow_.update();
  if (flow_.consumeDirty()) {
    store_.markDirty();
  }
  store_.saveIfDue(flow_.totalPulses(), valve_.isOpen(), EEPROM_SAVE_MS);
  led_.setFlowing(flow_.litresPerMinute() > 0.05f);
  if (state_ == HaltAuth || state_ == WaitModem) {
    led_.setMode(StatusLed::Error);
  } else if (state_ == Listen || state_ == Publish || state_ == Connect) {
    led_.setMode(StatusLed::Radio);
  } else {
    led_.setMode(StatusLed::Idle);
  }
  led_.update();
}

void WaterboxFsm::persistNow() {
  store_.save(flow_.totalPulses(), valve_.isOpen());
}

void WaterboxFsm::enter(State next) {
  state_ = next;
}

void WaterboxFsm::onCommand(GsmMqttClient::Command cmd) {
  if (cmd == GsmMqttClient::CmdOn) {
    valve_.open();
    persistNow();
  } else if (cmd == GsmMqttClient::CmdOff) {
    valve_.close();
    persistNow();
  }
}

void WaterboxFsm::begin() {
  instance_ = this;
  state_ = WaitModem;
  lastPublishMs_ = 0;
  listenStartMs_ = 0;
  lastModemTryMs_ = 0;

  Serial.begin(19200);
  Serial.println(F("waterbox"));
  Serial.print(F("pulses/L "));
  Serial.println(pulsesPerLiter());

  led_.begin(PIN_STATUS_LED);
  valve_.begin(PIN_VALVE_IA, PIN_VALVE_IB, VALVE_SPEED, VALVE_PULSE_MS);

  uint32_t pulses = 0;
  bool valveOpen = false;
  bool restored = store_.load(&pulses, &valveOpen);
  if (restored) {
    Serial.print(F("eeprom tl_ml "));
    Serial.println((unsigned long)((pulses * 1000UL) / pulsesPerLiter()));
    valve_.apply(valveOpen);
  } else {
    Serial.println(F("eeprom empty"));
  }

  flow_.begin(PIN_FLOW, pulsesPerLiter(), FLOW_SAMPLE_MS);
  if (restored) {
    flow_.setTotalPulses(pulses);
  }

  radio_.attachYield(yieldTrampoline);
  radio_.attachCommandHandler(commandTrampoline);

  if (!radio_.authConfigured()) {
    Serial.println(F("MQTT auth not set; radio disabled"));
    enter(HaltAuth);
  } else {
    enter(WaitModem);
    lastModemTryMs_ = millis() - SIM_RETRY_MS;
  }

  Watchdog::enable();
}

void WaterboxFsm::tick() {
  service();
  unsigned long now = millis();

  switch (state_) {
    case HaltAuth:
      break;

    case WaitModem:
      if ((now - lastModemTryMs_) >= SIM_RETRY_MS) {
        lastModemTryMs_ = now;
        if (radio_.begin()) {
          lastPublishMs_ = now - PUBLISH_INTERVAL_MS;
          enter(Idle);
        }
      }
      break;

    case Idle:
      if ((now - lastPublishMs_) >= PUBLISH_INTERVAL_MS) {
        enter(Connect);
      }
      break;

    case Connect:
      if (radio_.connect()) {
        enter(Publish);
      } else {
        Serial.println(F("radio gave up this cycle"));
        lastPublishMs_ = now;
        enter(Idle);
      }
      break;

    case Publish:
      radio_.publishInfo(radio_.imei(), pulsesPerLiter(), FLOW_SENSOR_REED, valve_.isOpen());
      if (radio_.publishTelemetry(radio_.imei(), flow_.litresPerMinute(),
                                  flow_.intervalMilliLitres(), flow_.totalMilliLitres(),
                                  valve_.isOpen())) {
        persistNow();
      }
      listenStartMs_ = millis();
      enter(Listen);
      break;

    case Listen:
      radio_.loop();
      if ((now - listenStartMs_) >= CMD_LISTEN_MS) {
        enter(Disconnect);
      }
      break;

    case Disconnect:
      radio_.disconnect();
      lastPublishMs_ = millis();
      enter(Idle);
      break;
  }
}
