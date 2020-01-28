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

// Variables for loop
bool doSend = false;
bool wasPressed = false;
int packetNumber = 0;
uint32_t stime;

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

  // Set up the UDP service
  Udp.begin(localUDPPort);

  // Print the ready message
  printConnected();
}

// ------------------------------
// Printing section

// Print a message saying that we are connected to WiFi
void printConnected() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Connected!");
  display.println( WiFi.localIP());
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
// MUST BE A MULTIPLE OF 4 TO ENSURE PROPER READING
const uint16_t PACKETSAMPLESIZE = 32;

// What is the current index of packet information
uint16_t currentSample = 0;

/*
cs = 7

h = 0
s0 = 1
s1 = 2
s2 = 3
s3 = 4
g1 = 5
s4 = 6
s5 = 7
s6 = 8
s7 = 9
g2 = 10
*/

// Sets the information in the header of the packet
void setupPacket() {
  currentSample = 0;
  opacket[0] = getStationID();
  opacket[1] = packetNumber % (256);
}

uint8_t getStationID() {
  return (WiFi.localIP()[3]);
}

// This method will get the data from the sensor and add it to the out packet
void collectData() {
  // For now, we will just store the audio information across two bytes.
  // We are also sending the raw data. I think it is unlikely that this is the
  //  data we want to send. We will probably do some of the audio processing
  //  on the stations instead of doing it all on the server
  // It is not efficient. It will work for now. I just want a Proof of Concept
  //todo:: improve
  uint16_t d = analogRead(A0);

  // Put data into group

  // get some position values
  int group_number   = currentSample/4;
  int s_group_pos    = currentSample%4;
  int group_byte_loc = HEADERSPACE + ((group_number+1) * 5) - 1;   // -1 for index 0
  int s_byte_loc     = HEADERSPACE + (group_number*5 + s_group_pos); 

  opacket[s_byte_loc]     = d & 0xFF;
  opacket[group_byte_loc] = opacket[group_byte_loc] | ((d >> 8) << 2*s_group_pos);

/*
  // sending raw data, little endian
  opacket[currentSample*2]   = d & 0xFF;
  opacket[currentSample*2+1] = d >> 8;
*/

  // Increment counter
  currentSample++;
}

// Sends whatever is in the opacket and resets counters
void sendPacket() {
  // Send the packet
  Udp.beginPacket(destip, serverUDPPort);
  Udp.write(opacket);
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
    // We use a while loop here so we have some guarantee on the delay between samples
    while (currentSample < PACKETSAMPLESIZE) {
      collectData();
      delay(1);
    } 
    sendPacket();
    packetNumber++;
    // Now setup the new packet
    setupPacket();
  }

  // We will take this time to process UDP
  handleUDPPacket();
}

/*
  // We will use the A button to toggle sending
  if (doSend) {
    // We need to collect the data we will send
    if (currentSample < PACKETSAMPLESIZE) {
      / *
      if (currentSample == 0) {
        stime = millis();
      }
      * /
      collectData();
      //delay(.1f);
    } else {
      // Send our data!
      sendPacket();
      
      // Do some debug stuff
      //todo:: remove
      ns++;

      if (ns == 1000) {
        doSend = false;
      }
      
      // Get the time difference and update display
/ *
      uint32_t diff = millis() - stime;
      printSendingMessage(ns, diff);
*b /
    }
  } else {
    if (!digitalRead(A_BUTTON)) {
      if (!wasPressed) {
        if (!doSend) {
          // We will also clear the counters
          currentSample = 0;
          ns = 0;

          doSend = true;
          // update display
          printSendingMessage(ns, 0);
        } else {
          doSend = false;
          // And display
          printStandbyMessage(ns);
        }
        wasPressed = true;
      }
    } else {
      wasPressed = false;
    }

  }
*/
