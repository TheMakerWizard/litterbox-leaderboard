


#ifndef C_CAT
#define C_CAT
#include <string>
using namespace std;

// HX711_ADC LoadCell(HX711_dout, HX711_sck);

class cat
{
// private:
  // vector<BM> bowel_movements;
public:
  cat(string name, int weight);
  int weight;
  string name;
  string repr(){ return name; }
};

#endif
