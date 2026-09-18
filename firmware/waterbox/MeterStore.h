#ifndef WATERBOX_METER_STORE_H
#define WATERBOX_METER_STORE_H

#include <stdint.h>

class MeterStore {
 public:
  bool load(uint32_t* pulses, bool* valveOpen);
  void save(uint32_t pulses, bool valveOpen);
  void saveIfDue(uint32_t pulses, bool valveOpen, unsigned long minIntervalMs);
  void markDirty();

 private:
  bool dirty_;
  unsigned long lastSaveMs_;
};

#endif
