
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


// DEBUG_PRINT* defines will output via the serial interface debug information 
#define DEBUG_PRINT
#define DEBUG_PRINT_SHOW_DATA_DETAILS

// Keeping things simple with a maximum number of stations that are tracked with this instance.
#define MAX_NUMBER_STATIONS 4
Station stations[MAX_NUMBER_STATIONS];

const char *ssid = "AMS-server";
unsigned int localUDPPort = 8888;

SegmentedBarGraph *g[MAX_NUMBER_STATIONS];

// Allocate buffers for sending and receiving UDP data.
// The maximum UDP packet size is defined in https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiUdp.h
// and, as of 1/22/2020, is 8k.  
byte incomingPacket[UDP_TX_PACKET_MAX_SIZE + 1]; 

// An HTTP server exists for diagnostic and debugging purposes
ESP8266WebServer server(80);

WiFiUDP Udp;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// The number of milliseconds to delay between each frame render.
#define RENDER_FRAME_DELAY_MS 100
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

#define XOFFSET 40
#define WIDTH 320

#define SEGMENTSIZE ((WIDTH - (XOFFSET*2)) / MAX_NUMBER_STATIONS)

 //  0     40    100    160    220    280     320
 //  |     |      +      +      +       |      |


  for (int i=0; i<MAX_NUMBER_STATIONS; i++) { 
    initializeStation(stations[i]);
    g[i] = new SegmentedBarGraph( tft, XOFFSET + (SEGMENTSIZE * i) + ((SEGMENTSIZE - 2*24)/2), 48, 24, 168 );
    g[i]->setBackgroundColor(ILI9341_BLACK);
    g[i]->setBorderColor(ILI9341_WHITE);
    g[i]->setMinAndMaxYAxisValues(0, 1024);
    g[i]->setSegmentGroupColors(ILI9341_GREEN, ILI9341_YELLOW, ILI9341_RED);
    g[i]->startGraphing();
  }
  for (int i=0; i<(MAX_NUMBER_STATIONS-1); i++) { 
    tft.drawFastVLine(XOFFSET + (SEGMENTSIZE * i) + 46, 42, 184, ILI9341_RED);
  }
}

void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void displayWelcome() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0); tft.setTextColor(ILI9341_RED); tft.setTextSize(2);
  tft.println("AMS Server");

  tft.setCursor(0,20);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.println( "Conns: 0");
  tft.setCursor(70,20);
  tft.println( "Packets: 0");
}

int numberOfPacketsReceived = 0;
bool updateDisplayRequired = false;

void handleUDPPacket() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    uint32_t currentTime = millis();
    updateDisplayRequired = true;
    numberOfPacketsReceived += 1;

    // read the packet into packetBufffer
    memset(incomingPacket, 0, UDP_TX_PACKET_MAX_SIZE + 1);
    int n = Udp.read(incomingPacket, UDP_TX_PACKET_MAX_SIZE);
    Serial.printf(
      "handleUDPPacket - got data; parsePacket packetSize: %d, dataRead: %d\n",
      packetSize, n);

    PacketData p;
    decodePacket(p, incomingPacket, n);
    
    IPAddress sender = Udp.remoteIP();
    int stationIndex = findStation(stations, MAX_NUMBER_STATIONS, p.packetSenderID);

    // When findStation returns -1 then this station is unknown to the server. 
    // Ideally we just add the station to the list of tracked stations but the 
    // number of these is finite.
    if (stationIndex == -1) { 
      // Attempt to add a station and get a stationIndex.  This might result in
      // a -1 for stationIndex if there are no free slots.
      stationIndex = addStation(stations, MAX_NUMBER_STATIONS, p.packetSenderID);
      Serial.printf(
        "handleUDPPacket - new station detected; IP: %s, ID: %d, AllocatedIndex: %d\n", 
        sender.toString().c_str(), p.packetSenderID, stationIndex);
    } 

    if (stationIndex >= 0) { 
      if ( !isPacketValid(stations[stationIndex], p.packetNumber) ) { 
        Serial.printf("handleUDPPacket - invalid packet seen, discarding; IP: %s, "
                      "ID: %d, stationIndex: %d, packetNumber: %d, datapoints: %d\n",
          sender.toString().c_str(), p.packetSenderID, stationIndex, p.packetNumber, 
          p.sampleCount);
      } else { 
#ifdef DEBUG_PRINT
        Serial.printf("handleUDPPacket - valid packet seen; IP: %s, ID: %d, "
                      "stationIndex: %d, packetNumber: %d, datapoints: %d\n",
          sender.toString().c_str(), p.packetSenderID, stationIndex, p.packetNumber, 
          p.sampleCount);
#endif
        for (int i=0; i < p.sampleCount; i++) { 
#ifdef DEBUG_PRINT_SHOW_DATA_DETAILS
          Serial.printf("handleUDPPacket; currentTime: %d, IP: %s, ID: %d, "
                        "stationIndex: %d, packetNumber: %d, dataIndex: %d, data: %d\n", 
                        currentTime, sender.toString().c_str(),
                        p.packetSenderID, stationIndex, p.packetNumber, i,
                        p.samples[i]);
#endif
          addDataPoint(stations[stationIndex], p.packetNumber, p.samples[i], currentTime);
          g[stationIndex]->addDatasetValue(p.samples[i]);
        }
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
  tft.fillRect(41,20,18,9, ILI9341_BLACK);
  tft.fillRect(124,20,40,9, ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);

  tft.setCursor(41,20);
  tft.print(WiFi.softAPgetStationNum());

  tft.setCursor(124,20);
  tft.print(numberOfPacketsReceived);
}

void loop() {
  uint32_t currentTime = millis();

  handleUDPPacket();
  updateDisplayStats();

  if ((currentTime - lastGraphRenderTime) > RENDER_FRAME_DELAY_MS) { 
    lastGraphRenderTime = currentTime;
    for (int i=0; i<MAX_NUMBER_STATIONS; i++) { 
      g[i]->render();
    }
  }

  /*
   * Code to consider when I want to respond to touch gestures.  Not sure if
   * this code will create a performance bottleneck and impact on the server's 
   * ability to read and consume UDP packets.  This cannot be compromised.
   
   TS_Point p = ts.getPoint();

   if (p.z < 100) { 
     priorAPStationNum = 0;
     numberOfPacketsReceived = 0;
   }
  */
}
