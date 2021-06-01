#include <Arduino.h>

#include <Wire.h>
#include <charger.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <secret.h>

#define discharge_pin D8
#define charge_pin D4

ESP8266WebServer server(80);

Charger charger(charge_pin, discharge_pin);

void httpGetData();
void httpReset();
void httpStop();
void httpCharge();
void httpDischarge();


void setup(void)
{
  Serial.begin(115200);
  WiFi.begin(ssid,password);
  WiFi.hostname("ESP-Charger");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  
  charger.begin();

  server.on("/getdata", httpGetData);
  server.on("/reset", httpReset);
  server.on("/stop", httpStop);
  server.on("/charge", httpCharge);
  server.on("/discharge", httpDischarge);
  server.begin();
  
  
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  
}
int power = 0;
float target = 0;

enum modes {stop, charge, discharge};

modes mode = stop;

float old = 0;
float capacity = 0;
float energy = 0;

float voltage;
float current;
float watt;
void loop(void)
{
  server.handleClient();
  if(Serial.available() > 0){
   target = Serial.parseFloat();
   Serial.println(target);
  }
  
  float sum_voltage = 0;
  float sum_current = 0;
  for(int i = 0; i < 5; i++){
    sum_voltage += charger.get_voltage();
    sum_current += charger.get_current();
  }
  voltage = sum_voltage / 5;
  current = sum_current / 5;
  watt = voltage * current / 1000;

  float now = millis();
  capacity += (current * (now-old) / 60 / 60 / 1000);
  energy += (watt * (now-old) / 60 / 60 / 1000);
  old = now;

  if (mode == stop){
    charger.stop();
  }
  else if(mode == charge){
    charger.charge();
  }
  else if(mode == discharge){
    charger.discharge();
  }


  delay(10);
}

void httpGetData(){
  char foo[255];
  float cp = charger.charge_power / 10.24;
  float dcp = charger.discharge_power / 10.24;
  sprintf(foo,"{\"voltage\":%.2f,\"current\":%.2f,\"watt\":%.2f,\"capacity\":%.2f,\"energy\":%.2f,\"chargePower\":%.2f,\"dischargePower\":%.2f}", voltage, current, watt, capacity, energy, cp, dcp);
  server.send(200, "text/plain", foo);
}

void httpStop(){
  mode = stop;
  server.send(200, "text/plain", "charger stoped");
}

void httpReset(){
  capacity = 0;
  energy = 0;
  server.send(200, "text/plain", "capacity and energy set to 0.0");
}

void httpCharge(){
  if(server.args() > 0){
    //https://example.org/charge?u=4100&i=500
    for(int i = 0; i < server.args(); i++){
      if(server.argName(i) == "u")
        charger.charge_voltage = server.arg(i).toFloat();
      else if(server.argName(i) == "i")
        charger.charge_current = server.arg(i).toFloat();
      Serial.print(server.argName(i));
      Serial.println(server.arg(i));
    }
  }
  if(mode == stop || mode == charge){
    mode = charge;
    server.send(200, "text/plain", "charging");
  }
  else{
    server.send(200, "text/plain", "Can't start charging.\n!Charger is allready runnging!");
  }
}

void httpDischarge(){
  if(server.args() > 0){
    //https://example.org/discharge?u=3100&i=500
    for(int i = 0; i < server.args(); i++){
      if(server.argName(i) == "u")
        charger.discharge_voltage = server.arg(i).toFloat();
      else if(server.argName(i) == "i")
        charger.discharge_current = server.arg(i).toFloat();
      Serial.print(server.argName(i));
      Serial.println(server.arg(i));
    }
  }
  if(mode == stop || mode == discharge){
    mode = discharge;
    server.send(200, "text/plain", "discharging");
  }
  else{
    server.send(200, "text/plain", "Can't start charging.\n!Charger is allready runnging!");
  }
}