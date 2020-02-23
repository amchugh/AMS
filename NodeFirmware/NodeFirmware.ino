
// This is the listener 'node' code and is intended to be run on an Adafruit
// Feather Huzzah (https://www.adafruit.com/product/2821) coupled with a
// 128x32 OLED FeatherWing display
// (https://learn.adafruit.com/adafruit-oled-featherwing?view=all)
// and an electret microphone (https://www.adafruit.com/product/1063).

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

// Define buffers for UDP incoming and outgoing data
char ipacket[UDP_TX_PACKET_MAX_SIZE];
char opacket[UDP_TX_PACKET_MAX_SIZE];

bool doSend = false;
bool wasPressed = false;
uint32_t packetNumber = 0;
uint8_t stationId;
float accumulationSamplesSent = 0;
uint32_t accumulationStartTime = 0;

// The listener node collects data 'samples' and packages them together
// into a 'packet' of data that is communicated with the server.   A 
// single 'sample' is actually many readings of the microphone where we
// are looking in the distance between the high and low value.  
typedef struct {
  uint16_t minValue;
  uint16_t maxValue;
  uint16_t sampleCount;
} Sample;

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
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidtarget, NULL);

  // Print the "searching" message
  int numDots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (numDots == 0) { 
      printSearching();
    }
    delay(50);
    Serial.print(".");
    display.print(".");
    display.display();
    numDots++;
    if (numDots == 32) { 
      numDots = 0;
    }
  }
  Serial.println();
  Serial.println("Connected!");

  stationId = WiFi.localIP()[3];

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

  // SPS is 'samples per second'.  This gives us a sense of how many 
  // noise readings per second we are sending.  
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

// ---------------------------
// Collecting and Sending Data
//
// For details on the packet layout for both the header data and the 
// samples themselves see PacketDecoder.h.  

// The number of bytes allocated for the header data.  
#define HEADER_SIZE_BYTES 2

// How many pieces of sensor information should be in each packet?
// MUST BE A MULTIPLE OF 4 TO ENSURE PROPER ENCODING
const uint16_t PACKETSAMPLESIZE = 16;

// The current index for a data sample within the larger packet. 
uint16_t currentSample = 0;

// Sets the information in the header of the packet
void setupPacket() {
  // Clear out the entire packet so the bit manipulations work correctly.
  memset(opacket, 0, UDP_TX_PACKET_MAX_SIZE);

  currentSample = 0;
  opacket[0] = stationId;
  opacket[1] = packetNumber % (256);
}

// In testing our hardware we can get about 50 samples in a 5 millisecond
// period.  Shorter durations here will make the unit more responsive.
// Keep in mind that we collect many samples before sending them to the
// server.
#define SAMPLE_DURATION_MS 10
Sample getDataSample() { 
  Sample result;

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
  return HEADER_SIZE_BYTES + (groupNumber) * 5;
}

// This method will get the data from the sensor and add it to the out packet
void collectData() {

  Sample s = getDataSample();

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
  int group_number   = currentSample / 4;
  int s_group_pos    = currentSample % 4;
  int group_byte_loc = HEADER_SIZE_BYTES + ((group_number+1) * 5) - 1;   // -1 for index 0
  int s_byte_loc     = HEADER_SIZE_BYTES + (group_number*5 + s_group_pos); 

  opacket[s_byte_loc]     = d & 0xFF;
  opacket[group_byte_loc] = opacket[group_byte_loc] | ((d >> 8) << 2*s_group_pos);

  // Increment counters
  currentSample++;
  accumulationSamplesSent++;
}

// Sends whatever is in the opacket and resets counters
void sendPacket() {
  Udp.beginPacket(destip, serverUDPPort);
  Udp.write(opacket, getPacketSize());
  Udp.endPacket();
}

void loop() {
  if (!digitalRead(A_BUTTON)) {
    // Toggle the behavior when the A button is pressed back and forth between
    // our two modes.
    if (!wasPressed) {
      if (doSend) {
        doSend = false;
        printStandbyMessage(packetNumber);
      } else {
        accumulationStartTime = millis();
        accumulationSamplesSent = 0;
        doSend = true;
        printSendingMessage();
      }
      wasPressed = true;
    }
  } else {
    wasPressed = false;
  }

  if (doSend) {
    // Get ready for the next packet to be sent. 
    setupPacket();

    // We use a tight while loop here so that we accumulate and send an 
    // entire packet of data with semi-strict timing requirements.  
    while (currentSample < PACKETSAMPLESIZE) {
      collectData();
    } 
    sendPacket();
    packetNumber++;
  
    display.fillRect(53,16,70,8, SSD1306_BLACK);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(53,16);
    display.print(packetNumber);

    unsigned long int elapsedTime = millis() - accumulationStartTime;
    if (elapsedTime > 3000) { 
      // Every so often update the sps - Samples Per Second.
      display.fillRect(53,24,70,8, SSD1306_BLACK);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(53,24);
      display.print( (float) 
        ( (accumulationSamplesSent) / ( (float)elapsedTime / 1000) ) 
      );

      accumulationStartTime = millis();
      accumulationSamplesSent = 0;
    }
    display.display();
  }

  // See if there are any UDP packets for us to process.
  handleUDPPacket();
}
