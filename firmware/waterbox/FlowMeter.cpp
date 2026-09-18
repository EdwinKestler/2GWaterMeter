#include "FlowMeter.h"
#include <Arduino.h>

#if !defined(digitalPinToInterrupt)
#if defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
#define digitalPinToInterrupt(p) (((p) == 2) ? 0 : ((p) == 3) ? 1 : ((p) == 6) ? 2 : -1)
#else
#define digitalPinToInterrupt(p) (((p) == 2) ? 0 : ((p) == 3) ? 1 : -1)
#endif
#endif


FlowMeter* FlowMeter::instance_ = 0;

void FlowMeter::isr() {
  if (instance_) {
    instance_->onPulse();
  }
}

void FlowMeter::onPulse() {
  pulseCount_++;
}

void FlowMeter::begin(uint8_t pin, uint16_t pulsesPerLiter, unsigned long sampleMs) {
  pin_ = pin;
  pulsesPerLiter_ = pulsesPerLiter;
  sampleMs_ = sampleMs;
  pulseCount_ = 0;
  totalPulses_ = 0;
  intervalMilliLitres_ = 0;
  litresPerMinute_ = 0.0f;
  dirty_ = false;
  oldTime_ = millis();

  pinMode(pin_, INPUT);
  digitalWrite(pin_, HIGH);
  instance_ = this;

  int irq = digitalPinToInterrupt(pin_);
  if (irq < 0) {
    Serial.println(F("flow pin is not an external interrupt"));
    return;
  }
  attachInterrupt(irq, isr, FALLING);
  Serial.print(F("flow IRQ "));
  Serial.println(irq);
}

void FlowMeter::setTotalPulses(uint32_t pulses) {
  totalPulses_ = pulses;
}

uint32_t FlowMeter::totalPulses() const {
  return totalPulses_;
}

uint16_t FlowMeter::pulsesPerLiter() const {
  return pulsesPerLiter_;
}

unsigned long FlowMeter::totalMilliLitres() const {
  if (pulsesPerLiter_ == 0) {
    return 0;
  }
  return (unsigned long)((totalPulses_ * 1000UL) / pulsesPerLiter_);
}

unsigned long FlowMeter::intervalMilliLitres() const {
  return intervalMilliLitres_;
}

float FlowMeter::litresPerMinute() const {
  return litresPerMinute_;
}

bool FlowMeter::consumeDirty() {
  bool d = dirty_;
  dirty_ = false;
  return d;
}

uint32_t FlowMeter::takePulses() {
  noInterrupts();
  uint32_t n = pulseCount_;
  pulseCount_ = 0;
  interrupts();
  return n;
}

void FlowMeter::update() {
  unsigned long now = millis();
  unsigned long elapsed = now - oldTime_;
  if (elapsed < sampleMs_) {
    return;
  }
  uint32_t pulses = takePulses();
  oldTime_ = now;
  if (pulses) {
    totalPulses_ += pulses;
    dirty_ = true;
  }
  if (pulsesPerLiter_ == 0) {
    intervalMilliLitres_ = 0;
    litresPerMinute_ = 0.0f;
    return;
  }
  intervalMilliLitres_ = (unsigned long)((pulses * 1000UL) / pulsesPerLiter_);
  if (elapsed == 0) {
    litresPerMinute_ = 0.0f;
  } else {
    litresPerMinute_ = (pulses * 60000.0f) / (elapsed * (float)pulsesPerLiter_);
  }
}
