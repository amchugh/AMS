
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

// The maximum number of datapoints.
#define MAX_DATA_POINTS 100

// Class information for a station
struct Station {
  IPAddress address;
  uint16_t audioDataPointValues[MAX_DATA_POINTS];
  uint32_t audioDataPointTimes[MAX_DATA_POINTS];

  uint8_t indexOfNextDataPoint;
  uint8_t numberDataPoints;
  uint32_t lastNonzeroDataPointTime;
};

void initializeStation(Station &s, const IPAddress &a) {
  s.address = a;

  for (int i=0; i<MAX_DATA_POINTS; i++) {
    s.audioDataPointValues[i] = 0;
    s.audioDataPointTimes[i] = 0;
  }
  s.indexOfNextDataPoint = 0;
  s.numberDataPoints = 0;
  s.lastNonzeroDataPointTime = 0;
}

void initializeStation(Station &s) {
  initializeStation(s, INADDR_NONE);
}


// Find the station for a specified IPAddress and return the index
// or return -1 if the station is not found.
int8_t findStation(const Station* s, int numberOfStations, const IPAddress &a) {
  int result = -1;
  for (int i=0; i<numberOfStations; i++) {
    if (s[i].address == a) {
      return i;
    }
  }
  return -1;
}

// Find a slot for a new station and intialize all of the data members.
// This function will return -1 if no slot was available or -2 if the
// station already exists.
int8_t addStation(Station *s, int numberOfStations, IPAddress &a) {
  int8_t i = findStation(s, numberOfStations, a);
  if (i =! -1) {
    return -2;
  }


  int index = 0;
  for (; index<numberOfStations; index++) {
    if (s[index].address == INADDR_NONE) {
      break;
    }
  }


}


void addDataPoint(Station &s, uint16_t value, uint32_t time) {
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
