#include "MeterStore.h"
#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_MAGIC 0xA5
#define EEPROM_VERSION 1
#define EEPROM_BASE 0

struct EepromImage {
  uint8_t magic;
  uint8_t version;
  uint32_t pulses;
  uint8_t valve;
  uint8_t crc;
} __attribute__((packed));

static uint8_t crc8(const uint8_t* data, uint8_t n) {
  uint8_t crc = 0;
  while (n--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++) {
      crc = (crc & 1) ? (uint8_t)((crc >> 1) ^ 0x8C) : (uint8_t)(crc >> 1);
    }
  }
  return crc;
}

bool MeterStore::load(uint32_t* pulses, bool* valveOpen) {
  dirty_ = false;
  lastSaveMs_ = millis();
  EepromImage img;
  EEPROM.get(EEPROM_BASE, img);
  if (img.magic != EEPROM_MAGIC || img.version != EEPROM_VERSION) {
    return false;
  }
  if (crc8((const uint8_t*)&img.pulses, 5) != img.crc) {
    return false;
  }
  *pulses = img.pulses;
  *valveOpen = img.valve != 0;
  return true;
}

void MeterStore::save(uint32_t pulses, bool valveOpen) {
  EepromImage img;
  img.magic = EEPROM_MAGIC;
  img.version = EEPROM_VERSION;
  img.pulses = pulses;
  img.valve = valveOpen ? 1 : 0;
  img.crc = crc8((const uint8_t*)&img.pulses, 5);
  EEPROM.put(EEPROM_BASE, img);
  dirty_ = false;
  lastSaveMs_ = millis();
}

void MeterStore::markDirty() {
  dirty_ = true;
}

void MeterStore::saveIfDue(uint32_t pulses, bool valveOpen, unsigned long minIntervalMs) {
  if (!dirty_) {
    return;
  }
  if ((millis() - lastSaveMs_) < minIntervalMs) {
    return;
  }
  save(pulses, valveOpen);
}
