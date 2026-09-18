#ifndef WATERBOX_FSM_H
#define WATERBOX_FSM_H

#include "FlowMeter.h"
#include "LatchingValve.h"
#include "MeterStore.h"
#include "StatusLed.h"
#include "GsmMqttClient.h"

class WaterboxFsm {
 public:
  enum State {
    HaltAuth,
    WaitModem,
    Idle,
    Connect,
    Publish,
    Listen,
    Disconnect
  };

  void begin();
  void tick();
  State state() const;

 private:
  static WaterboxFsm* instance_;
  static void yieldTrampoline();
  static void commandTrampoline(GsmMqttClient::Command cmd);

  void service();
  void enter(State next);
  void onCommand(GsmMqttClient::Command cmd);
  void persistNow();
  uint16_t pulsesPerLiter() const;

  FlowMeter flow_;
  LatchingValve valve_;
  MeterStore store_;
  StatusLed led_;
  GsmMqttClient radio_;
  State state_;
  unsigned long lastPublishMs_;
  unsigned long listenStartMs_;
  unsigned long lastModemTryMs_;
};

#endif
