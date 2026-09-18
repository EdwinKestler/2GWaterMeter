#include "ConsumptionSignature.h"
#include "settings.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ConsumptionSignature::ConsumptionSignature() {
  clear();
}

void ConsumptionSignature::clear() {
  params_.mean = 0;
  params_.a1 = 0;
  params_.phi1 = 0;
  params_.t1 = SIG_T1_H;
  params_.a2 = 0;
  params_.phi2 = 0;
  params_.t2 = SIG_T2_H;
  valid_ = false;
}

void ConsumptionSignature::setParams(const Params& params) {
  params_ = params;
  if (params_.t1 <= 0.0f) {
    params_.t1 = SIG_T1_H;
  }
  if (params_.t2 <= 0.0f) {
    params_.t2 = SIG_T2_H;
  }
  valid_ = true;
}

const ConsumptionSignature::Params& ConsumptionSignature::params() const {
  return params_;
}

bool ConsumptionSignature::valid() const {
  return valid_;
}

float ConsumptionSignature::predict(float hourOfDay) const {
  float w = params_.mean;
  if (params_.t1 > 0.0f && params_.a1 != 0.0f) {
    w += params_.a1 * sinf((2.0f * (float)M_PI * hourOfDay / params_.t1) + params_.phi1);
  }
  if (params_.t2 > 0.0f && params_.a2 != 0.0f) {
    w += params_.a2 * sinf((2.0f * (float)M_PI * hourOfDay / params_.t2) + params_.phi2);
  }
  if (w < 0.0f) {
    w = 0.0f;
  }
  return w;
}

bool ConsumptionSignature::isNight(float hourOfDay) const {
  if (SIG_NIGHT_START_H <= SIG_NIGHT_END_H) {
    return hourOfDay >= SIG_NIGHT_START_H && hourOfDay < SIG_NIGHT_END_H;
  }
  return hourOfDay >= SIG_NIGHT_START_H || hourOfDay < SIG_NIGHT_END_H;
}

ConsumptionSignature::Verdict ConsumptionSignature::classify(float measuredLm,
                                                             float hourOfDay) const {
  if (!valid_) {
    return Ok;
  }
  float expected = predict(hourOfDay);
  if (isNight(hourOfDay)) {
    if (measuredLm >= SIG_FLOW_FLOOR_LM && measuredLm > expected + SIG_LEAK_MARGIN_LM) {
      return NightLeak;
    }
    return Ok;
  }
  if (measuredLm >= SIG_FLOW_FLOOR_LM && measuredLm > expected + SIG_OVER_MARGIN_LM) {
    return Overuse;
  }
  return Ok;
}
