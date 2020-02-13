
//
// Station.h
//

// This class contains structures and inline definitions for a 'station'.
// A station is an external node that is sending data to the server instance.
// These data points need to be displayed in a somewhat synchronized way
// across all of the instances so the audio values are stored along with
// an approximate viewpoint on when they occurred.  We use approximate since
// the exactness doesn't matter - just that similar time descritized units
// are shared across all nodes.


#ifndef STATION_H
#define STATION_H

// The amount of time which can separate two datapoints where the two
// time periods are considered the same.  So, if time t1 and t2 are separated
// by at most TIME_DISCRETIZE_UNIT_MS then they will be considered the
// 'same' time for the purposes of accounting.
#define TIME_DISCRETIZE_UNIT_MS 100

// The maximum number of datapoints that will be tracked per station and
// maintained in memory.
#define MAX_DATA_POINTS 100
#define NO_STATION_ALLOCATED 255
#define INVALID_PACKET_NUMBER_RANGE 5
#define MAX_PACKET_NUMBER 255

typedef uint8_t PacketNumber;
typedef uint8_t StationIdentifier;

// Class information for a station
struct Station {
  StationIdentifier id;
  uint16_t audioDataPointValues[MAX_DATA_POINTS];
  uint32_t audioDataPointTimes[MAX_DATA_POINTS];

  uint8_t indexOfNextDataPoint;
  uint8_t numberDataPoints;
  uint32_t lastNonzeroDataPointTime;
  uint32_t invalidPacketCount;
  PacketNumber lastPacketNumber;
};

void initializeStation(Station &s, const StationIdentifier id) {
  s.id = id;

  for (int i=0; i<MAX_DATA_POINTS; i++) {
    s.audioDataPointValues[i] = 0;
    s.audioDataPointTimes[i] = 0;
  }
  s.indexOfNextDataPoint = 0;
  s.numberDataPoints = 0;
  s.lastNonzeroDataPointTime = 0;
  s.invalidPacketCount = 0;
  s.lastPacketNumber = 0;
}

void initializeStation(Station &s) {
  initializeStation(s, NO_STATION_ALLOCATED);
}


// Find the station for a specified identifier and return the index
// or return -1 if the station is not found.
int8_t findStation(const Station* s, int numberOfStations, const StationIdentifier id) {
  int result = -1;
  for (int i=0; i<numberOfStations; i++) {
    if (s[i].id == id) {
      return i;
    }
  }
  return -1;
}

// Find a slot for a new station and intialize all of the data members.
// This function will return -1 if no slot was available or -2 if the
// station already exists.
int8_t addStation(Station *s, int numberOfStations, const StationIdentifier id) {
  int8_t i = findStation(s, numberOfStations, id);
  if (i =! -1) {
    return -2;
  }

  int index = 0;
  for (; index<numberOfStations; index++) {
    if (s[index].id == NO_STATION_ALLOCATED) {
      break;
    }
  }
  if (index == numberOfStations) {
    // Consider doing a victims selection at some point in the future.
    // That is, if a node hasn't been active in awhile then kick it
    // out and allow this new one to replace it.
    return -1;
  }
  initializeStation(s[index], id);
  return index;
}

// Current implementation of isPacketValid will look at the five previous
// packet number values for Station. If the new packet number is equal to
// the current, or if it is contained in the old range, it will be declared
// invalid
bool isPacketValid(Station &s, const PacketNumber p) {
  // Starting from the last packet number seen and then working backwards
  // we look to see if there is a match between the passed in packet numbers
  // and any of those.
  int16_t c = s.lastPacketNumber;
  for (uint8_t i = 0; i <= INVALID_PACKET_NUMBER_RANGE; i++) {
    if (c == p) {
      Serial.printf(
        "isPacketValid - returning false; i: %d, lastPacketNumber: %d, "
        "receivedPacketNumber: %d\n", i, s.lastPacketNumber, p);
      s.invalidPacketCount++;
      return false;
    }
    c--;
    if (c < 0) c = MAX_PACKET_NUMBER;
  }

  s.lastPacketNumber = p;
  // If it passed all the tests, its good to go
  return true;
}

void addDataPoint(Station &s, uint16_t value, PacketNumber p, uint32_t time) {
  if (s.indexOfNextDataPoint == MAX_DATA_POINTS) {
    s.indexOfNextDataPoint = 0;
  }
  s.audioDataPointValues[s.indexOfNextDataPoint] = value;
  s.audioDataPointTimes[s.indexOfNextDataPoint] = time;
  if (s.numberDataPoints <= MAX_DATA_POINTS) {
    s.numberDataPoints++;
  }
  s.indexOfNextDataPoint++;

  if (value!=0 && time > s.lastNonzeroDataPointTime) {
    s.lastNonzeroDataPointTime = time;
  }
}


/**
  * Determines whether a station is managing any data values that are
  * close in time to the passed in time value.  This is used to
  * determine whether a station is updating their data in a roughly
  * similar way (from a time perspective) to other stations.
  *
  * A caller could determine that a lack of recent values means that the
  * station has fallen silent and can then synthesize fake values or
  * even remove the station from consideration.
  *
  */
bool doesStationHaveRecentValues(Station &s, uint32_t time) {
  // If the number of data points is zero then clearly it doesn't have a
  // recent value.
  if (s.numberDataPoints == 0 ) {
    return false;
  }

  // Otherwise we can look at the most recently added value.
  int index = s.indexOfNextDataPoint - 1;
  if (index < 0) {
    index = MAX_DATA_POINTS - 1;
  }

  uint32_t timeDifference = abs(s.audioDataPointTimes[index] - time);
  if (timeDifference > TIME_DISCRETIZE_UNIT_MS) {
    return false;
  } else {
    return true;
  }
}

#endif
