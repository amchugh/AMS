import socket
import struct
import sys
import ctypes

if len(sys.argv) < 4:
  print("Script requires three parameters:  <stationId>  <packetNumber> <SingleDataValue>")
  sys.exit(-1);


stationID = int(sys.argv[1])
packetNumber = int(sys.argv[2])
value = int(sys.argv[3])

UDP_IP = "192.168.4.1"
UDP_PORT = 8888
data = struct.pack('BBH', stationID, packetNumber, value)

print("UDP target IP: {}".format(UDP_IP))
print("UDP target port: {}".format(UDP_PORT))
print("Data: {} (stationId: {}, PacketNumber: {}, Value: {})".format(data,
    stationID, packetNumber, value))

sock = socket.socket(socket.AF_INET, # Internet
             socket.SOCK_DGRAM) # UDP
sock.sendto(data, (UDP_IP, UDP_PORT))
