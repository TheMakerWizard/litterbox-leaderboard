


#ifndef C_BM
#define C_BM

// HX711_ADC LoadCell(HX711_dout, HX711_sck);

class BM
{
public:
  BM();
  BM(int catIndex);
  int catIndex;
  // int start_time; TODO
  // int stop_time; TODO
  int peakPM;
  float averagePm;
  float weight;
};

#endif
