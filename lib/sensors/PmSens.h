

// inherit from i2c base class, get i2c values and extra funcs

#ifndef C_PM25
#define C_PM25
#include "Seeed_HM330X.h"

class PmSens
{
private:
  uint8_t buf[30];
  int lastValue;
  HM330X sensor;
public:
  PmSens();
  void init();
  void update();
  int getValue();
};

#endif
