import sys
import PacketSender

def main():
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

    PacketSender.sendData(stationID, packetNumber, valueData1, valueData2, valueData3, valueData4);

if __name__ == '__main__':
    main()
