
# This is a helper program to create a behavior which we need to test for
# and then allow you, the engineer, to manually check the server station
# and logs for messages that indicate that a duplicate packet was 
# detected. 
#
# TODO:  Add an http server to the ServerFirmware that allows for introspection
#   and fully automate this test.
#

import PacketSender

def main():
    stationID = 1
    packetNumber = 1
    d = 1

    PacketSender.sendData(stationID, packetNumber, d, d, d, d)

    # Sending the same packet will cause an invalid
    PacketSender.sendData(stationID, packetNumber, d, d, d, d)

    # Send three valid packets
    PacketSender.sendData(stationID, packetNumber+1, d, d, d, d)
    PacketSender.sendData(stationID, packetNumber+2, d, d, d, d)
    PacketSender.sendData(stationID, packetNumber+3, d, d, d, d)
    # Now an older one.
    PacketSender.sendData(stationID, packetNumber, d, d, d, d)

    # Finally send a bunch of packets then an old one which will slip through.
    packetNumber = 100
    for i in range(6):
        PacketSender.sendData(stationID, packetNumber+1, d, d, d, d)

    # Should be considered valid.
    PacketSender.sendData(stationID, packetNumber+1, d, d, d, d)

if __name__ == '__main__':
    main()
