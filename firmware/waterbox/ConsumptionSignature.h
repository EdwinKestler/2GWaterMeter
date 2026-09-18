#ifndef WATERBOX_CONSUMPTION_SIGNATURE_H
#define WATERBOX_CONSUMPTION_SIGNATURE_H

#include <stdint.h>

// Cloud-fitted household fingerprint from
// W(t) = mean + a1*sin(2π t/t1 + phi1) + a2*sin(2π t/t2 + phi2)
// with t in hours of day. Coefficients are in L/min.
class ConsumptionSignature {
 public:
  struct Params {
    float mean;
    float a1;
    float phi1;
    float t1;
    float a2;
    float phi2;
    float t2;
  };

  enum Verdict {
    Ok = 0,
    NightLeak = 1,
    Overuse = 2
  };

  ConsumptionSignature();
  void clear();
  void setParams(const Params& params);
  const Params& params() const;
  bool valid() const;
  float predict(float hourOfDay) const;
  bool isNight(float hourOfDay) const;
  Verdict classify(float measuredLm, float hourOfDay) const;

 private:
  Params params_;
  bool valid_;
};

#endif
