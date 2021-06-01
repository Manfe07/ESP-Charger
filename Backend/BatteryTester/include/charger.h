#ifndef CHARGER_H
#define CHARGER_H

#include <Adafruit_ADS1015.h>

class Charger
{
    private:
        /* data */
        Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
        int charge_pin;
        int discharge_pin;
        float voltage_multiplier = 0.221681F;
        float current_multiplier = 0.059008F;
        
        int set_discharge(int);
        int set_charge(int);

    public:
        int charge_power = 0;
        int discharge_power = 0;
        float charge_current = 500;
        float charge_voltage = 4000;
        float discharge_current = 750;
        float discharge_voltage = 3000;

        Charger(int, int);
        ~Charger();
        void begin();
        float get_current();
        float get_voltage();
        void stop();
        void charge();
        void discharge();
};

Charger::Charger(int _charge_pin, int _discharge_pin){
    charge_pin = _charge_pin;
    discharge_pin = _discharge_pin;
}

Charger::~Charger(){
}

void Charger::begin(){
    pinMode(charge_pin, OUTPUT);
    analogWrite(charge_pin, 0);
    pinMode(discharge_pin, OUTPUT);
    analogWrite(discharge_pin, 0);
    ads.begin();
}

float Charger::get_voltage(){
    int16_t results;
    ads.setGain(GAIN_TWOTHIRDS);
    results = ads.readADC_Differential_2_3();  
    float voltage = results * voltage_multiplier;
    return voltage;
}

float Charger::get_current(){
    int16_t results;
    ads.setGain(GAIN_EIGHT);
    results = ads.readADC_Differential_0_1();  
    float current = results * current_multiplier;
    return current;
}


int Charger::set_charge(int _power){
    if(discharge_power != 0){
        charge_power = 0;
        return -1;
    }
    else{
        charge_power = _power;
        analogWrite(charge_pin, charge_power);
        return 1;
    }
}

//BEGIN int Charger::set_discharge
int Charger::set_discharge(int _power){
    if(charge_power != 0){
        discharge_power = 0;
        return -1;
    }
    else{
        discharge_power = _power;
        analogWrite(discharge_pin, discharge_power);
        return 1;
    }
}
//END int Charger::set_discharge

//BEGIN void Charger::stop
void Charger::stop(){
    set_charge(0);
    set_discharge(0);
}
//END void Charger::stop

//BEGIN void Charger::charge
void Charger::charge(){
float _maxVoltage = charge_voltage;
float _maxCurrent = charge_current;
float sum_voltage = 0;
  float sum_current = 0;
  for(int i = 0; i < 5; i++){
    sum_voltage += get_voltage();
    sum_current += get_current();
  }
  float voltage = sum_voltage / 5;
  float current = sum_current / 5;
  
  set_discharge(0);
  if(voltage < 2500){
      _maxCurrent = 100;
  }
  else if(abs(current) < _maxCurrent && voltage < _maxVoltage){
      charge_power += 5;
      if(charge_power > 1024)
        charge_power = 1024;
      set_charge(charge_power);
  }
  else{
      charge_power -= 10;
      if(discharge_power < 0)
        charge_power = 0;
      set_charge(charge_power);
  }
}
//END void Charger::charge

//BEGIN void Charger::discharge
void Charger::discharge(){
  float _maxCurrent = discharge_current;
  float _minVoltage = discharge_voltage;
  float sum_voltage = 0;
  float sum_current = 0;
  for(int i = 0; i < 5; i++){
    sum_voltage += get_voltage();
    sum_current += get_current();
  }
  float voltage = sum_voltage / 5;
  float current = sum_current / 5;
  
    set_charge(0);
  if(voltage < 2000){
    discharge_power = 0;
    set_discharge(discharge_power);
  }
  else if(abs(current) < _maxCurrent && voltage > _minVoltage){
      discharge_power += 5;
      if(discharge_power > 1024)
        discharge_power = 1024;
      set_discharge(discharge_power);
  }
  else if(abs(current) > (_maxCurrent + 5) || voltage < _minVoltage){
      discharge_power -= 10;
      if(discharge_power < 0)
        discharge_power = 0;
      set_discharge(discharge_power);
  }
}
//END void Charger::discharge
#endif