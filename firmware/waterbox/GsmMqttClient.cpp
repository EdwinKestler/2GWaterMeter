#include "GsmMqttClient.h"
#include "settings.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <Time.h>

GsmMqttClient* GsmMqttClient::instance_ = 0;

GsmMqttClient::GsmMqttClient()
    : mqtt_(mqttServer, mqttPort, mqttTrampoline, modem_),
      yield_(0),
      handler_(0) {
  instance_ = this;
  imeicode_[0] = 0;
  topicData_[0] = 0;
  topicCmd_[0] = 0;
  topicInfo_[0] = 0;
  msg_[0] = 0;
}

void GsmMqttClient::attachYield(YieldFn fn) {
  yield_ = fn;
}

void GsmMqttClient::attachCommandHandler(CommandHandler handler) {
  handler_ = handler;
}

void GsmMqttClient::wait(unsigned long ms) {
  unsigned long start = millis();
  while ((millis() - start) < ms) {
    if (yield_) {
      yield_();
    }
  }
}

bool GsmMqttClient::authConfigured() const {
#if MQTT_ALLOW_ANONYMOUS
  return true;
#else
  return mqttUser[0] != '\0' && mqttPass[0] != '\0';
#endif
}

const char* GsmMqttClient::imei() const {
  return imeicode_;
}

const char* GsmMqttClient::commandTopic() const {
  return topicCmd_;
}

void GsmMqttClient::buildTopics() {
  snprintf(topicData_, sizeof(topicData_), "%s/%s/data", TOPIC_ROOT, imeicode_);
  snprintf(topicCmd_, sizeof(topicCmd_), "%s/%s/cmd", TOPIC_ROOT, imeicode_);
  snprintf(topicInfo_, sizeof(topicInfo_), "%s/%s/info", TOPIC_ROOT, imeicode_);
}

void GsmMqttClient::mqttTrampoline(char* topic, uint8_t* payload, unsigned int length) {
  if (instance_) {
    instance_->onMessage(topic, payload, length);
  }
}

void GsmMqttClient::onMessage(char* topic, uint8_t* payload, unsigned int length) {
  (void)topic;
  while (length > 0) {
    char c = (char)payload[0];
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
      break;
    }
    payload++;
    length--;
  }
  while (length > 0) {
    char c = (char)payload[length - 1];
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '\0') {
      break;
    }
    length--;
  }
  Command cmd = CmdNone;
  if (length == 2 && payload[0] == 'o' && payload[1] == 'n') {
    cmd = CmdOn;
  } else if (length == 3 && payload[0] == 'o' && payload[1] == 'f' && payload[2] == 'f') {
    cmd = CmdOff;
  }
  if (cmd != CmdNone && handler_) {
    handler_(cmd);
  }
}

bool GsmMqttClient::mqttConnect() {
  if (!authConfigured()) {
    Serial.println(F("MQTT auth missing (set mqttUser/mqttPass or MQTT_ALLOW_ANONYMOUS)"));
    return false;
  }
#if MQTT_ALLOW_ANONYMOUS
  if (mqttUser[0] == '\0') {
    return mqtt_.connect(imeicode_);
  }
#endif
  return mqtt_.connect(imeicode_, mqttUser, mqttPass);
}

bool GsmMqttClient::begin() {
  Serial.println(F("SIM800 init"));
  for (int i = 0; i < SIM_INIT_TRIES; i++) {
    if (yield_) {
      yield_();
    }
#ifdef HARDWARESERIAL
    if (modem_.init(PIN_SIM_PWR, PIN_SIM_RST)) {
      break;
    }
#else
    if (modem_.init(&Serial1, PIN_SIM_PWR, PIN_SIM_RST)) {
      break;
    }
#endif
    Serial.println(F("init retry"));
    wait(1000);
  }
  if (!modem_.isInitialized()) {
    Serial.println(F("SIM800 init failed"));
    return false;
  }
  modem_.stop();
  modem_.TCPstop();
  if (!modem_.getIMEI(imeicode_)) {
    Serial.println(F("IMEI failed"));
    strncpy(imeicode_, "unknown", sizeof(imeicode_));
    imeicode_[sizeof(imeicode_) - 1] = 0;
    return false;
  }
  imeicode_[15] = 0;
  Serial.print(F("IMEI: "));
  Serial.println(imeicode_);
  buildTopics();
  if (!modem_.setup()) {
    Serial.println(F("SIM800 CREG not ready yet"));
  }
  return true;
}

bool GsmMqttClient::connect() {
  if (!authConfigured()) {
    return false;
  }
  byte attempt = 0;
  while (attempt < RADIO_RETRY_MAX) {
    attempt++;
    if (yield_) {
      yield_();
    }
    Serial.print(F("radio up try "));
    Serial.println(attempt);
    if (!modem_.TCPstart(GSMAPN, GSMUSER, GSMPASSWORD)) {
      Serial.println(F("TCPstart failed"));
      modem_.TCPstop();
      wait(RADIO_RETRY_DELAY_MS);
      continue;
    }
    if (yield_) {
      yield_();
    }
    if (!mqttConnect()) {
      Serial.println(F("MQTT connect failed"));
      modem_.stop();
      modem_.TCPstop();
      wait(RADIO_RETRY_DELAY_MS);
      continue;
    }
    mqtt_.subscribe(topicCmd_);
    Serial.print(F("MQTT up, cmd "));
    Serial.println(topicCmd_);
    return true;
  }
  return false;
}

void GsmMqttClient::disconnect() {
  if (mqtt_.connected()) {
    mqtt_.disconnect();
  }
  modem_.stop();
  modem_.TCPstop();
  Serial.println(F("radio down"));
}

bool GsmMqttClient::loop() {
  return mqtt_.loop();
}

bool GsmMqttClient::publishTelemetry(const char* imei, float litresPerMinute,
                                     unsigned long intervalMl, unsigned long totalMl,
                                     bool valveOpen) {
  char lmBuf[10];
  char ts[21];
  dtostrf(litresPerMinute, 1, 2, lmBuf);
  time_t t = modem_.RTCget();
  if (t == 0) {
    strncpy(ts, "unset", sizeof(ts));
    ts[sizeof(ts) - 1] = 0;
  } else {
    snprintf(ts, sizeof(ts), "%04d-%02d-%02dT%02d:%02d:%02d",
             year(t), month(t), day(t), hour(t), minute(t), second(t));
  }
  snprintf(msg_, sizeof(msg_),
           "{\"imei\":\"%s\",\"lm\":%s,\"mls\":%lu,\"tl_ml\":%lu,\"ts\":\"%s\",\"v\":%u}",
           imei, lmBuf, intervalMl, totalMl, ts, valveOpen ? 1u : 0u);
  Serial.print(F("pub "));
  Serial.print(topicData_);
  Serial.print(F(" -> "));
  Serial.println(msg_);
  if (mqtt_.publish(topicData_, msg_)) {
    Serial.println(F("Publish OK"));
    return true;
  }
  Serial.println(F("Publish FAILED"));
  return false;
}

bool GsmMqttClient::publishInfo(const char* imei, uint16_t pulsesPerLiter, bool reed,
                                bool valveOpen) {
  snprintf(msg_, sizeof(msg_), "{\"imei\":\"%s\",\"ppl\":%u,\"reed\":%u,\"v\":%u}",
           imei, (unsigned)pulsesPerLiter, reed ? 1u : 0u, valveOpen ? 1u : 0u);
  return mqtt_.publish(topicInfo_, msg_);
}
