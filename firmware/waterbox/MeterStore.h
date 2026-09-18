#ifndef WATERBOX_METER_STORE_H
#define WATERBOX_METER_STORE_H

#include <stdint.h>
#include "ConsumptionSignature.h"

class MeterStore {
 public:
  bool load(uint32_t* pulses, bool* valveOpen);
  void save(uint32_t pulses, bool valveOpen);
  void saveIfDue(uint32_t pulses, bool valveOpen, unsigned long minIntervalMs);
  void markDirty();
  bool loadFingerprint(ConsumptionSignature::Params* params);
  void saveFingerprint(const ConsumptionSignature::Params& params);

 private:
  bool dirty_;
  unsigned long lastSaveMs_;
};

#endif

