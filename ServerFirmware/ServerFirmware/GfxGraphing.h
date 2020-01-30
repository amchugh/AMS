
#ifndef GFX_GRAPHING
#define GFX_GRAPHING

#define MAX_DATASETS 4
#define MOST_RECENT_VALUES_COUNT_MAX 10

class Graphing {
public:
  Graphing(Adafruit_GFX &display, int topLeftX, int topLeftY, int width, int height);

  void setBackgroundColor(uint16_t backgroundColor);
  void setBorderColor(uint16_t majorAxisColor);
  void setMinAndMaxYAxisValues(float minYAxisValue, float maxYAxisValue);
  void setDatasetColor(uint16_t color);
  void recomputeLayoutProperties();

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

private:
  Adafruit_GFX& display;
  uint16_t topLeftX, topLeftY;
  uint16_t width, height;
  uint8_t numberSegments;
  bool setupMode;

  uint16_t backgroundColor;
  uint16_t borderColor;
  uint16_t datasetColor;
  uint16_t segmentSize;

  float minYAxisValue, maxYAxisValue;
  float mapDataValueToYScale;

  uint16_t currentX;

  float mostRecentValues[MOST_RECENT_VALUES_COUNT_MAX];
  uint8_t mostRecentValuesCount;
  uint32_t mostRecentValueMillis;

  float currentValue;
  float currentValueCount;
  float priorValue;

  float priorMax;
};

#endif
