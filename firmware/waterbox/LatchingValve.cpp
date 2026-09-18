#include "LatchingValve.h"
#include <Arduino.h>

void LatchingValve::begin(uint8_t pinIa, uint8_t pinIb, uint8_t speed, unsigned long pulseMs) {
  pinIa_ = pinIa;
  pinIb_ = pinIb;
  speed_ = speed;
  pulseMs_ = pulseMs;
  open_ = false;
  pinMode(pinIa_, OUTPUT);
  pinMode(pinIb_, OUTPUT);
  idle();
}

void LatchingValve::idle() {
  analogWrite(pinIa_, 0);
  analogWrite(pinIb_, 0);
}

void LatchingValve::pulse(uint8_t ia, uint8_t ib) {
  analogWrite(pinIa_, ia);
  analogWrite(pinIb_, ib);
  delay(pulseMs_);
  idle();
}

void LatchingValve::open() {
  pulse(speed_, 0);
  open_ = true;
}

void LatchingValve::close() {
  pulse(0, speed_);
  open_ = false;
}

void LatchingValve::apply(bool open) {
  if (open) {
    this->open();
  } else {
    close();
  }
}

bool LatchingValve::isOpen() const {
  return open_;
}
