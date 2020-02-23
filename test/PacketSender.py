import socket
import struct

def sendData(stationID, packetNumber, valueData1, valueData2, valueData3, valueData4):
    UDP_IP = "192.168.4.1"
    UDP_PORT = 8888

    # PacketNumber must be modulo 255
    packetNumber %= 255

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

    print("UDP target IP: {}, port: {}, stationId: {}, PacketNumber: {}, Value1: {}".format(UDP_IP, UDP_PORT, stationID, packetNumber,
                valueData1))

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
    sock.sendto(data, (UDP_IP, UDP_PORT))
