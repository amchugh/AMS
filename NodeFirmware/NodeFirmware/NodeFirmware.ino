// Needed Libraries:
// Adafruit GFX Library - https://github.com/adafruit/Adafruit-GFX-Library
// WiFiManager for ESP8266 by tzapu - https://github.com/tzapu/WiFiManager
// Adafruit_SSD1306 - https://github.com/adafruit/Adafruit_SSD1306

// This is the 'node' code and is intended to be run on an Adafruit Feather 
// Huzzah (https://www.adafruit.com/product/2821)
// It also supports a 128x32 OLED FeatherWing display
// (https://learn.adafruit.com/adafruit-oled-featherwing?view=all)

// When uploading the code ensure that 
//  Board: Adafruit Feather Huzzah ESP8266
//  Upload speed: 115200
//  CPU: 80 Mhz

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// Define server constants
const char *ssidtarget = "AMS-server";

// Location of the server
IPAddress destip(192,168,4,1);
unsigned int serverUDPPort = 8888;

// Setup our listen port
unsigned int localUDPPort = 8888;

// Define the display
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

// Define the Udp service!
WiFiUDP Udp;

// Define the button pins
#define A_BUTTON 0
#define B_BUTTON 16
#define C_BUTTON 2

// Define buffers for UDP data
char ipacket[UDP_TX_PACKET_MAX_SIZE + 1];
char opacket[UDP_TX_PACKET_MAX_SIZE + 1];

void setup() {
  Serial.begin(9600);
  // Set the button pinmodes
  pinMode(A_BUTTON, INPUT_PULLUP);
  pinMode(B_BUTTON, INPUT_PULLUP);
  pinMode(C_BUTTON, INPUT_PULLUP);
  // Define the pinmode for the audio sensor
  pinMode(A0, INPUT);

  // Init the display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  // Clear the display
  display.clearDisplay();
  display.display();

  // Try to connect to the server
  WiFi.begin(ssidtarget, NULL);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected!");
  printConnected();

  // Set up the UDP service
  Udp.begin(localUDPPort);
}

void printConnected() {
  // Print a little message saying that we are connected
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Connected!");
  display.display();
  yield();
}

void sendPacket() {
  // Encode the outgoing message
  strcpy(opacket, "Hello World!");

  // Send the packet
  Udp.beginPacket(destip, serverUDPPort);
  Udp.write(opacket);
  Udp.endPacket();
  yield();
  delay(1);

  // Print to serial so we know what happened
  Serial.println("Sent a UDP packet");
}

void loop() {
  // Wait until the A button is pressed to send packets
  if (!digitalRead(A_BUTTON)) {
    sendPacket();
    delay(10);
  }
}
