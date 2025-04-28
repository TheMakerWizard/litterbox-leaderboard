#include "PmSens.h"

PmSens::PmSens(){}

void PmSens::init(){
  // init Seed PM2.5 sensor
  bool pmSensorInit = sensor.init();
  while(pmSensorInit){
    pmSensorInit = sensor.init();
    Serial.println("Retrying PM2.5 Sensor Init...");
  }
}

void PmSens::update(){
  if(sensor.read_sensor_value(buf,29)){
    // read failed
    lastValue = -1;
  } else {
    // parse buffer
    uint16_t value = buf[3 * 2] << 8 | buf[3 * 2 + 1];
    lastValue = (int) value;
  }

}
int PmSens::getValue(){
  return lastValue;
}
