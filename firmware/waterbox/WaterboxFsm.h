#ifndef WATERBOX_FSM_H
#define WATERBOX_FSM_H

#include "FlowMeter.h"
#include "LatchingValve.h"
#include "MeterStore.h"
#include "StatusLed.h"
#include "GsmMqttClient.h"
#include "ConsumptionSignature.h"

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
  static void fingerprintTrampoline(const ConsumptionSignature::Params& params);

  void service();
  void enter(State next);
  void onCommand(GsmMqttClient::Command cmd);
  void onFingerprint(const ConsumptionSignature::Params& params);
  void persistNow();
  void evaluateSignature();
  uint16_t pulsesPerLiter() const;

  FlowMeter flow_;
  LatchingValve valve_;
  MeterStore store_;
  StatusLed led_;
  GsmMqttClient radio_;
  ConsumptionSignature signature_;
  State state_;
  unsigned long lastPublishMs_;
  unsigned long lastHeartbeatMs_;
  unsigned long listenStartMs_;
  unsigned long lastModemTryMs_;
  float modelLm_;
  uint8_t leakFlag_;
  uint8_t overFlag_;
};

#endif
