
#ifndef GFX_GRAPHING
#define GFX_GRAPHING

// The number of recent data points to track in order to show the
// maximum value for the past.  The value here is tracked as 'seconds'
// with one maximum datapoint value for each second.
#define MOST_RECENT_VALUES_COUNT_MAX 10

/**
  * A class to handle the display of a dataset by visualizing it as
  * lite segments in a bar graph.  The utility here mimics what is seen
  * on a sound display board where the segments descritize the values
  * into a small number of easily read 'leds'.  There is also a high
  * water mark that indicates the maximum value seen in the recent past.
  */
class SegmentedBarGraph {
public:
  SegmentedBarGraph(Adafruit_GFX &display, int topLeftX, int topLeftY, int width, int height);

  void setBackgroundColor(uint16_t backgroundColor);
  void setBorderColor(uint16_t majorAxisColor);
  void setMinAndMaxYAxisValues(float minYAxisValue, float maxYAxisValue);
  void recomputeLayoutProperties();

  void setSegmentGroupColors(uint16_t colorGroupOne, uint16_t colorGroupTwo,
      uint16_t colorGroupThree);

  void startGraphing();
  void addDatasetValue(float y);
  void update();

protected:
  void drawFrame();
  void setupDefaults();
  void drawBars(float current, float prior);
  void drawMax(float current, float prior);
  uint8_t mapValueToSegmentCount(float v);
  float getMostRecentMaxValue();
  void handleMostRecentValues(float value);
  uint16_t getSegmentColor(uint8_t segment);
  uint16_t getSegmentTopLeftY(uint8_t segment);

private:
  Adafruit_GFX& display;
  uint16_t topLeftX, topLeftY;
  uint16_t width, height;
  bool setupMode;

  uint8_t numberSegments;
  uint16_t segmentHeight;

  uint16_t segmentGroupOneColor;
  uint16_t segmentGroupTwoColor;
  uint16_t segmentGroupThreeColor;

  uint16_t backgroundColor;
  uint16_t borderColor;

  float minYAxisValue, maxYAxisValue;

  float mostRecentValues[MOST_RECENT_VALUES_COUNT_MAX];
  int mostRecentValuesIndex;
  uint32_t mostRecentValueMillis;

  float currentValue;
  float currentValueCount;

  float priorValue;
  float priorMax;
};

#endif
