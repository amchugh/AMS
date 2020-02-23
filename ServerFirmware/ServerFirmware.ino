
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
#include "SmartTextField.h"

// Defines used for the TFT display
#define STMPE_CS 16
#define TFT_CS   0
#define TFT_DC   15
#define SD_CS    2


// DEBUG_PRINT* defines will output via the serial interface debug information 
// #define DEBUG_PRINT
// #define DEBUG_PRINT_SHOW_DATA_DETAILS

// Keeping things simple with a maximum number of stations that are tracked with this instance.
#define MAX_NUMBER_STATIONS 4
Station stations[MAX_NUMBER_STATIONS];

const char *ssid = "AMS-server";
unsigned int localUDPPort = 8888;

// Graphical elements
SegmentedBarGraph *g[MAX_NUMBER_STATIONS];
SmartTextField<int> *connTextField;
SmartTextField<int> *packetsTextField;

// Allocate buffers for sending and receiving UDP data.
// The maximum UDP packet size is defined in https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiUdp.h
// and, as of 1/22/2020, is 8k.  
byte incomingPacket[UDP_TX_PACKET_MAX_SIZE+1]; 

// An HTTP server exists for diagnostic and debugging purposes
ESP8266WebServer server(80);

WiFiUDP Udp;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// The number of milliseconds to delay between each frame render.
#define RENDER_FRAME_DELAY_MS 30
uint32_t lastGraphRenderTime = 0;

void setup() {
  Serial.begin(57600);

  delay(10);
  Serial.println("Server firmware started.");
  tft.begin();
  tft.setRotation(1);

  // The Wifi library for the ESP8266 tries to be too smart for its own
  // good.  At one point I had flashed another program to the server which 
  // connected to the AP.  This was saved in flash and then it would always
  // even when softAP mode was enabled, try to connect to it.  When it 
  // didn't exist then the AP SSID would go in and out and I was only able
  // to receive 1 of every 4 packets.  Finally tracked it down.  The below
  // makes it explicit that I don't want the 'smarts' found within the 
  // library.
  WiFi.setAutoConnect(false);
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);

  // Create a soft access point for the stations to connect to.
  bool result = WiFi.softAP(
    ssid,
    NULL,  /* Password - null means none */
    7,     /* Channel number 1 - 13 available.  Default is 1. */
           /* Use a program like WiFi Analyzer to find best channel */
    false, /* Is the SSID hidden? */ 
    5      /* Maximum number of connections */
  ); 
  if (!result) { 
    Serial.println("Unable to setup the softAP mode!");
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0); tft.setTextColor(ILI9341_RED); tft.setTextSize(2);
    tft.println("Error creating soft AP");
    delay(5000);
    while(1) {}
  }

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
    g[i] = new SegmentedBarGraph( tft, XOFFSET + (SEGMENTSIZE * i) + ((SEGMENTSIZE - 2*24)/2), 32, 24, 152 );
    g[i]->setBackgroundColor(ILI9341_BLACK);
    g[i]->setBorderColor(ILI9341_WHITE);
    g[i]->setMinAndMaxYAxisValues(0, 1024);
    g[i]->setSegmentGroupColors(ILI9341_GREEN, ILI9341_YELLOW, ILI9341_RED);
    g[i]->startGraphing();
  }
  for (int i=0; i<(MAX_NUMBER_STATIONS-1); i++) { 
    tft.drawFastVLine(XOFFSET + (SEGMENTSIZE * i) + 46, 22, 210, ILI9341_RED);
  }
}

void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void displayWelcome() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0); tft.setTextColor(ILI9341_RED); tft.setTextSize(2);
  tft.println("AMS Server");

  tft.setCursor(150,4);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.println( "Conns: 0");
  tft.setCursor(220,4);
  tft.println( "Packets: 0");

  connTextField = new SmartTextField<int>(tft, 191, 4, 24, 8, ILI9341_BLACK);
  packetsTextField = new SmartTextField<int>(tft, 274, 4, 64, 8, ILI9341_BLACK);
}

int numberOfPacketsReceived = 0;

int handleUDPPacket() {
  int packetsProcessed = 0;
  int packetSize = Udp.parsePacket();
  while (packetSize > 0 && packetsProcessed < 10) { 
    uint32_t currentTime = millis();
    numberOfPacketsReceived += 1;

    // read the packet into packetBufffer
    memset(incomingPacket, 0, UDP_TX_PACKET_MAX_SIZE+1);
    int n = Udp.read(incomingPacket, UDP_TX_PACKET_MAX_SIZE);
#ifdef DEBUG_PRINT
    Serial.printf(
        "handleUDPPacket - got data; parsePacket packetSize: %d, dataRead: %d\n",
        packetSize, n);
#endif
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
        Serial.printf("handleUDPPacket - valid packet seen; ID: %d, "
            "packetNumber: %d, datapoints: %d\n",
            p.packetSenderID, p.packetNumber, p.sampleCount);
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
    packetsProcessed++;
    packetSize = Udp.parsePacket();
  }
  return packetsProcessed;
}

void renderStats() {
  tft.setTextColor(ILI9341_WHITE); 
  tft.setTextSize(1);
  connTextField->render(WiFi.softAPgetStationNum());
  packetsTextField->render(numberOfPacketsReceived);
}

void loop() {
  uint32_t currentTime = millis();

  handleUDPPacket();

  uint32_t elapsedTime = currentTime - lastGraphRenderTime;
  if (elapsedTime > RENDER_FRAME_DELAY_MS) { 
    lastGraphRenderTime = currentTime;
    for (int i=0; i<MAX_NUMBER_STATIONS; i++) { 
      g[i]->render();
    }
  }
  renderStats();
}
