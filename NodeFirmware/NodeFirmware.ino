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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidtarget, NULL);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
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

void printConnected() {
  // Print a little message saying that we are connected
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Connected!");
  display.println( WiFi.localIP());
  display.println("Press A to begin");
  display.display();
}

// This one needs to print the number of packets sent for debuging
void printSendingMessage(int numSent) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Sending Data");
  display.print("Packets Sent: ");
  display.print(numSent);
  display.display();
}

void printStandbyMessage() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Standing By");
  display.println("Press A to begin");
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

// How many pieces of sensor information should be in each packet?
const uint16_t PACKETSAMPLESIZE = 128;

// What is the current index of packet information
uint16_t currentSample = 0;

// This method will get the data from the sensor and add it to the out packet
void collectData() {
  // For now, we will just store the audio information across two bytes.
  // We are also sending the raw data. I think it is unlikely that this is the
  //  data we want to send. We will probably do some of the audio processing
  //  on the stations instead of doing it all on the server
  // It is not efficient. It will work for now. I just want a Proof of Concept
  //todo:: improve
  uint16_t d = analogRead(A0);

  // sending raw data, little endian
  opacket[currentSample*2]   = d & 0xFF;
  opacket[currentSample*2+1] = d >> 8;

  // Increment counter
  currentSample++;
}

// Sends whatever is in the opacket and resets counters
void sendPacket() {
  // Send the packet
  Udp.beginPacket(destip, serverUDPPort);
  Udp.write(opacket);
  Udp.endPacket();

  // Print to serial just so we know to expect a packet
  Serial.println("Sent packet");

  // Reset counters
  currentSample = 0;
}

// ------------------------------

bool doSend = false;
bool wasPressed = false;
int ns = 0;

void loop() {
  // We will use the A button to toggle sending
  if (!digitalRead(A_BUTTON)) {
    if (!wasPressed) {
      if (!doSend) {
        doSend = true;
        printSendingMessage(ns);
      } else {
        doSend = false;
        // We will also clear the counters
        currentSample = 0;
        ns = 0;
        // And display
        printStandbyMessage();
      }
      wasPressed = true;
    }
  } else {
    wasPressed = false;
  }

  if (doSend) {
    // We need to collect the data we will send
    if (currentSample < PACKETSAMPLESIZE) {
      collectData();
      delay(1);
    } else {
      // Send our data!
      sendPacket();
      
      // Do some debug stuff
      //todo:: remove
      ns++;
      printSendingMessage(ns);
    }
  }

  // We handle UDP packets even if we aren't sending data
  handleUDPPacket();
}
