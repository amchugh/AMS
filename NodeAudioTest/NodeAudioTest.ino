// Needed Libraries:
// Adafruit GFX Library - https://github.com/adafruit/Adafruit-GFX-Library
// Adafruit_SSD1306 - https://github.com/adafruit/Adafruit_SSD1306

// This is some tester code to display the value of the microphone.
// Intended to be run on an Adafruit Feather 
// Huzzah (https://www.adafruit.com/product/2821)
// It also supports a 128x32 OLED FeatherWing display
// (https://learn.adafruit.com/adafruit-oled-featherwing?view=all)

// Mic is an adjustable gain https://www.adafruit.com/product/1063

// When uploading the code ensure that 
//  Board: Adafruit Feather Huzzah ESP8266
//  Upload speed: 115200
//  CPU: 80 Mhz

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// Define the display
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);


// Define the button pins
#define A_BUTTON 0
#define B_BUTTON 16
#define C_BUTTON 2

#define DISABLE_PIN 14


void setup() {
  Serial.begin(57600);

  // Set the button pinmodes
  pinMode(A_BUTTON, INPUT_PULLUP);
  pinMode(B_BUTTON, INPUT_PULLUP);
  pinMode(C_BUTTON, INPUT_PULLUP);
  // Define the pinmode for the audio sensor
  pinMode(A0, INPUT);
 
  // Define the disable pin as being low.  This will
  // pull the enable pin to the disable pin's value (low) when the 
  // button is pressed which will turn off the device.
  pinMode(DISABLE_PIN, OUTPUT);
  digitalWrite(DISABLE_PIN, LOW);

  delay(1000);

  // Init the display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  delay(1000);
}

bool displayMenu = true;


// Transform the value passed in, in the range of [0, highValue)
// to be within the [0,transform max value).
//
// This transform is not linear.  Small changes early on have a greater
// impact on the resulting value.
int nonLinearMidScale(unsigned int value, 
  unsigned int highValue, 
  unsigned int transformMax) 
{
  if (value>highValue) { 
    value = highValue;
  }

  // Using https://www.desmos.com/calculator/auubsajefh
  // The function y=1-e^{-4x} on a domain 0-1 results in
  // a value very close to 0-1 with the curve that I'm looking for.
  double v = ( (float) value ) / ((float) highValue);
  double y = 1 - ( exp( - 4 * v) );
   
  if (y<0) { 
    y = 0;
  } else if (y>1) { 
    y = 1;
  }

  return y * transformMax;
}
 
void loop() {
  if (displayMenu) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);

    display.println( " A - single range");
    display.println( " B - Waveform");
    display.println( " C - continuous" );
    display.println( "Use C to exit modes");
    display.display();
    displayMenu = false;
    delay(200);
  }

  // C is continuous
  if (!digitalRead(C_BUTTON)) {
    while( !digitalRead(C_BUTTON) ) { delay(1); }  // Wait for the button to be released. 
    delay(50);

    while(digitalRead(C_BUTTON) ) { 
      unsigned long startMillis = millis();  
      unsigned int maxValue = 0;
      unsigned int minValue = 1025;

      while (millis() - startMillis < 50){
        int v  = analogRead(A0);
        if (v > maxValue) {
          maxValue = v; 
        } else if (v < minValue) {
          minValue = v; 
        }
      }
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0,0);
      display.print( "Max: ");
      display.print( maxValue );
      display.print( ", Min: " );
      display.println( minValue );
      display.print( "R: " );
      display.print( maxValue - minValue );
      display.println();
      display.println( "C to exit");
      display.display();
      delay(50);
    }

    displayMenu = true;
  }

  // B is waveform
  if (!digitalRead(B_BUTTON)) {
    display.clearDisplay();
    int x = 0;

    while (digitalRead(C_BUTTON) ) { 
      unsigned long startMillis = millis();  
      unsigned int maxValue = 0;
      unsigned int minValue = 1025;

      while (millis() - startMillis < 10){
        int v  = analogRead(A0);
        if (v > maxValue) {
          maxValue = v; 
        } else if (v < minValue) {
          minValue = v; 
        }
      }

      // Meh, I'm not going to try to draw the max scaled and min scaled.
      // The problem is that 512 is not the midpoint and I would need that
      // in order to scale the way I want.
      int range = maxValue - minValue;
      int r = nonLinearMidScale(range, 512, 32);

      display.fillRect(x, 32-r, 1, r, SSD1306_WHITE);

      // Serial.printf("loop - non-linear processed; min: %d, max: %d, range: %d, mapped Range: %d\n", minValue, maxValue, range, r);

      display.fillRect(x+1, 0, 10, 32, SSD1306_BLACK);
      display.display();
      delay(10);
      x = x + 1;
      if (x>128) { 
        x = 0;
      }
    }
    displayMenu = true;
  }

  // A is pressed and then information is gathered until C is pressed.
  if (!digitalRead(A_BUTTON)) {
    int samples = 0;
    int summation = 0;
    int maxValue = 0;
    int minValue = 1025;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print( "Press C to stop reading samples");
    display.display();

    while(digitalRead(C_BUTTON)) {
      int v = analogRead(A0);
      if(v > maxValue) {
        maxValue = v;
      }
      if( v < minValue) { 
        minValue = v;
      }
      summation += v;
      samples++;
      delay(5);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print( "V: ");
    display.println( summation / samples );
    display.print( "Max: ");
    display.print( maxValue );
    display.print( ", Min: " );
    display.println( minValue );
    display.print( "R: " );
    display.print( maxValue - minValue );
    display.print( ", S: ");
    display.println(samples);
    display.println("Hit B to continue");
    display.display();
    while( digitalRead(B_BUTTON) ) { 
      delay(10);
    }
    delay(20);
    displayMenu = true;
  }
}
