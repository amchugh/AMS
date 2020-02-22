
// This is the 'node' code and is intended to be run on an Adafruit Feather 
// Huzzah (https://www.adafruit.com/product/2821)
// It also supports a 128x32 OLED FeatherWing display
// (https://learn.adafruit.com/adafruit-oled-featherwing?view=all)

// When uploading the code ensure that 
//  Board: Adafruit Feather Huzzah ESP8266
//  Upload speed: 115200
//  CPU: 80 Mhz

// Needed Libraries:
// Adafruit GFX Library - https://github.com/adafruit/Adafruit-GFX-Library
// WiFiManager for ESP8266 by tzapu - https://github.com/tzapu/WiFiManager
// Adafruit_SSD1306 - https://github.com/adafruit/Adafruit_SSD1306

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
char ipacket[UDP_TX_PACKET_MAX_SIZE];
char opacket[UDP_TX_PACKET_MAX_SIZE];

// Variables for loop
bool doSend = false;
bool wasPressed = false;
uint32_t packetNumber = 0;
uint32_t startTime;
uint8_t stationId;

void setup() {
  Serial.begin(57600);
  // Set the button pinmodes
  pinMode(A_BUTTON, INPUT_PULLUP);
  pinMode(B_BUTTON, INPUT_PULLUP);
  pinMode(C_BUTTON, INPUT_PULLUP);
  // Define the pinmode for the audio sensor
  pinMode(A0, INPUT);

  // Init the display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);

  // Try to connect to the server
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidtarget, NULL);
  // Print the "searching" message
  printSearching();
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
    yield();
  }
  Serial.println();
  Serial.println("Connected!");

  stationId = WiFi.localIP()[3];

  // Set up the UDP service
  Udp.begin(localUDPPort);

  // Print the ready message
  printConnected();

  startTime = millis();
}

// ------------------------------
// Printing section

// Print a message saying that we are connected to WiFi
void printConnected() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Station ");
  display.print(stationId);
  display.println(" Connected!");
  display.println();
  display.println("Press A to begin");
  display.display();
}

// Print a message to the display that we are searching
void printSearching() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Connecting");
  display.display();
}

void printSendingMessage() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Sending Data");

  display.setCursor(0,16);
  display.println("Packets: ");

  display.setCursor(0,24);
  display.println("SPS: ");

  display.display();
}

void printStandbyMessage(int packetCount) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Standing By");
  display.println("Press A to begin");
  display.print("Packets Sent: ");
  display.println(packetCount);
  display.display();
}

// ------------------------------

void handleUDPPacket() {
 int packetSize = Udp.parsePacket();
 if (packetSize) {
    Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
                  packetSize,
                  Udp.remoteIP().toString().c_str(), Udp.remotePort(),
                  Udp.destinationIP().toString().c_str(), Udp.localPort(),
                  ESP.getFreeHeap());

    // read the packet into packetBufffer
    int n = Udp.read(ipacket, UDP_TX_PACKET_MAX_SIZE);
    ipacket[n] = 0;
    Serial.println("Contents:");
    Serial.println(ipacket);
  }
}

// ------------------------------
// Collecting and Sending Data


/*

  PACKET STRUCTURE

The first two bits will be used for the station number.
(0, 1, 2, or 3)
This is where the data is coming from. This is encoded in the packet
instead of by the sending IP Address so we can easily spoof data
from a computer for testing purposes before we build the rest of the stations

The next six bits represent the packet number. This gives 64 numbers before looping.
64 should be plenty of width for ensuring we are reading in the correct order.
I doubt we will drop 63 packets in a row without having a larger issue.

-------------------------------------
|Bit number | 0| 1| 2| 3| 4| 5| 6| 7|
|Represents |SI|SI|PN|PN|PN|PN|PN|PN|
-------------------------------------

SI = Station ID Number
PN = Packet Number

This has also changed.
Now it is 
one byte for statiod ID
one byte for packet number,

*/

// How much space is allocated for the header (in bytes)?
#define HEADERSPACE 2

/*

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!! No, there's a far more simple way of doing this

~!~!~!~!~!~!~! Except that this might actually be the simple way

Then, the data is sent in 5 byte chunks. As the samples are 10 bits long, we need 40
bits to transmit 4 samples. 
The first 8 least significant bits of the sample will be stored in their own byte.
The other 2 bits (the 2 most significant bits) will be put into the fifth byte of the
sequence. 

--------------------------------------------------------------
|Byte        |  1  |  2  |  3  |  4  |           5           |
|Bit Numbers |0 - 7|8 -15|16-23|24-31|32|33|34|35|36|37|38|39|
|Sample      |  1  |  2  |  3  |  4  |  1  |  2  |  3  |  4  |
--------------------------------------------------------------

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

I thought about it for about 30 seconds and realized there is an easier way of encoding
that information.

4 samples will be sent in 5 byte chunks. It would be most efficient to send packets
with sample counts being a multiple of four (4 samples, 8, 12, 64, 128) as it
means there will be no empty data. It also makes reading packets a little easier.
Packets will likely have a size corrosponding to 1 + 4x
The data looks as follows

--------------------------------------
|Bit Numbers |0 - 9|10-19|20-29|30-39|
|Sample      |  1  |  2  |  3  |  4  |
--------------------------------------

.... I've made another mistake. While my new solution is far easier to conceptualize,
the code is really difficult. I think it might be smarter to use the original idea.

Here is the code I started writing

~~~~~~ void collectData()
~~~~~~ ...
  // This gives us the number of bytes to move to the right by
  int byte_off = HEADERSPACE + (currentSample*10/8);
  // This gives the number of bits to move to the right by
  int bit_off  = currentSample*10%8;

  for (int i = 0; i < 5; i++) {
    if (bit_off == 0) {
      // We can (probably) fill the rest of the byte with the rest of the data
      if (i == 0) {
        // The whole sample won't fit in the byte, so we need to only put
        // the first 8 bites
        opacket[byte_off] = d & 0xFF;
        i = 3;
      } else {
        // We need to bit-shift the data to the left so it fills the most significant
        opacket
        break;
      }
    }
  }
~~~~~ ... 

*/

// How many pieces of sensor information should be in each packet?
// MUST BE A MULTIPLE OF 4 TO ENSURE PROPER ENCODING
const uint16_t PACKETSAMPLESIZE = 16;

// What is the current index of packet information
uint16_t currentSample = 0;

// Track some simple metrics that allow a very generic idea of how things
// are going.
float accumulatedSamplesSent = 0;

// Sets the information in the header of the packet
void setupPacket() {

  // Clear out the entire packet so the bit manipulations work correctly.
  memset(opacket, 0, UDP_TX_PACKET_MAX_SIZE);

  currentSample = 0;
  opacket[0] = getStationID();
  opacket[1] = packetNumber % (256);
}

uint8_t getStationID() {
  return stationId;
}

struct Sample {
  uint16_t minValue;
  uint16_t maxValue;
  uint16_t sampleCount;
};


// In testing our hardware we can get about 50 samples in a 5 millisecond
// period.  Shorter durations here will make the unit more responsive.
// Keep in mind that we collect many samples before sending them to the
// server.
#define SAMPLE_DURATION_MS 3
struct Sample getDataSample() { 
  struct Sample result;

  result.maxValue = 0;
  result.minValue = 1025;
  result.sampleCount = 0;

  unsigned long startMillis = millis();  

  while (millis() - startMillis < SAMPLE_DURATION_MS) {
    uint16_t v = analogRead(A0);
    if (v > result.maxValue) {
      result.maxValue = v; 
    } 
    if (v < result.minValue) {
      result.minValue = v; 
    }
    result.sampleCount++;
  }
  return result;
}

uint8_t getPacketSize() {
  int groupNumber = currentSample / 4;
  return HEADERSPACE + (groupNumber) * 5;
}

// This method will get the data from the sensor and add it to the out packet
void collectData() {

  struct Sample s = getDataSample();

  // The raw data values are not useful but the difference in the minimum and
  // maximum value gives us a sense of volume level that has occured during
  // the time we have sampled the waveform. 
  uint16_t d = s.maxValue - s.minValue;

#ifdef DEBUG
  Serial.printf("collectData; min: %d, max: %d, count: %d, v: %d\n", 
    s.minValue, s.maxValue, s.sampleCount, d);
#endif

  // The 10 bit value appearing in d is placed in the outgoing packet. 
  // Current encoding puts the first 8 bits in the next available slot
  // then the last 2 bits in with a group of other high order bits.
  int group_number   = currentSample/4;
  int s_group_pos    = currentSample%4;
  int group_byte_loc = HEADERSPACE + ((group_number+1) * 5) - 1;   // -1 for index 0
  int s_byte_loc     = HEADERSPACE + (group_number*5 + s_group_pos); 

  opacket[s_byte_loc]     = d & 0xFF;
  opacket[group_byte_loc] = opacket[group_byte_loc] | ((d >> 8) << 2*s_group_pos);

  // Increment counters
  currentSample++;
  accumulatedSamplesSent++;
}

// Sends whatever is in the opacket and resets counters
void sendPacket() {
  Serial.printf("sendPacket; packetSize: %d\n", getPacketSize());

  // Send the packet
  Udp.beginPacket(destip, serverUDPPort);
  Udp.write(opacket, getPacketSize());
  Udp.endPacket();
}

// ------------------------------


/**

The goal for transmission is: 
- 25 packets per second 
- <=5% drop rate at 30ft through a wall
- 50 new data points per packet

**/

void loop() {
  if (!digitalRead(A_BUTTON)) {
    // toggle
    if (!wasPressed) {
      if (doSend) {
        doSend = false;
        printStandbyMessage(packetNumber);
      } else {
        doSend = true;
        setupPacket();
        printSendingMessage();
      }
      wasPressed = true;
    }
  } else {
    wasPressed = false;
  }

  if (doSend) {
    // We use a while loop here so we have some guarantee on the delay between
    // samples.
    while (currentSample < PACKETSAMPLESIZE) {
      collectData();
    } 
    sendPacket();
    packetNumber++;
    // Now setup the new packet
    setupPacket();
  
    display.fillRect(53,16,70,8, SSD1306_BLACK);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(53,16);
    display.print(packetNumber);

    unsigned long int elapsedTime = millis() - startTime;
    if (elapsedTime > 3000) { 
      display.fillRect(53,24,70,8, SSD1306_BLACK);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(53,24);
      display.print( (float)elapsedTime / accumulatedSamplesSent );

      startTime = millis();
      accumulatedSamplesSent = 0;
    }
    display.display();
  }

  // We will take this time to process UDP
  handleUDPPacket();
}
