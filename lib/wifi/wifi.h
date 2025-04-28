

#ifndef C_WIFI
#include "WiFiS3.h"
#include <string>
using namespace std;
#define C_WIFI

class wifi
{
private:
  int status = WL_IDLE_STATUS;
  string ssid;
  string pass;

public:
  wifi(string ssid, string pass);
  string repr() { return ssid; }
  void printWifiStatus();
  void setup();
  void listNetworks();
  void printEncryptionType(int encType);
  void printMacAddress(byte mac[]);
};


#endif
