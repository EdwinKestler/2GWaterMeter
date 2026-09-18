#ifndef WATERBOX_FLOW_METER_H
#define WATERBOX_FLOW_METER_H

#include <stdint.h>

class FlowMeter {
 public:
  void begin(uint8_t pin, uint16_t pulsesPerLiter, unsigned long sampleMs);
  void update();
  void setTotalPulses(uint32_t pulses);
  uint32_t totalPulses() const;
  uint16_t pulsesPerLiter() const;
  unsigned long totalMilliLitres() const;
  unsigned long intervalMilliLitres() const;
  float litresPerMinute() const;
  bool consumeDirty();

 private:
  static FlowMeter* instance_;
  static void isr();
  void onPulse();
  uint32_t takePulses();

  uint8_t pin_;
  uint16_t pulsesPerLiter_;
  unsigned long sampleMs_;
  volatile uint32_t pulseCount_;
  uint32_t totalPulses_;
  unsigned long intervalMilliLitres_;
  unsigned long oldTime_;
  float litresPerMinute_;
  bool dirty_;
};

#endif
