#ifndef WATERBOX_GSM_MQTT_CLIENT_H
#define WATERBOX_GSM_MQTT_CLIENT_H

#include <stdint.h>
#include <sim800Client.h>
#include <GSMPubSubClient.h>

class GsmMqttClient {
 public:
  enum Command {
    CmdNone,
    CmdOn,
    CmdOff
  };

  typedef void (*CommandHandler)(Command cmd);
  typedef void (*YieldFn)();

  GsmMqttClient();
  void attachYield(YieldFn fn);
  void attachCommandHandler(CommandHandler handler);
  bool authConfigured() const;
  bool begin();
  bool connect();
  void disconnect();
  bool loop();
  bool publishTelemetry(const char* imei, float litresPerMinute, unsigned long intervalMl,
                        unsigned long totalMl, bool valveOpen);
  bool publishInfo(const char* imei, uint16_t pulsesPerLiter, bool reed, bool valveOpen);
  const char* imei() const;
  const char* commandTopic() const;

 private:
  static GsmMqttClient* instance_;
  static void mqttTrampoline(char* topic, uint8_t* payload, unsigned int length);
  void onMessage(char* topic, uint8_t* payload, unsigned int length);
  void wait(unsigned long ms);
  bool mqttConnect();
  void buildTopics();

  sim800Client modem_;
  PubSubClient mqtt_;
  YieldFn yield_;
  CommandHandler handler_;
  char imeicode_[16];
  char topicData_[40];
  char topicCmd_[40];
  char topicInfo_[40];
  char msg_[160];
};

#endif
