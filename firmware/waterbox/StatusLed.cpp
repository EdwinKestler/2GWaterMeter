#include "StatusLed.h"
#include <Arduino.h>

void StatusLed::begin(uint8_t pin) {
  pin_ = pin;
  mode_ = Error;
  flowing_ = false;
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, HIGH);
}

void StatusLed::setMode(Mode mode) {
  mode_ = mode;
}

void StatusLed::setFlowing(bool flowing) {
  flowing_ = flowing;
}

void StatusLed::update() {
  unsigned long now = millis();
  if (mode_ == Radio) {
    digitalWrite(pin_, HIGH);
    return;
  }
  if (mode_ == Error) {
    digitalWrite(pin_, ((now / 200) & 1) ? HIGH : LOW);
    return;
  }
  unsigned long phase = now % 2000UL;
  bool on = (phase < 80);
  if (flowing_ && phase >= 250 && phase < 330) {
    on = true;
  }
  digitalWrite(pin_, on ? HIGH : LOW);
}
