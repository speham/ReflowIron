#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "max6675.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool displayReflowTargetTempChange = true;


int thermoDO = D6;
int thermoCS = D7;
int thermoCLK = D5;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

const int led = D3;
const int button = D4;
const int solidstate = D8;
const int poti = A0;
const int temp_preheat = 150;
const int temp_reflow = 240;


typedef enum
{
  NO_CLICK,
  SHORT_CLICK,
  LONG_CLICK
} ButtonClick;

typedef enum
{
  OFF = 0,
  PREHEAT = 1,
  REFLOW = 2,
  COOLING = 3
} State;

int temp_now = 0;
int temp_next = 0;
int temp_poti = 0;
int temp_poti_old = 0;
String state[] = {"OFF", "PREHEAT", "REFLOW", "COOLING"};
// int state_now = 0;
State actualState = OFF;

int time_count = 0;
int perc = 0;
int offset = 0;

long millisDisplayRefresh = millis();
long t_solder = millis();

int X(int textgroesse, int n)
{
  return (0.5 * (display.width() - textgroesse * (6 * n - 1))); //end int X
}
int Y(int textgroesse, float f)
{
  return (f * display.height() - (textgroesse * 4)); //end int Y
}

void regulate_temp(int temp, int should)
{
  if (should <= temp - offset)
  {
    digitalWrite(solidstate, LOW);
    digitalWrite(led, LOW);    
  }
  else if (should > temp + offset)
  {
    digitalWrite(solidstate, HIGH);
    digitalWrite(led, HIGH);
  }
}

void PrintScreen(String state, int soll_temp, int ist_temp, int tim, int percentage)
{
  String str = String(soll_temp) + " deg";
  if (!displayReflowTargetTempChange)
  {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(state);
    display.setCursor(80, 0);
    display.println(str);
  }
  

  if (tim != 0)
  {
    display.setCursor(0, 50);
    str = String(tim) + " sec";
    display.println(str);
  }
  if (percentage != 0)
  {
    display.setCursor(80, 50);
    str = String(percentage) + " %";
    display.println(str);
  }
  
  if (!displayReflowTargetTempChange)
  {
    display.setTextSize(2);
    display.setCursor(30, 22);
    str = String(ist_temp) + " deg";
    display.println(str);
    display.display();
  }
}

int readPotiTemeratur()
{ 
  int potiTemperatur = map(analogRead(poti), 0, 1023, temp_preheat, temp_reflow);

  static long lastPotiTempChange;
  long now = millis();

  if (potiTemperatur >= temp_poti_old + 1 || potiTemperatur <= temp_poti_old - 1)
  {
    display.fillScreen(WHITE);
    display.setTextColor(BLACK);
    display.setTextSize(2);
    display.setCursor(X(1, 10), Y(1, 0.1));
    display.println("REFLOW");
    display.setTextSize(2);
    display.setCursor(X(2, 3), Y(2, 0.5));
    display.println(String(potiTemperatur));
    display.display();

    Serial.printf("Neuer Sollwert: %i °C\n", potiTemperatur);
    lastPotiTempChange = now;
    displayReflowTargetTempChange = true;
    temp_poti_old = potiTemperatur;
    return potiTemperatur;
  }
  else if (now > lastPotiTempChange + 2000)
  {
    displayReflowTargetTempChange = false;
    return potiTemperatur;
  }
  
  return potiTemperatur;
}

void refreshDisplay()
{
  if (millis() > millisDisplayRefresh + 10 || millis() < millisDisplayRefresh)
  {
    //Serial.printf("Measured Temperature: %i\n", temp_now);
    PrintScreen(state[actualState], temp_next, temp_now, time_count, perc);
    millisDisplayRefresh = millis();
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("setup starts noww ...");
  pinMode(button, INPUT);
  pinMode(solidstate, OUTPUT);
  digitalWrite(solidstate, LOW);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  delay(200);
  digitalWrite(led, HIGH);
  delay(200);
  digitalWrite(led, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

ButtonClick detectButtonClick()
{
  if (digitalRead(button) == 0)
  {
    Serial.println("clicked");
    if (displayReflowTargetTempChange)
    {
      displayReflowTargetTempChange =false;
      Serial.println("Display cleared");
      display.clearDisplay();
      display.display();
    }
    
    delay(100);
    long milSec = millis();
    while (digitalRead(button) == 0)
    {
      yield();
      if (millis() > milSec + 1500)
      {
        return LONG_CLICK;
      }
    }
    Serial.println("shortclicked");

    return SHORT_CLICK;
  }

  return NO_CLICK;
}

void switchOff()
{
  Serial.println("Long Button Click detected - Switch OFF now.\n");
  digitalWrite(solidstate, LOW);
  digitalWrite(led, LOW);    
  actualState = OFF;
  display.fillScreen(WHITE);
  display.setTextColor(BLACK);
  display.setTextSize(2);
  display.setCursor(X(2, 3), Y(2, 0.5));
  display.println("OFF");
  display.display();
  while (digitalRead(button) == 0)
  {
    yield();
    delay(1);
  }
}

void nextState()
{
  t_solder = millis();
  perc = 0;

  switch (actualState)
  {
  case OFF:
    actualState = PREHEAT;
    temp_next = temp_preheat;
    break;
  case PREHEAT:
    actualState = REFLOW;
    temp_next = temp_poti;
    break;
  case REFLOW:
    actualState = COOLING;
    temp_next = 0;
    break;
  case COOLING:
    actualState = OFF;
    temp_next = 0;
    break;
  }

  Serial.print("Short Button Click detected - Switch to State ");
  Serial.println(state[actualState]);
}

void executeActualState()
{
  switch (actualState)
  {
  case PREHEAT:
    regulate_temp(temp_now, temp_next);
    perc = int((float(temp_now) / float(temp_next)) * 100.00);
    break;
  case REFLOW:
    regulate_temp(temp_now, temp_next);
    perc = int((float(temp_now) / float(temp_next)) * 100.00);
    if (perc >= 100)
    {
      actualState = COOLING;
      t_solder = millis();
      perc = 0;
      temp_next = 0;
    }
    break;
    {
    case COOLING:
      digitalWrite(solidstate, LOW);
      digitalWrite(led, LOW);    
      time_count = int((t_solder + 60000 - millis()) / 1000);
      if (time_count <= 0)
      {
        actualState = OFF;
      }
      break;
    default:
      digitalWrite(solidstate, LOW);
      digitalWrite(led, LOW);    
      time_count = 0;
      break;
    }
  }
}

void loop()
{
  
  long now = millis();
  static long lastTempRead;
  static int lastTemp;

  if ( now > lastTempRead + 200)            // Needed for MAX6675. Otherwise temperature values does not change
  {
    if (actualState == OFF)
    {
      temp_poti = readPotiTemeratur();
    }
    
    temp_now = thermocouple.readCelsius();  
    lastTempRead = now;

    if (lastTemp != temp_now)
    {
      Serial.printf("2 - %i\n", temp_now);
      lastTemp = temp_now;
      if (temp_now > 1000)
      {
        Serial.println("Temperature sensor maybe damaged");
        temp_now = 1000;
      }
    }
}
  
  refreshDisplay();

  ButtonClick cl = detectButtonClick();
  switch (cl)
  {
  case LONG_CLICK:
    switchOff();
    return;
  case SHORT_CLICK:
    nextState();
    break;
  default:
    break;
  }

  executeActualState();
  
}
