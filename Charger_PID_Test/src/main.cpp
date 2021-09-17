#include <Arduino.h>
#include "PID_v1.h"
#include <ADS1X15.h>
#include "WiFiClient.h"
#include <DNSServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "ArduinoOTA.h"

#define chargePin     D6
#define dischargePin  D5

enum modeList {standby, discharging, charging, idle};
modeList mode = standby;

unsigned int INFLUXDB_PORT = 8086; // INFLUXPort

double SetpointC, InputC, OutputC;
double SetpointV, InputV, OutputV;

float voltage_multiplier = 0.221681F;
float voltage_offset = -654.0F;
float current_multiplier = 0.059008F;


int value = 0;

float voltage;
float current;
float capacity = 0;

//Specify the links and initial tuning parameters
double KpC=0.07, KiC=0.3, KdC=0.001;
double KpV=0.1, KiV=1, KdV=0.001;
PID pidCurrent(&InputC, &OutputC, &SetpointC, KpC, KiC, KdC, DIRECT);
PID pidVoltage(&InputV, &OutputV, &SetpointV, KpV, KiV, KdV, DIRECT);

ADS1115 ADS(0x48);
WiFiManager wifiManager;
HTTPClient http;  //Declare an object of class HTTPClient
WiFiClient wifiClient;

 
void setup() {
  wifiManager.autoConnect("AutoConnectAP");
  String mac = WiFi.macAddress();
  String hostname ="espCharger_" + mac;
  WiFi.hostname(hostname);
  pidCurrent.SetOutputLimits(-1024,1024);
  pidVoltage.SetOutputLimits(-1024,1024);
  pidCurrent.SetMode(AUTOMATIC);
  pidVoltage.SetMode(AUTOMATIC);
  Serial.begin(115200);
  // put your setup code here, to run once:
  pinMode(chargePin, OUTPUT);
  pinMode(dischargePin, OUTPUT);
  digitalWrite(chargePin, LOW);
  digitalWrite(dischargePin, LOW);
  
  ADS.begin();
  ADS.setGain(0);
}

float getVoltage(){
    int16_t results;
    ADS.setGain(0);
    results = ADS.readADC_Differential_2_3();  
    float voltage = results * voltage_multiplier;
    voltage -= voltage_offset;
    return voltage;
}


float getCurrent(){
    int16_t results;
    ADS.setGain(8);
    results = ADS.readADC_Differential_0_1();  
    float current = results * current_multiplier;
    return current;
}


void resetPIDcurrent(){
  pidCurrent.SetMode(MANUAL);
  OutputC = 0;
  pidCurrent.SetMode(AUTOMATIC);
}


void resetPIDvoltage(){
  pidVoltage.SetMode(MANUAL);
  OutputV = 0;
  pidVoltage.SetMode(AUTOMATIC);
}


void calcCapacity(){
  static long oldTick = 0;
  float now = millis();
  float duration = (now - oldTick) / 1000 / 60; //duration in minutes
  capacity += current * (duration / 60);        //capactiy in mAh
  oldTick = now;
  Serial.println(duration,4);
}

void setOutput(int value){
  if(value > 0){
    analogWrite(dischargePin, 0);
    analogWrite(chargePin, value);
  }
  else if(value < 0){
    analogWrite(chargePin, 0);
    analogWrite(dischargePin, abs(value));
  }
  else{
    analogWrite(chargePin, 0);
    analogWrite(dischargePin, 0);
  }
}

void runPID(){
  SetpointV = 3600;
  SetpointC = -750;
  pidVoltage.Compute();
  pidCurrent.Compute();
  
  if(InputV < 250)      //min voltage to detect battery is 250mV
  {
    setOutput(0);
    mode = standby;
    Serial.println("low Voltage");
  }
  else if(((-100 > OutputC > 100) | (-100 > OutputV > 100)) & ( -5 < InputC < 5)){    //if Charger is charging or discharging, but no currentflow, turn off (missing Battery)
    Serial.println("missing current");
    setOutput(0);
    mode = standby;
  }
  if(abs(OutputV) <= abs(OutputC))
    setOutput(int(OutputV));
  else if(abs(OutputC) <= abs(OutputV))
    setOutput(int(OutputC));
  else
    setOutput(0);
}


void loop() {
  /*
  for(int i = 0; i < 1024; i+=50){
    analogWrite(chargePin, i);
    Serial.print(i);Serial.print("\t");
    Serial.print(ADS.readADC_Differential_2_3());Serial.print("\t");
    Serial.println("");
    delay(5000);
  }
  */
  if(Serial.available() > 1){
    int x = Serial.parseInt();
    if(x != 0)
      value = x;
      analogWrite(chargePin, value);
  }
  for(int n = 0; n < 1; n++){
    float sum_voltage = 0;
    float sum_current = 0;
    for(int i = 0; i < 10; i++){
      sum_voltage += getVoltage();
      sum_current += getCurrent();
      delay(100);
    }
    voltage = sum_voltage / 10;
    current = sum_current / 10;

    //update values for PID  
    InputV = voltage;
    InputC = current;
    calcCapacity();
  
    switch (mode)
    {
      case standby:
        resetPIDcurrent();
        resetPIDvoltage();
        capacity = 0;
        if (voltage > SetpointV)
          mode = discharging;
        else if (voltage > 250)   //min voltage to detect battery is 250mV
          mode = charging;
        break;
      case charging:
        runPID();
        break;
      case discharging:
        runPID();
        break;
      default:
        setOutput(0);
        mode = standby;
        break;
    }
    
    //delay(100);
  }
  String header = "http://charger.manuel-fehren.de/push?id=" + WiFi.macAddress() + "&channel=1&voltage=" + String((voltage/1000),5) + "&current=" + String((current/1000),5) + "&capacity=" + String((capacity/1000),4);
  http.begin(wifiClient, header);
  int httpCode = http.GET();                                  //Send the request
  Serial.println(httpCode);  
    if (httpCode > 0) { //Check the returning code
 
      String payload = http.getString();   //Get the request response payload
      Serial.println(payload);             //Print the response payload
      //Serial.println(header);             //Print the response payload
 
    }
  Serial.print(mode);Serial.print("\t");
  Serial.print(SetpointV);Serial.print("\t");
  Serial.print(InputV);Serial.print("\t");
  Serial.print(OutputV);Serial.print("\t");
  Serial.print("");Serial.print("\t");
  Serial.print(SetpointC);Serial.print("\t");
  Serial.print(InputC);Serial.print("\t");
  Serial.print(OutputC);Serial.print("\t");
  Serial.print("");Serial.print("\t");
  Serial.print(voltage);Serial.print("\t");
  Serial.print(current);Serial.print("\t");
  Serial.print(capacity,5);Serial.print("\t");
  Serial.println("");

  // put your main code here, to run repeatedly:
}