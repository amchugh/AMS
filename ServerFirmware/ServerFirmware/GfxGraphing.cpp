
#include <stdexcept>
#include <Adafruit_GFX.h>
#include "GfxGraphing.h"

#define AXIS_OFFSET_X 3
#define AXIS_OFFSET_Y 3

#define DATAPOINT_OFFSET_X AXIS_OFFSET_X + 2
#define DATAPOINT_OFFSET_Y AXIS_OFFSET_Y + 2

Graphing::Graphing(Adafruit_GFX &d, int topLeftX, int topLeftY, int
    width, int height) : display(d)
{
  this->topLeftX = topLeftX;
  this->topLeftY = topLeftY;
  this->width = width;
  this->height = height;

  setupDefaults();

  setupMode = true;
}

void Graphing::setupDefaults() {
  backgroundColor = 0x0;
  borderColor = 0xFFFF;
  minYAxisValue = 0;
  maxYAxisValue = 1.0;
  currentX = 0;
  numberSegments = 8;

  for (int i=0; i<MOST_RECENT_VALUES_COUNT_MAX; i++) {
    mostRecentValues[i] = 0;
  }
  mostRecentValuesCount = 0;
  mostRecentValueMillis = 0;

  currentValue = 0;
  currentValueCount = 0;
  priorValue = 0;
  priorMax = 0;

  recomputeLayoutProperties();
}

void Graphing::recomputeLayoutProperties() {
  mapDataValueToYScale = (float) height / (maxYAxisValue - minYAxisValue);

  segmentSize = height / numberSegments;

  Serial.printf("recomputeLayoutProperties - done; minYAxisValue: %f, maxYAxisValue: %f, height: %d, mapDataValueToYScale: %f\n",
      minYAxisValue, maxYAxisValue, height, mapDataValueToYScale);
  Serial.printf("recomputeLayoutProperties - done; numberSegments: %d, segmentSize: %d\n",
      numberSegments, segmentSize);
}

void Graphing::setBackgroundColor(uint16_t backgroundColor) {
  this->backgroundColor = backgroundColor;
}

void Graphing::setBorderColor(uint16_t borderColor) {
  this->borderColor = borderColor;
}

void Graphing::setMinAndMaxYAxisValues(float minYAxisValue,
    float maxYAxisValue) {
  this->minYAxisValue = minYAxisValue;
  this->maxYAxisValue = maxYAxisValue;
  recomputeLayoutProperties();
}

void Graphing::setDatasetColor(uint16_t datasetColor) {
  this->datasetColor = datasetColor;
}

void Graphing::startGraphing() {
  if (setupMode == false) {
    throw std::logic_error("Cannot call start graphing twice");
  }
  setupMode = false;
  drawFrame();
}

void Graphing::drawFrame() {
  display.fillRect(topLeftX, topLeftY, width, height, backgroundColor);
  display.drawRect(topLeftX, topLeftY, width, height, borderColor);

  for (int i=0; i<numberSegments; i++) {
    display.drawFastHLine(topLeftX, topLeftY+(segmentSize*i), width,
        borderColor);
  }
}

void Graphing::handleMostRecentValues(float value) {
  uint32_t currentMillis = millis();

  if (currentMillis - mostRecentValueMillis > 1000) {
    // A new second has passed.
    mostRecentValueMillis = currentMillis;
    mostRecentValuesCount++;

    if (mostRecentValuesCount == MOST_RECENT_VALUES_COUNT_MAX) {
      // Shuffle everything to the left
      for (int i=1; i<MOST_RECENT_VALUES_COUNT_MAX; i++) {
        mostRecentValues[i-1] = mostRecentValues[i];
      }
      mostRecentValuesCount = MOST_RECENT_VALUES_COUNT_MAX - 1;
    }
    mostRecentValues[mostRecentValuesCount] = value;
  } else {
    // Otherwise we look at the mostRecentValuesCount index in the
    // mostRecentValues array and see if this new value is larger.
    if (value > mostRecentValues[mostRecentValuesCount]) {
      mostRecentValues[mostRecentValuesCount] = value;
    }
  }
}

float Graphing::getMostRecentMaxValue() {
  float max = 0;
  for (int i=0; i<MOST_RECENT_VALUES_COUNT_MAX; i++) {
    if (mostRecentValues[i] > max) {
      max = mostRecentValues[i];
    }
  }
  return max;
}


void Graphing::addDatasetValue(float y) {
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

  currentValue += y;
  currentValueCount++;
}

void Graphing::update() {

  // CurrentValue has the sum of all of the values added to the dataset
  // since the last update call.  We average them to get a simple aggregate
  // value.
  if (currentValueCount > 0) {
    currentValue /= currentValueCount;
  }

  if (currentValue != priorValue) {
    // Update the bars that are drawn.
    drawBars(currentValue, priorValue);
  }

  float max = getMostRecentMaxValue();
  if (priorMax != max) {
    drawMax(max, priorMax);
  }

  priorValue = currentValue;
  currentValue = 0;
  currentValueCount = 0;

  priorMax = max;
}

uint8_t Graphing::mapValueToSegmentCount(float v) {
  float separator = maxYAxisValue / numberSegments;

  float raw = v / separator;
  uint8_t result = (raw + 0.5);

  if (v>0 && result==0) {
    result = 1;
  }
  return result;
}

void Graphing::drawBars(float current, float prior) {
  int currentSegmentIndex = mapValueToSegmentCount(current);
  int priorSegmentIndex = mapValueToSegmentCount(prior);

  Serial.printf("drawBars; current: %f, prior: %f, currentSegment: %d, priorSegment: %d\n",
      current, prior, currentSegmentIndex, priorSegmentIndex);

  if (currentSegmentIndex == priorSegmentIndex) {
    return;
  } else if (currentSegmentIndex < priorSegmentIndex) {
    // Need to erase some segments
    for (int i=priorSegmentIndex; i>=currentSegmentIndex; i--) {
      display.fillRect(topLeftX+1, topLeftY+1+segmentSize*i, width-2,
            segmentSize-2, backgroundColor);
    }
  } else {
    // Need to fill in some segments
    for (int i=priorSegmentIndex; i<currentSegmentIndex; i++) {
      display.fillRect(topLeftX+1, topLeftY+1+(segmentSize)*i, width-2,
            segmentSize-2, datasetColor);
    }
  }
}

void Graphing::drawMax(float current, float prior) {
  int currentSegmentIndex = mapValueToSegmentCount(current);
  int priorSegmentIndex = mapValueToSegmentCount(prior);

  Serial.printf("drawMax; current: %f, prior: %f, currentSegment: %d, priorSegment: %d\n",
      current, prior, currentSegmentIndex, priorSegmentIndex);

  // Erase the red line drawn on the border for the prior (replace it
  // with white) and then draw a new red line.
  display.drawFastHLine(topLeftX, topLeftY+(segmentSize*priorSegmentIndex), width,
      borderColor);
  display.drawFastHLine(topLeftX, topLeftY+(segmentSize*currentSegmentIndex), width,
      0xFF0);
}
