#include "hx711.h"
#include <HX711_ADC.h>

HX711::HX711(int dout, int sck) : HX711_ADC(dout,sck) {}

// Initalize load cell
void HX711::setup(){
  this->begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  this->start(stabilizingtime, _tare);
  if (this->getTareTimeoutFlag() || this->getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  while (!this->update());
}

void HX711::setScaleValue(float scale){
  this->setCalFactor(scale);
}

// Calibrate load cell
void HX711::calibrate(){
  const int calVal_eepromAdress = 0;
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load this->");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false;
  while (_resume == false) {
    this->update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') this->tareNoDelay();
      }
    }
    if (this->getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }
    Serial.println("Now, place your known mass on the loadcell.");
    Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

    float known_mass = 0;
    _resume = false;
    while (_resume == false) {
      this->update();
      if (Serial.available() > 0) {
        known_mass = Serial.parseFloat();
        if (known_mass != 0) {
          Serial.print("Known mass is: ");
          Serial.println(known_mass);
          _resume = true;
        }
      }
    }

    this->refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
    float newCalibrationValue = this->getNewCalibration(known_mass); //get the new calibration value

    Serial.print("New calibration value has been set to: ");
    Serial.print(newCalibrationValue);
    Serial.println(", use this as calibration value (calFactor) in your project sketch.");
    Serial.print("Save this value to EEPROM adress ");
    Serial.print(calVal_eepromAdress);
    Serial.println("? y/n");

    _resume = false;
    while (_resume == false) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 'y') {
  #if defined(ESP8266)|| defined(ESP32)
          EEPROM.begin(512);
  #endif
          EEPROM.put(calVal_eepromAdress, newCalibrationValue);
  #if defined(ESP8266)|| defined(ESP32)
          EEPROM.commit();
  #endif
          EEPROM.get(calVal_eepromAdress, newCalibrationValue);
          Serial.print("Value ");
          Serial.print(newCalibrationValue);
          Serial.print(" saved to EEPROM address: ");
          Serial.println(calVal_eepromAdress);
          _resume = true;

        }
        else if (inByte == 'n') {
          Serial.println("Value not saved to EEPROM");
          _resume = true;
        }
      }
    }
    Serial.println("End calibration");
}

void HX711::print(){
  this->update();
  float i = this->getData();
  Serial.print("Load_cell output val: ");
  Serial.println(i);
}

float HX711::data(){
  this->update();
  return this->getData();
}
