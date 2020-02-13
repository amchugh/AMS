# AMS - Audio Monitoring System

This system utilizes multiple hardware nodes to listen for sound levels and display an aggregate view on a 
'server' instance.  

Nodes are comprised of:
 * Adafruit Feather Huzzah with ESP8266 - https://www.adafruit.com/product/2821
 * Featherwing OLED displays - https://www.adafruit.com/product/2900
 * Electret microphones - https://www.adafruit.com/product/1063

 
Server instance is:
 * Adafruit Feather Huzzah with ESP8266 - https://www.adafruit.com/product/2821
 * Featherwing touch screen display - https://www.adafruit.com/product/3315
 

The goal for transmission from each station is: 
- 25 packets per second 
- <=5% drop rate at 30ft through a wall
- 50 new data points per packet


## Diagnosing a server instance

It is possible to run the server instance in a 'data gathering' mode.  Compile
and flash the ServerFirmware with DEBUG_PRINT_SHOW_DATA_DETAILS defined at the 
top of the file.  Use the arduino software's serial monitor (or any other
program to see the serial output) making sure that the baud rate matches the
configuration in the server firmware (currently 57600 baud).  

Connect a client or run:

```
python sendExampleSequence.py 1 1 
```

from the commnad line.  The output lines look like this:

```
...
handleUDPPacket; currentTime: 409759, IP: 192.168.4.2, ID: 1, stationIndex: 0, packetNumber: 132, dataIndex: 3, data: 47
handleUDPPacket; currentTime: 409844, IP: 192.168.4.2, ID: 1, stationIndex: 0, packetNumber: 134, dataIndex: 0, data: 93
...
```

The IP, ID and stationIndex are all used to distinguish one listening station
from another.  IP will likely be unique and sufficient.  A single packet can
contain many data values and dataIndex is the index of the data value (the
last component in the log line) within the packet.
