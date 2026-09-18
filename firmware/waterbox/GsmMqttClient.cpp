#include "GsmMqttClient.h"
#include "settings.h"
#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Time.h>

GsmMqttClient* GsmMqttClient::instance_ = 0;

GsmMqttClient::GsmMqttClient()
    : mqtt_(mqttServer, mqttPort, mqttTrampoline, modem_),
      yield_(0),
      handler_(0),
      fingerprintHandler_(0) {
  instance_ = this;
  imeicode_[0] = 0;
  topicData_[0] = 0;
  topicCmd_[0] = 0;
  topicInfo_[0] = 0;
  topicFp_[0] = 0;
  topicHb_[0] = 0;
  msg_[0] = 0;
  rssi_ = 99;
}

void GsmMqttClient::attachYield(YieldFn fn) {
  yield_ = fn;
}

void GsmMqttClient::attachCommandHandler(CommandHandler handler) {
  handler_ = handler;
}

void GsmMqttClient::attachFingerprintHandler(FingerprintHandler handler) {
  fingerprintHandler_ = handler;
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

const char* GsmMqttClient::fingerprintTopic() const {
  return topicFp_;
}

const char* GsmMqttClient::heartbeatTopic() const {
  return topicHb_;
}

int GsmMqttClient::lastRssi() const {
  return rssi_;
}

void GsmMqttClient::buildTopics() {
  snprintf(topicData_, sizeof(topicData_), "%s/%s/data", TOPIC_ROOT, imeicode_);
  snprintf(topicCmd_, sizeof(topicCmd_), "%s/%s/cmd", TOPIC_ROOT, imeicode_);
  snprintf(topicInfo_, sizeof(topicInfo_), "%s/%s/info", TOPIC_ROOT, imeicode_);
  snprintf(topicFp_, sizeof(topicFp_), "%s/%s/fp", TOPIC_ROOT, imeicode_);
  snprintf(topicHb_, sizeof(topicHb_), "%s/%s/hb", TOPIC_ROOT, imeicode_);
}

void GsmMqttClient::formatTimestamp(char* out, size_t n) {
  time_t t = modem_.RTCget();
  if (t == 0) {
    strncpy(out, "unset", n);
    out[n - 1] = 0;
    return;
  }
  snprintf(out, n, "%04d-%02d-%02dT%02d:%02d:%02d",
           year(t), month(t), day(t), hour(t), minute(t), second(t));
}

static bool jsonFloat(const char* json, const char* key, float* out) {
  const char* p = strstr(json, key);
  if (!p) {
    return false;
  }
  p = strchr(p, ':');
  if (!p) {
    return false;
  }
  *out = (float)atof(p + 1);
  return true;
}

static bool payloadEq(const uint8_t* payload, unsigned int length, const char* token) {
  unsigned int n = (unsigned int)strlen(token);
  return length == n && memcmp(payload, token, n) == 0;
}

GsmMqttClient::Command GsmMqttClient::parseCommand(const uint8_t* payload, unsigned int length) {
  if (payloadEq(payload, length, "on") || payloadEq(payload, length, "open") ||
      payloadEq(payload, length, "1")) {
    return CmdOn;
  }
  if (payloadEq(payload, length, "off") || payloadEq(payload, length, "close") ||
      payloadEq(payload, length, "0")) {
    return CmdOff;
  }
  if (length >= 5 && payload[0] == '{') {
    if (length >= sizeof(msg_)) {
      length = sizeof(msg_) - 1;
    }
    memcpy(msg_, payload, length);
    msg_[length] = 0;
    float v = -1.0f;
    if (jsonFloat(msg_, "\"v\"", &v)) {
      if (v >= 0.5f) {
        return CmdOn;
      }
      return CmdOff;
    }
  }
  return CmdNone;
}

bool GsmMqttClient::parseFingerprint(const char* json, unsigned int length,
                                     ConsumptionSignature::Params* params) {
  (void)length;
  params->mean = 0;
  params->a1 = 0;
  params->phi1 = 0;
  params->t1 = SIG_T1_H;
  params->a2 = 0;
  params->phi2 = 0;
  params->t2 = SIG_T2_H;
  bool gotMean = jsonFloat(json, "\"w\"", &params->mean);
  jsonFloat(json, "\"a1\"", &params->a1);
  jsonFloat(json, "\"p1\"", &params->phi1);
  jsonFloat(json, "\"t1\"", &params->t1);
  jsonFloat(json, "\"a2\"", &params->a2);
  jsonFloat(json, "\"p2\"", &params->phi2);
  jsonFloat(json, "\"t2\"", &params->t2);
  return gotMean;
}

bool GsmMqttClient::hourOfDay(float* hourOut) {
  time_t t = modem_.RTCget();
  if (t == 0) {
    return false;
  }
  *hourOut = (float)::hour(t) + ((float)::minute(t) / 60.0f) + ((float)::second(t) / 3600.0f);
  return true;
}

void GsmMqttClient::mqttTrampoline(char* topic, uint8_t* payload, unsigned int length) {
  if (instance_) {
    instance_->onMessage(topic, payload, length);
  }
}

void GsmMqttClient::onMessage(char* topic, uint8_t* payload, unsigned int length) {
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

  if (topic && topicFp_[0] && strcmp(topic, topicFp_) == 0) {
    if (length >= sizeof(msg_)) {
      length = sizeof(msg_) - 1;
    }
    memcpy(msg_, payload, length);
    msg_[length] = 0;
    ConsumptionSignature::Params params;
    if (parseFingerprint(msg_, length, &params) && fingerprintHandler_) {
      Serial.println(F("fp received"));
      fingerprintHandler_(params);
    }
    return;
  }

  if (topic && topicCmd_[0] && strcmp(topic, topicCmd_) == 0) {
    Command cmd = parseCommand(payload, length);
    if (cmd != CmdNone && handler_) {
      Serial.println(cmd == CmdOn ? F("cmd open") : F("cmd close"));
      handler_(cmd);
    }
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
    {
      int ber = 0;
      if (!modem_.getSignalQualityReport(&rssi_, &ber)) {
        rssi_ = 99;
      }
    }
    if (!mqttConnect()) {
      Serial.println(F("MQTT connect failed"));
      modem_.stop();
      modem_.TCPstop();
      wait(RADIO_RETRY_DELAY_MS);
      continue;
    }
    mqtt_.subscribe(topicCmd_);
    mqtt_.subscribe(topicFp_);
    Serial.print(F("MQTT up, cmd "));
    Serial.println(topicCmd_);
    Serial.print(F("fp "));
    Serial.println(topicFp_);
    Serial.print(F("hb "));
    Serial.println(topicHb_);
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
                                     bool valveOpen, float modelLm, uint8_t leak,
                                     uint8_t overuse) {
  char lmBuf[10];
  char wmBuf[10];
  char ts[21];
  dtostrf(litresPerMinute, 1, 2, lmBuf);
  dtostrf(modelLm, 1, 2, wmBuf);
  formatTimestamp(ts, sizeof(ts));
  snprintf(msg_, sizeof(msg_),
           "{\"imei\":\"%s\",\"lm\":%s,\"mls\":%lu,\"tl_ml\":%lu,\"ts\":\"%s\",\"v\":%u,\"wm\":%s,\"lk\":%u,\"oc\":%u}",
           imei, lmBuf, intervalMl, totalMl, ts, valveOpen ? 1u : 0u, wmBuf, leak, overuse);
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
                                bool valveOpen, bool signatureValid) {
  snprintf(msg_, sizeof(msg_), "{\"imei\":\"%s\",\"ppl\":%u,\"reed\":%u,\"v\":%u,\"fp\":%u}",
           imei, (unsigned)pulsesPerLiter, reed ? 1u : 0u, valveOpen ? 1u : 0u,
           signatureValid ? 1u : 0u);
  return mqtt_.publish(topicInfo_, msg_);
}

bool GsmMqttClient::publishHeartbeat(const char* imei, unsigned long uptimeS,
                                     unsigned long totalMl, bool valveOpen, bool signatureValid,
                                     uint8_t leak, uint8_t overuse) {
  char ts[21];
  formatTimestamp(ts, sizeof(ts));
  snprintf(msg_, sizeof(msg_),
           "{\"imei\":\"%s\",\"ts\":\"%s\",\"up\":%lu,\"v\":%u,\"fp\":%u,\"lk\":%u,\"oc\":%u,\"rssi\":%d,\"tl_ml\":%lu,\"ok\":1}",
           imei, ts, uptimeS, valveOpen ? 1u : 0u, signatureValid ? 1u : 0u, leak, overuse, rssi_,
           totalMl);
  Serial.print(F("hb "));
  Serial.print(topicHb_);
  Serial.print(F(" -> "));
  Serial.println(msg_);
  return mqtt_.publish(topicHb_, msg_);
}
