# AMS - Audio Monitoring System

This system utilizes multiple hardware nodes to listen for sound levels and display an aggregate view on a 
'server' instance.  

Nodes are comprised of:
 * Adafruit Feather Huzzah with ESP8266 - https://www.adafruit.com/product/2821
 * Featherwing OLED displays - https://www.adafruit.com/product/2900
 
Server instance is:
 * Adafruit Feather Huzzah with ESP8266 - https://www.adafruit.com/product/2821
 * Featherwing touch screen display - https://www.adafruit.com/product/3315
 

The goal for transmission from each station is: 
- 25 packets per second 
- <=5% drop rate at 30ft through a wall
- 50 new data points per packet
