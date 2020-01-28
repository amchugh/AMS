// PacketDecoder.h
//
// This code is responsible for taking in a packet from an AMS station and extracting
// the information into a usable form.

#ifndef PACKET_DECODER_H
#define PACKET_DECODER_H

// This requires Station.h because I use some of the constants.
#include "Station.h"

// Locations and total header size, in bytes, for various components of the
// packet.
#define PACKET_SENDER_LOC 0
#define PACKET_NUMBER_LOC 1
#define HEADER_SIZE (PACKET_NUMBER_LOC+1)

//
// Packet structure
//
// 0           7 8         15 16        23 24        31
// +------------+------------+------------+------------+
// | Sender     | Number     |      Data Block 1       |
// +------------+------------+------------+------------+
// |         Data Block 1                 |   Data     |
// +------------+------------+------------+------------+
// |                   Block 2                         |
// +------------+------------+------------+------------+
// |                    Data Block 3                   |
// +------------+------------+------------+------------+
//            ...
//
// A data block consists of 5 bytes each:
//
// 0           7 8         15 16        23 24        31          39
// +------------+------------+------------+------------+-----------+
// | LSBs Data1 | LSBs Data2 | LSBs Data3 | LSBs Data4 |D1|D2|D3|D4|
// +------------+------------+------------+------------+-----------+
//                                                       2 MSBs of
//                                                       each of the
//                                                       four prior
//                                                       data points

struct PacketData {
  PacketNumber packetNumber;
  StationIdentifier packetSenderID;
  uint16_t sampleCount;
  uint16_t samples[MAX_DATA_POINTS];
};

PacketNumber getPacketNumber(const byte * const packet) {
  byte result = packet[PACKET_NUMBER_LOC];
  return (PacketNumber) result;
}

StationIdentifier getPacketSenderID(const byte * const packet) {
  byte result = packet[PACKET_SENDER_LOC];
  return (StationIdentifier) result;
}

int getSampleLength(int packetLength) {
  return (packetLength - HEADER_SIZE) / 5 * 4;
}

/**
 * This method takes in a PacketData object to put the data into, along with a
 * byte array to process. This allows for tracking of "most recent packet" and
 * doesn't inflate memory. Packet Length should be the length of the message as
 * the expected packet size cannot be assumed constant.
 */
void processPacket(PacketData &p, const byte * const packet, int packetLength) {
  // Get some of the misc. information
  p.sampleCount = getSampleLength(packetLength);
  p.packetNumber = getPacketNumber(packet);
  p.packetSenderID = getPacketSenderID(packet);

  // Extract the data from the packet
  // Need to be done in groups of 4
  for (int offset = 0; offset < p.sampleCount/4; offset++) {
    p.samples[offset * 4 + 0] = packet[HEADER_SIZE + offset * 5 + 0];
    p.samples[offset * 4 + 1] = packet[HEADER_SIZE + offset * 5 + 1];
    p.samples[offset * 4 + 2] = packet[HEADER_SIZE + offset * 5 + 2];
    p.samples[offset * 4 + 3] = packet[HEADER_SIZE + offset * 5 + 3];

    p.samples[offset * 4 + 0] +=
      ((uint16_t)(packet[HEADER_SIZE + (offset * 5) - 1] & 0b11000000) >> 6) << 8;
    p.samples[offset * 4 + 1] +=
      ((uint16_t)(packet[HEADER_SIZE + (offset * 5) - 1] & 0b00110000) >> 4) << 8;
    p.samples[offset * 4 + 2] +=
      ((uint16_t)(packet[HEADER_SIZE + (offset * 5) - 1] & 0b00001100) >> 2) << 8;
    p.samples[offset * 4 + 3] +=
      ((uint16_t)(packet[HEADER_SIZE + (offset * 5) - 1] & 0b00000011) >> 0) << 8;
  }
}

#endif
