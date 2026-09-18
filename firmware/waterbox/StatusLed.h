#ifndef WATERBOX_STATUS_LED_H
#define WATERBOX_STATUS_LED_H

#include <stdint.h>

class StatusLed {
 public:
  enum Mode {
    Error,
    Radio,
    Idle
  };

  void begin(uint8_t pin);
  void setMode(Mode mode);
  void setFlowing(bool flowing);
  void update();

 private:
  uint8_t pin_;
  Mode mode_;
  bool flowing_;
};

#endif
