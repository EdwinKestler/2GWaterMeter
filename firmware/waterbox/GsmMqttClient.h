#ifndef WATERBOX_GSM_MQTT_CLIENT_H
#define WATERBOX_GSM_MQTT_CLIENT_H

#include <stdint.h>
#include <sim800Client.h>
#include <GSMPubSubClient.h>
#include "ConsumptionSignature.h"

class GsmMqttClient {
 public:
  enum Command {
    CmdNone,
    CmdOn,
    CmdOff
  };

  typedef void (*CommandHandler)(Command cmd);
  typedef void (*FingerprintHandler)(const ConsumptionSignature::Params& params);
  typedef void (*YieldFn)();

  GsmMqttClient();
  void attachYield(YieldFn fn);
  void attachCommandHandler(CommandHandler handler);
  void attachFingerprintHandler(FingerprintHandler handler);
  bool authConfigured() const;
  bool begin();
  bool connect();
  void disconnect();
  bool loop();
  bool hourOfDay(float* hourOut);
  bool publishTelemetry(const char* imei, float litresPerMinute, unsigned long intervalMl,
                        unsigned long totalMl, bool valveOpen, float modelLm, uint8_t leak,
                        uint8_t overuse);
  bool publishInfo(const char* imei, uint16_t pulsesPerLiter, bool reed, bool valveOpen,
                   bool signatureValid);
  bool publishHeartbeat(const char* imei, unsigned long uptimeS, unsigned long totalMl,
                        bool valveOpen, bool signatureValid, uint8_t leak, uint8_t overuse);
  int lastRssi() const;
  const char* imei() const;
  const char* commandTopic() const;
  const char* fingerprintTopic() const;
  const char* heartbeatTopic() const;

 private:
  static GsmMqttClient* instance_;
  static void mqttTrampoline(char* topic, uint8_t* payload, unsigned int length);
  void onMessage(char* topic, uint8_t* payload, unsigned int length);
  void wait(unsigned long ms);
  bool mqttConnect();
  void buildTopics();
  void formatTimestamp(char* out, size_t n);
  Command parseCommand(const uint8_t* payload, unsigned int length);
  bool parseFingerprint(const char* json, unsigned int length,
                        ConsumptionSignature::Params* params);

  sim800Client modem_;
  PubSubClient mqtt_;
  YieldFn yield_;
  CommandHandler handler_;
  FingerprintHandler fingerprintHandler_;
  char imeicode_[16];
  char topicData_[40];
  char topicCmd_[40];
  char topicInfo_[40];
  char topicFp_[40];
  char topicHb_[40];
  char msg_[180];
  int rssi_;
};

#endif
