#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "max6675.h"
#include <PID_v1.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool showPresetSelection = true;


int thermoDOPin = D6;
int thermoCSPin = D7;
int thermoCLKPin = D5;
MAX6675 thermocouple(thermoCLKPin, thermoCSPin, thermoDOPin);

const int ledPin = D3;
const int buttonPin = D4;
const int solidstatePin = D8;
const int potiPin = A0;

// ***** PID CONTROL VARIABLES *****
double setpoint;
double input;
double output;
unsigned int windowSize = 7000;
unsigned long windowStartTime;
unsigned long nextCheck;
unsigned long nextRead;
double Kp=13, Ki=0.1, Kd=1;
PID reflowOvenPID(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT);

typedef enum
{
  NO_CLICK,
  SHORT_CLICK,
  LONG_CLICK
} ButtonClick;

typedef enum
{
  OFF = 0,
  STARTING = 1,
  PREHEATONE = 2,
  PREHEATTWO = 3,
  REFLOW = 4,
  COOLING = 5
} State, LastState;

int temp_now,temp_next,temp_poti,potiLast = 0;

String state[] = {"OFF", "STARTING", "PREHEATONE","PREHEATTWO", "REFLOW", "COOLING"};
State actualState = OFF;
LastState lastState = OFF;

//Profiles
int         profileSelected         = 0;
String      profileNames[]          = {"Sn42/Bi57.6/Ag0.4", "Standard"};
const float profilePreHeat1Temps[]  = {90, 150};
const float profilePreHeat1KP[]     = {27, 15};
const float profilePreHeat1KI[]     = {0.1, 0.1};
const float profilePreHeat1KD[]     = {1, 1};

const float profilePreHeat2Temps[]  = {130, 180};
const float profilePreHeat2KP[]     = {3, 10};
const float profilePreHeat2KI[]     = {0.1, 0.1};
const float profilePreHeat2KD[]     = {1, 1};

const float profileReflowTemps[]    = {165, 200};
const float profileReflowKP[]       = {10, 18};
const float profileReflowKI[]       = {0.1, 0.11};
const float profileReflowKD[]       = {1, 1};

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
  input = temp; 
  setpoint = should;
  reflowOvenPID.Compute();


  if (millis() - windowStartTime > windowSize)
  { //time to shift the Relay Window
    windowStartTime += windowSize;
  }

  Serial.print("input: ");
  Serial.print(input);
  Serial.print(" setpoint: ");
  Serial.print(setpoint);
  Serial.print(" output: ");
  Serial.println(output);

  if (output < millis() - windowStartTime) 
  {
    digitalWrite(solidstatePin, LOW);
    digitalWrite(ledPin, LOW);
    
  }
  else     
  {
    digitalWrite(solidstatePin, HIGH);
    digitalWrite(ledPin, HIGH);    
  }
}

void PrintScreen(String state, int soll_temp, int ist_temp, int tim, int percentage)
{
  String str = String(soll_temp);
  if (!showPresetSelection)
  {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(state);
    display.setCursor(90, 0);
    display.print(str);
    display.print((char)247);
    display.println(F("C"));

  }
  

  if (tim != 0)
  {
    display.setCursor(0, 50);
    str = String(tim) + " sec";
    display.println(str);
  }
  if (percentage != 0)
  {
    display.setCursor(90, 50);
    str = String(percentage) + " %";
    display.println(str);
  }
  
  if (!showPresetSelection)
  {
    display.setTextSize(3);
    display.setCursor(20, 25);
    str = String(ist_temp);
    display.print(str);
    display.print((char)247);
    display.println(F("C"));
    display.display();
  }
}

int readPoti()
{ 
  int poti = map(analogRead(potiPin), 0, 1023, 0, 1);

  static long lastPotiChange;
  long now = millis();

  if (poti >= potiLast + 1 || poti <= potiLast - 1)
  {
    display.fillScreen(WHITE);
    display.setTextColor(BLACK);
    display.setTextSize(1.5);
    display.setCursor(0, Y(1, 0.1));
    display.print(String(poti+1));
    display.print(" - ");
    display.println(profileNames[poti]);
    display.writeFastHLine(0,12,128,BLACK);

    Serial.printf("Profile %i selected\n", poti);

    display.setTextSize(1);
    display.setCursor(5, 25);
    if (profilePreHeat1Temps[poti] < 100) display.print("Preheat1:  ");
    else display.print("Preheat1: ");
    display.print(String(profilePreHeat1Temps[poti]));
    display.print((char)247);
    display.println(F("C"));

    display.setCursor(5, 35);
    if (profilePreHeat2Temps[poti] < 100) display.print("Preheat2:  ");
    else display.print("Preheat2: ");
    display.print(String(profilePreHeat2Temps[poti]));
    display.print((char)247);
    display.println(F("C"));

    display.setCursor(5, 45);
    display.print("Reflow:   ");
    display.print(String(profileReflowTemps[poti]));
    display.print((char)247);
    display.println(F("C"));

    display.display();
    lastPotiChange = now;
    showPresetSelection = true;
    potiLast = poti;
  }
  else if (now > lastPotiChange + 1500)
  {
    showPresetSelection = false;
  }
  return poti;
}

void refreshDisplay()
{
  if (millis() > millisDisplayRefresh + 100 || millis() < millisDisplayRefresh)
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
  pinMode(buttonPin, INPUT);
  pinMode(solidstatePin, OUTPUT);
  digitalWrite(solidstatePin, LOW);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  delay(200);
  digitalWrite(ledPin, HIGH);
  delay(200);
  digitalWrite(ledPin, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  
    // Initialize PID control window starting time
  windowStartTime = millis();
  
  setpoint = 150;

  // Tell the PID to range between 0 and the full window size
  reflowOvenPID.SetOutputLimits(0, windowSize);
  //reflowOvenPID.SetSampleTime(1000);
  // Turn the PID on
  reflowOvenPID.SetMode(AUTOMATIC);
  // Proceed to preheat stage

}

ButtonClick detectButtonClick()
{
  if (digitalRead(buttonPin) == 0)
  {
    Serial.println("clicked");
    if (showPresetSelection)
    {
      showPresetSelection =false;
      Serial.println("Display cleared");
      display.clearDisplay();
      display.display();
    }
    
    delay(200);
    long milSec = millis();
    while (digitalRead(buttonPin) == 0)
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
  digitalWrite(solidstatePin, LOW);
  digitalWrite(ledPin, LOW);    
  actualState = OFF;
  lastState = OFF;
  display.fillScreen(WHITE);
  display.setTextColor(BLACK);
  display.setTextSize(3);
  display.setCursor(X(2, 3), Y(2, 0.5));
  display.println("OFF");
  display.display();
  while (digitalRead(buttonPin) == 0)
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
    actualState = STARTING;
    lastState = OFF;
    temp_next = 0;
    break;
  case STARTING:
    actualState = PREHEATONE;
    lastState = STARTING;
    temp_next = profilePreHeat1Temps[profileSelected];
    break;
  case PREHEATONE:
    actualState = PREHEATTWO;
    lastState = PREHEATONE;
    temp_next = profilePreHeat2Temps[profileSelected];
    break;
  case PREHEATTWO:
    actualState = REFLOW;
    lastState = PREHEATTWO;
    temp_next = profileReflowTemps[profileSelected];
    break;
  case REFLOW:
    actualState = COOLING;
    lastState = REFLOW;
    temp_next = 0;
    break;
  case COOLING:
    actualState = OFF;
    lastState = COOLING;
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
  case PREHEATONE:
    Kp=profilePreHeat1KP[profileSelected], Ki=profilePreHeat1KI[profileSelected], Kd=profilePreHeat1KD[profileSelected];
    regulate_temp(temp_now, temp_next);
    perc = int((float(temp_now) / float(temp_next)) * 100.00);
    time_count = int((millis() - t_solder) / 1000);
    if (perc >= 100)
    {
      nextState();
      time_count = 0;
      Serial.println("preheat 1");
    }
    break;
  case PREHEATTWO:
    Kp=profilePreHeat2KP[profileSelected], Ki=profilePreHeat2KI[profileSelected], Kd=profilePreHeat2KD[profileSelected];
    regulate_temp(temp_now, temp_next);
    perc = int((float(temp_now) / float(temp_next)) * 100.00);
    time_count = int((millis() - t_solder) / 1000);
    if (perc >= 100)
    {
      nextState();
      time_count = 0;
      Serial.println("preheat 2");
    }
    break;
  case REFLOW:
    Kp=profileReflowKP[profileSelected], Ki=profileReflowKI[profileSelected], Kd=profileReflowKD[profileSelected];
    regulate_temp(temp_now, temp_next);
    perc = int((float(temp_now) / float(temp_next)) * 100.00);
    time_count = int((millis() - t_solder) / 1000);
    if (perc >= 100)
    {
      nextState();
      time_count = 0;
      Serial.println("reflow");
    }
    break;
    {
    case COOLING:
      digitalWrite(solidstatePin, LOW);
      digitalWrite(ledPin, LOW);    
      time_count = int((t_solder + 60000 - millis()) / 1000);
      if (time_count <= 0)
      {
        actualState = OFF;
      }
      break;
    default:
      digitalWrite(solidstatePin, LOW);
      digitalWrite(ledPin, LOW);    
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
      profileSelected = readPoti();
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
  if (actualState == STARTING)
  {
    t_solder = millis();
    temp_next = 0;
    nextState();
  }
  
}
