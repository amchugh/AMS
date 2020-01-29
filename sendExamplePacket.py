import socket
import struct
import sys
import ctypes

if len(sys.argv) < 4:
    print("Script required args: <stationId> <packetNumber> <DataValue1>")
    sys.exit(-1);


stationID = int(sys.argv[1])
packetNumber = int(sys.argv[2])
valueData1 = int(sys.argv[3])
valueData2 = 0
valueData3 = 0
valueData4 = 0

if len(sys.argv) >= 5:
    valueData2 = int(sys.argv[4])
if len(sys.argv) >= 6:
    valueData3 = int(sys.argv[5])
if len(sys.argv) >= 7:
    valueData4 = int(sys.argv[6])

UDP_IP = "192.168.4.1"
UDP_PORT = 8888

# See the C++ code for encoding structures.
dataByte1 = valueData1 & 0xFF
dataByte2 = valueData2 & 0xFF
dataByte3 = valueData3 & 0xFF
dataByte4 = valueData4 & 0xFF
dataByte5  = ((int(valueData1 / 256 ) ) & 0x3 ) << 6
dataByte5 |= ((int(valueData2 / 256 ) ) & 0x3 ) << 4
dataByte5 |= ((int(valueData3 / 256 ) ) & 0x3 ) << 2
dataByte5 |= ((int(valueData4 / 256 ) ) & 0x3 )

data = struct.pack('BBBBBBB', stationID, packetNumber, dataByte1, dataByte2,
        dataByte3, dataByte4, dataByte5 )

print("UDP target IP: {}".format(UDP_IP))
print("UDP target port: {}".format(UDP_PORT))
print("Data: {} (stationId: {}, PacketNumber: {}, Value: {})".format(data,
    stationID, packetNumber, valueData1))

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
sock.sendto(data, (UDP_IP, UDP_PORT))
