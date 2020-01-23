
// Libraries necessary for this firmware:
//   Adafruit GFX Library - https://github.com/adafruit/Adafruit-GFX-Library
//   Adafruit ILI9341 Library - https://github.com/adafruit/Adafruit_ILI9341
//   Adafruit STMPE610 - https://github.com/adafruit/Adafruit_STMPE610
//   Mini Grafx by Daniel Eichhorn - https://github.com/ThingPulse/minigrafx
//   WiFiManager for ESP8266 by tzapu - https://github.com/tzapu/WiFiManager

// 
// This is the 'server' code and is intended to be run on an Adafruit Feather 
// Huzzah (https://www.adafruit.com/product/2821) coupled with a TFT 
// Featherwing 2.4" touchscreen (https://www.adafruit.com/product/3315).  
// 
// When uploading the code ensure that 
//  Board: Adafruit Feather Huzzah ESP8266
//  Upload speed: 115200
//  CPU: 80 Mhz


#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>


// Defines used for the TFT display
#define STMPE_CS 16
#define TFT_CS   0
#define TFT_DC   15
#define SD_CS    2

const char *ssid = "AMS-server";
unsigned int localUDPPort = 8888;

// Allocate buffers for sending and receiving UDP data.
// The maximum UDP packet size is defined in https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiUdp.h
// and, as of 1/22/2020, is 8k.  
char incomingPacket[UDP_TX_PACKET_MAX_SIZE + 1]; 
char outgoingPacket[UDP_TX_PACKET_MAX_SIZE + 1];

// An HTTP server exists for diagnostic and debugging purposes
ESP8266WebServer server(80);

WiFiUDP Udp;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
  Serial.begin(9600);

  delay(10);
  Serial.println("Server firmware started.");
  tft.begin();
  tft.setRotation(1);

  // TFT diagnostic information that can be useful for debugging.
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 

  displayWelcome();
  WiFi.softAP(ssid,NULL); 
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("IP address: ");
  Serial.println(myIP);

  tft.setCursor(0,50);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.print( "IP Address: ");
  tft.println(myIP);

  // Start listening for packets on the UDP port.
  Udp.begin(localUDPPort);
    
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void displayWelcome() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_RED);  tft.setTextSize(2);
  tft.println("AMS Server");
  tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(1);
  tft.print("Entering into AP mode with SSID: ");
  tft.println(ssid);
}

void handleUDPPacket() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
                  packetSize,
                  Udp.remoteIP().toString().c_str(), Udp.remotePort(),
                  Udp.destinationIP().toString().c_str(), Udp.localPort(),
                  ESP.getFreeHeap());

    // read the packet into packetBufffer
    int n = Udp.read(incomingPacket, UDP_TX_PACKET_MAX_SIZE);
    incomingPacket[n] = 0;
    Serial.println("Contents:");
    Serial.println(incomingPacket);

    // Handle the incoming packet appropriately.
    // Formulate a response that indicates that the packet was received.
    
    strcpy( outgoingPacket, "Gotit");
    
    // send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(outgoingPacket);
    Udp.endPacket();
  }
}

void loop() {
  handleUDPPacket();
 
}
