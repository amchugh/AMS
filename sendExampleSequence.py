import PacketSender
import np
import sys
import time

def sendDataForSeconds(stationID, packetNumber, seconds, average, std):
    numberDataSamples = 33*seconds
    samples = np.random.normal(average, std, numberDataSamples)
    for i in range(numberDataSamples):
        print("Sending: {}".format(samples[i]))
        d = int(samples[i])

        PacketSender.sendData(stationID, packetNumber + i, d, d, d, d)
        time.sleep(0.030)

    return packetNumber + i

def main():
    if len(sys.argv) < 3:
        print("Script required args: <stationId> <StartingpacketNumber>")
        sys.exit(-1);

    stationID = int(sys.argv[1])
    packetNumber = int(sys.argv[2])

    numberDataSamples = 33*25
    samples = np.random.normal(512, 120, numberDataSamples)

    packetNumber = sendDataForSeconds(stationID, packetNumber, 3, 512,80)
    packetNumber = sendDataForSeconds(stationID, packetNumber, 3, 200,120)
    packetNumber = sendDataForSeconds(stationID, packetNumber, 3, 800,60)
    packetNumber = sendDataForSeconds(stationID, packetNumber, 2, 300,60)
    packetNumber = sendDataForSeconds(stationID, packetNumber, 1, 100,40)

if __name__ == '__main__':
    main()
