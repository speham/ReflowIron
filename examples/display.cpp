#include <Arduino.h>
/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x32 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES 10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16
static const unsigned char PROGMEM logo_bmp[] =
    {B00000000, B11000000,
     B00000001, B11000000,
     B00000001, B11000000,
     B00000011, B11100000,
     B11110011, B11100000,
     B11111110, B11111000,
     B01111110, B11111111,
     B00110011, B10011111,
     B00011111, B11111100,
     B00001101, B01110000,
     B00011011, B10100000,
     B00111111, B11100000,
     B00111111, B11110000,
     B01111100, B11110000,
     B01110000, B01110000,
     B00000000, B00110000};

#define CHANNEL 0
#define POWER 1
#define SPREADF 2
#define PORT 3
#define TXL 4
#define TXD 5
#define ACK 6
#define SEND 7

int b = SEND;

//--- user interface setup ---
typedef struct
{
  String label;
  int x;
  int y;
  int min;
  int max;
  int value;
  int format;
} ui_type;

#define MAX_UI 8
String nodeid = "myNode";

ui_type ui[MAX_UI] = {
    {"CH ", 0, 1, -1, 7, 0, DEC},
    {"PW ", 5, 1, 0, 20, 14, DEC},
    {"SF ", 0, 2, 6, 12, 7, DEC},
    {"PO ", 5, 2, 1, 99, 1, DEC},
    {"DL ", 0, 3, 0, 51, 1, DEC},
    {"TX ", 5, 3, 0, 255, 0xAB, HEX},
    {"AK ", 0, 4, 0, 1, 0, DEC},
    {"-> ", 5, 4, 0, 1, 0, DEC},
};

void setCursorChar(int x, int y)
{
  //for text size=1
  display.setCursor(x * 6, y * 8);
}

void drawUI()
{
  display.clearDisplay();

  setCursorChar(0, 0);
  display.setTextColor(WHITE);
  display.print(nodeid);

  for (int i = 0; i < 3; i++)
  {
    setCursorChar(ui[i].x, ui[i].y);
    if (i == b)
      display.setTextColor(BLACK, WHITE);
    else
      display.setTextColor(WHITE);

    display.print(ui[i].label);
    display.print(ui[i].value, ui[i].format);
  }
  display.display();
}

void setup()
{
  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  Serial.println("TextSize(1)");

  display.setTextSize(1); //10x6
  drawUI();

  Serial.println("display.setTextSize(1) done");
}

void loop()
{
}
