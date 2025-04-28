

#ifndef C_HX711
#include <HX711_ADC.h>
#include <EEPROM.h>
#define C_HX711

// HX711_ADC LoadCell(HX711_dout, HX711_sck);

class HX711 : public HX711_ADC
{

public:
  HX711(int dout, int sck);
  void setup();
  void calibrate();
  void print();
  void setScaleValue(float scale);
  float data();
};

#endif
