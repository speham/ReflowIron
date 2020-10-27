// this example is public domain. enjoy!
// https://learn.adafruit.com/thermocouple/

#include <max6675.h>
#include <LiquidCrystal.h>
#include <Wire.h>

int thermoDO = D6;
int thermoCS = D7;
int thermoCLK = D5;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);


void setup() {
  Serial.begin(115200);
  
  // wait for MAX chip to stabilize
  delay(500);
  Serial.println("setup done");
}

void loop() {
  // basic readout test, just print the current temp

  Serial.print("Temperatur: ");
  Serial.print(thermocouple.readCelsius());
  Serial.print(" C \t\t");

  Serial.print(thermocouple.readFahrenheit());
  Serial.println(" F");
  
  delay(1000);
}
