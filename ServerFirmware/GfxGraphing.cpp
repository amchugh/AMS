
#include <stdexcept>
#include <Adafruit_GFX.h>
#include "GfxGraphing.h"

#define AXIS_OFFSET_X 3
#define AXIS_OFFSET_Y 3

#define DATAPOINT_OFFSET_X AXIS_OFFSET_X + 2
#define DATAPOINT_OFFSET_Y AXIS_OFFSET_Y + 2

SegmentedBarGraph::SegmentedBarGraph(Adafruit_GFX &d, int topLeftX, int topLeftY, int
    width, int height) : display(d)
{
  this->topLeftX = topLeftX;
  this->topLeftY = topLeftY;
  this->width = width;
  this->height = height;

  /* Some default values set for the graph. */
  backgroundColor = 0x0;
  borderColor = 0xFFFF;
  minYAxisValue = 0;
  maxYAxisValue = 1.0;
  numberSegments = 8;
  segmentHeight = height / numberSegments;

  /* Initialize state values used by the graph. */
  for (int i=0; i<MOST_RECENT_VALUES_COUNT_MAX; i++) {
    mostRecentValues[i] = 0;
  }
  mostRecentValuesIndex = -1;
  mostRecentValueMillis = 0;

  currentValue = 0;
  currentSampleSum = 0;
  currentSampleMax = 0;
  currentSampleCount = 0;
  priorValue = 0;
  priorMax = 0;

  setupMode = true;
}

void SegmentedBarGraph::setBackgroundColor(uint16_t backgroundColor) {
  this->backgroundColor = backgroundColor;
}

void SegmentedBarGraph::setBorderColor(uint16_t borderColor) {
  this->borderColor = borderColor;
}

void SegmentedBarGraph::setSegmentGroupColors(uint16_t colorGroupOne,
    uint16_t colorGroupTwo, uint16_t colorGroupThree) {
  this->segmentGroupOneColor = colorGroupOne;
  this->segmentGroupTwoColor = colorGroupTwo;
  this->segmentGroupThreeColor = colorGroupThree;
}

void SegmentedBarGraph::setMinAndMaxYAxisValues(float minYAxisValue,
    float maxYAxisValue) {
  this->minYAxisValue = minYAxisValue;
  this->maxYAxisValue = maxYAxisValue;
}

void SegmentedBarGraph::startGraphing() {
  if (setupMode == false) {
    throw std::logic_error("Cannot call start graphing twice");
  }
  setupMode = false;
  drawFrame();
}

void SegmentedBarGraph::drawFrame() {
  display.fillRect(topLeftX, topLeftY, width, height, backgroundColor);
  display.drawRect(topLeftX, topLeftY, width, height, borderColor);

  for (int i=0; i<numberSegments; i++) {
    uint16_t y = getSegmentTopLeftY(i);
    display.drawFastHLine(topLeftX, y, width, borderColor);
  }
}

void SegmentedBarGraph::handleMostRecentValues(float value) {
  uint32_t currentMillis = millis();

  if (currentMillis - mostRecentValueMillis > 1000) {
    // A new second has passed.
    mostRecentValueMillis = currentMillis;
    mostRecentValuesIndex++;

    if (mostRecentValuesIndex == MOST_RECENT_VALUES_COUNT_MAX) {
      // Shuffle everything to the left
      for (int i=1; i<MOST_RECENT_VALUES_COUNT_MAX; i++) {
        mostRecentValues[i-1] = mostRecentValues[i];
      }
      mostRecentValuesIndex = MOST_RECENT_VALUES_COUNT_MAX - 1;
    }
    mostRecentValues[mostRecentValuesIndex] = value;
  } else {
    // Otherwise we look at the mostRecentValuesIndex index in the
    // mostRecentValues array and see if this new value is larger.
    if (value > mostRecentValues[mostRecentValuesIndex]) {
#ifdef DEBUG_PRINT
      Serial.printf(
          "handleMostRecentValues - duplicate second but larger value; "
          "old Max: %f, newMax: %f\n",
          mostRecentValues[mostRecentValuesIndex], value);
#endif
      mostRecentValues[mostRecentValuesIndex] = value;
    }
  }
}

float SegmentedBarGraph::getMostRecentMaxValue() {
  float max = 0;
  for (int i=0; i<MOST_RECENT_VALUES_COUNT_MAX; i++) {
    if (mostRecentValues[i] > max) {
      max = mostRecentValues[i];
    }
  }
  return max;
}


void SegmentedBarGraph::addDatasetValue(float y) {
  if (setupMode) {
    throw std::logic_error("Must be fully setup before usage.");
  }
  if (y>maxYAxisValue) {
    throw std::invalid_argument("y value provided is too large");
  }
  if (y<minYAxisValue) {
    throw std::invalid_argument("y value provided is too small");
  }
  handleMostRecentValues(y);

  currentSampleSum += y;
  if (y > currentSampleMax) {
    currentSampleMax = y;
  }
  currentSampleCount++;
}

void SegmentedBarGraph::render() {

  // currentSampleSum has the sum of all of the values added to the dataset
  // since the last render call.  We average them to get a simple aggregate
  // value.
  if (currentSampleCount == 0) {
    // Assume that 0 is the correct default value to be used if no data has
    // been given.
    addDatasetValue(0);
  }

#ifdef USE_AVERAGE
  // Determine the average value
  currentValue = currentSampleSum / currentSampleCount;
#else
  currentValue = currentSampleMax;
#endif

  if (currentValue != priorValue) {
    // Update the bars that are drawn.
    drawBars(currentValue, priorValue);
  }

  float max = getMostRecentMaxValue();
  if (priorMax != max) {
    drawMax(max, priorMax);
  }

  priorValue = currentValue;
  currentSampleSum = 0;
  currentSampleCount = 0;
  currentSampleMax = 0;

  priorMax = max;
}

/**
  * This returns the number of segments (and not the segment index which is
  * based zero!) that should be lite up given a value v within the range of
  * minYAxisValue and maxYAxisValue.  Given a value 'x' returned from this
  * function you would light up [0..x).  (Not inclusive of x.)
  *
  */
uint8_t SegmentedBarGraph::mapValueToSegmentCount(float v) {
  float separator = (maxYAxisValue - minYAxisValue) / numberSegments;

  float raw = (v - minYAxisValue) / separator;
  uint8_t result = (raw + 0.5);

  if (v>0 && result==0) {
    result = 1;
  }
  return result;
}

uint16_t SegmentedBarGraph::getSegmentColor(uint8_t segment) {
  // The bottom half will all be the first color and then the top half is
  // divided into two evenly between the new two color groups.
  if (segment < numberSegments / 2) {
    return segmentGroupOneColor;
  } else if (segment < numberSegments * 3 / 4 ) {
    return segmentGroupTwoColor;
  } else {
    return segmentGroupThreeColor;
  }
}

/**
  * Returns the top left Y coordinate for a segment.
  * The segments are arranged from top to bottom in decreasing numbers.
  * The segments are base zero so the 0th segment is always at the bottom.
  */
uint16_t SegmentedBarGraph::getSegmentTopLeftY(uint8_t segment) {
  // This ASCII art shows the pixel locations for the segments.
  //
  //
  //
  //                         topLeftY
  //                               +----------+
  //                               |          |
  //                               |          |  Segment: numberSegments - 1
  //                               |          |
  //      topLeftY + segmentHeight +----------+
  //                               |          |
  //                                   ...
  //                               |          |
  // (topLeftY + (n*segmentHeight) +----------+
  //      - segmentHeight)         |          |
  //                               |          |  Segment: n
  //                               |          |
  // topLeftY + (n*segmentHeight)  +----------+
  //                               |          |
  //                                   ...

  // Two things to remember:
  //   1/  The 0th segment is the bottom most one so we find the bottom point
  //   and work backwards from that.
  //   2/  We subtract an additional segmentHeight because we want the top
  //   y point of the segment.
  return topLeftY + height - (segmentHeight * segment) - segmentHeight;
}

void SegmentedBarGraph::drawBars(float current, float prior) {
  int currentSegmentIndex = mapValueToSegmentCount(current);
  int priorSegmentIndex = mapValueToSegmentCount(prior);

#ifdef DEBUG_PRINT
  Serial.printf("drawBars; current: %f, prior: %f, currentSegment: %d, priorSegment: %d\n",
      current, prior, currentSegmentIndex, priorSegmentIndex);
#endif

  if (currentSegmentIndex == priorSegmentIndex) {
    return;
  } else if (currentSegmentIndex < priorSegmentIndex) {
    // Need to erase some segments
    for (int i=priorSegmentIndex; i>=currentSegmentIndex; i--) {
      display.fillRect(topLeftX+1, getSegmentTopLeftY(i)+1,
          width-2, segmentHeight-2, backgroundColor);
    }
  } else {
    // Need to fill in some segments
    for (int i=priorSegmentIndex; i<currentSegmentIndex; i++) {
      display.fillRect(topLeftX+1, getSegmentTopLeftY(i)+1,
          width-2, segmentHeight-2, getSegmentColor(i));
    }
  }
}

void SegmentedBarGraph::drawMax(float current, float prior) {
  // mapValueToSegmentCount returns a count of the number of
  // segments are filled in.  The zeroth segment index would
  // have a count value of 0.  We want to draw the topmost part
  // as a high water mark so we subtract one from the count in
  // order to know which segment line we want to draw.
  int currentSegmentIndex = mapValueToSegmentCount(current) - 1;
  int priorSegmentIndex = mapValueToSegmentCount(prior) - 1;

#ifdef DEBUG_PRINT
  Serial.printf("drawMax; current: %f, prior: %f, currentSegment: %d, priorSegment: %d\n",
      current, prior, currentSegmentIndex, priorSegmentIndex);
#endif

  // Erase the prior line drawn and then draw a new marker line.
  if (priorSegmentIndex >= 0) {
    display.drawFastHLine(topLeftX-3, getSegmentTopLeftY(priorSegmentIndex), width+6,
        backgroundColor);
    display.drawFastHLine(topLeftX, getSegmentTopLeftY(priorSegmentIndex), width,
        borderColor);
  }
  if (currentSegmentIndex >= 0) {
    display.drawFastHLine(topLeftX-3, getSegmentTopLeftY(currentSegmentIndex),
        width+6, segmentGroupThreeColor);
  }
}
