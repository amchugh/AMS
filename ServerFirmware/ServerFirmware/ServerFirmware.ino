
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
#include <Adafruit_STMPE610.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include "Station.h"
#include "PacketDecoder.h"
#include "GfxGraphing.h"

// Defines used for the TFT display
#define STMPE_CS 16
#define TFT_CS   0
#define TFT_DC   15
#define SD_CS    2

// Keeping things simple with a maximum number of stations that are tracked with this instance.
#define MAX_NUMBER_STATIONS 4
Station stations[MAX_NUMBER_STATIONS];

const char *ssid = "AMS-server";
unsigned int localUDPPort = 8888;

SegmentedBarGraph *g;

// Allocate buffers for sending and receiving UDP data.
// The maximum UDP packet size is defined in https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiUdp.h
// and, as of 1/22/2020, is 8k.  
byte incomingPacket[UDP_TX_PACKET_MAX_SIZE + 1]; 

// An HTTP server exists for diagnostic and debugging purposes
ESP8266WebServer server(80);

WiFiUDP Udp;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

uint32_t lastGraphRenderTime = 0;

void setup() {
  Serial.begin(57600);

  delay(10);
  Serial.println("Server firmware started.");
  tft.begin();
  tft.setRotation(1);

  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }

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

  WiFi.softAP(ssid,NULL); 

  displayWelcome();

  // Start listening for packets on the UDP port.
  Udp.begin(localUDPPort);
    
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  for (int i=0; i<MAX_NUMBER_STATIONS; i++) { 
    initializeStation(stations[i]);
  }

  // g = new SegmentedBarGraph( tft, 10, 120, 24, 96 );
  g = new SegmentedBarGraph( tft, 260, 40, 24, 176 );
  g->setBackgroundColor(ILI9341_BLACK);
  g->setBorderColor(ILI9341_WHITE);
  g->setMinAndMaxYAxisValues(0, 1024);
  g->setSegmentGroupColors(ILI9341_GREEN, ILI9341_YELLOW, ILI9341_RED);
  g->startGraphing();
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

  tft.setCursor(0,50);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.print( "IP Address: ");
  tft.println(WiFi.softAPIP());

  tft.setCursor(0,70);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.println( "Number connections: 0");
  tft.println( "Number packets received: 0");
}

int numberOfPacketsReceived = 0;
bool updateDisplayRequired = false;

// todo:: add a check utilizing bool isPacketValid(Station &, PacketNumber) to
// todo:: avoid using old or repeat packets.
void handleUDPPacket() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    uint32_t currentTime = millis();
    updateDisplayRequired = true;
    numberOfPacketsReceived += 1;

    // read the packet into packetBufffer
    int n = Udp.read(incomingPacket, UDP_TX_PACKET_MAX_SIZE);

    PacketData p;
    decodePacket(p, incomingPacket, n);
    
    IPAddress sender = Udp.remoteIP();
    int stationIndex = findStation(stations, MAX_NUMBER_STATIONS, p.packetSenderID);
    if (stationIndex == -1) { 
      stationIndex = addStation(stations, MAX_NUMBER_STATIONS, p.packetSenderID);
      Serial.printf("handleUDPPacket - new station detected; IP: %s, ID: %d, AllocatedIndex: %d\n", 
        sender.toString().c_str(), p.packetSenderID, stationIndex);
    } 

    if (stationIndex >= 0) { 
      Serial.printf("handleUDPPacket - processing data; IP: %s, ID: %d, stationIndex: %d, packetNumber: %d, datapoints: %d\n",
        sender.toString().c_str(), p.packetSenderID, stationIndex, p.packetNumber, p.sampleCount);
      for (int i=0; i < p.sampleCount; i++) { 
        Serial.printf("handleUDPPacket - index: %2d, data: %d\n", i, p.samples[i]);
        addDataPoint(stations[stationIndex], p.packetNumber, p.samples[i], currentTime);
        g->addDatasetValue(p.samples[i]);
      }
    } else {
      Serial.println("handleUDPPacket - discarding data due to all station slots full");
    }
  }
}

uint8_t priorAPStationNum = 0;

void updateDisplayStats() {

  if( WiFi.softAPgetStationNum() != priorAPStationNum ) { 
    updateDisplayRequired = true;
  }
  if (updateDisplayRequired == false) { 
    return;
  }

  updateDisplayRequired = false;
  priorAPStationNum = WiFi.softAPgetStationNum();
  
  // Clear the prior lines
  tft.fillRect(119,70,40,9, ILI9341_BLACK);
  tft.fillRect(148,78,40,9, ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);

  tft.setCursor(119,70);
  tft.print(WiFi.softAPgetStationNum());

  tft.setCursor(148,78);
  tft.print(numberOfPacketsReceived);
}

void loop() {
  uint32_t currentTime = millis();

  handleUDPPacket();
  updateDisplayStats();

  if ((currentTime - lastGraphRenderTime) > 33) { 
    lastGraphRenderTime = currentTime;
    g->render();
  }

  /*
   * Code to consider when I want to respond to touch gestures.  Not sure how long this will
   * take and whether we should be careful about impact on receiving packets.
   *
   TS_Point p = ts.getPoint();

   if (p.z < 100) { 
     priorAPStationNum = 0;
     numberOfPacketsReceived = 0;
   }
  */
}
