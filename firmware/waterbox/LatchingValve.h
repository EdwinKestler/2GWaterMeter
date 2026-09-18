#ifndef WATERBOX_LATCHING_VALVE_H
#define WATERBOX_LATCHING_VALVE_H

#include <stdint.h>

class LatchingValve {
 public:
  void begin(uint8_t pinIa, uint8_t pinIb, uint8_t speed, unsigned long pulseMs);
  void open();
  void close();
  void apply(bool open);
  bool isOpen() const;

 private:
  void pulse(uint8_t ia, uint8_t ib);
  void idle();

  uint8_t pinIa_;
  uint8_t pinIb_;
  uint8_t speed_;
  unsigned long pulseMs_;
  bool open_;
};

#endif
