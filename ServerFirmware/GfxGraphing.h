
#ifndef GFX_GRAPHING
#define GFX_GRAPHING

// The number of recent data points to track in order to show the
// maximum value for the past.  The value here is tracked as 'seconds'
// with one maximum datapoint value for each second.
#define MOST_RECENT_VALUES_COUNT_MAX 4

/**
  * SegmentedBarGraph is a class that handles the display of a dataset by
  * visualizing it as lite segments in a bar graph.
  *
  * The UI here mimics what is seen on a sound display board where the segments
  * discretize the values into a small number of easily read 'leds'.  There is
  * also a high water mark that indicates the maximum value seen in the recent
  * past.
  *
  * Example Initialization:
  *
  *   Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
  *   g = new SegmentedBarGraph(tft, 260, 40, 24, 176 );
  *   g->setBackgroundColor(ILI9341_BLACK);
  *   g->setBorderColor(ILI9341_WHITE);
  *   g->setMinAndMaxYAxisValues(0, 1024);
  *   g->setSegmentGroupColors(ILI9341_GREEN, ILI9341_YELLOW, ILI9341_RED);
  *   g->startGraphing();
  *
  * Then, you call addDatasetValue some number of times and finally
  * a render call when you want the max/average value of what has been fed
  * to show on the screen.  Each render call results in the display being
  * updated.
  *
  * Example:
  *    for (int i=0; i < sampleCount; i++) {
  *     g->addDatasetValue(samples[i]);
  *    }
  *    g->render();
  *
  */
class SegmentedBarGraph {
public:
  SegmentedBarGraph(Adafruit_GFX &display, int topLeftX, int topLeftY, int width, int height);

  /* The background color for the entire area encompassed by the graph. */
  void setBackgroundColor(uint16_t backgroundColor);

  /* The border color used to distinguish the segments. */
  void setBorderColor(uint16_t majorAxisColor);

  /* The range of values expected in the data points.  */
  void setMinAndMaxYAxisValues(float minYAxisValue, float maxYAxisValue);

  /* The amount of time that a render will maintain the current view
     even though no data values have been processed. */
  void setMaintainPriorViewMills(uint32_t maintainPriorViewMillis);

  /* The group colors used by bar graph.  The bottom most 'leds' will be
   * shown as colorGroupOne.  These are considered "Ok" values and usually
   * should be set 'green' or some other affirmative color.  The next set
   * of 'leds' are in colorGroupTwo and indicate a near warning.  The
   * final set of topmost leds will be drawn as colorGroupThree and indicate
   * warning states.
   */
  void setSegmentGroupColors(uint16_t colorGroupOne, uint16_t colorGroupTwo,
      uint16_t colorGroupThree);

  /**
    * call startGraphing when you are done with the configuration and
    * want the initial drawing of the frame to occur.  This must be
    * called prior to adding data and calling render.
    */
  void startGraphing();

  /**
    * call addDatasetValue as many times as you want between render calls
    * but know that the values passed in will be averaged and then used
    * to render the display when render is called.
    */
  void addDatasetValue(float y);

  /**
    * render will draw the average of all values passed in between this
    * call and the prior render.  If no data values are provided in the last
    * timeoutMillis then it assumed that the 0 is the latest value and render
    * the graph based on that.
    */
  void render();

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

  uint32_t timeViewDisplayedMillis;
  uint32_t maintainPriorViewMillis;

  // In between renders we receive lots of transient values (or samples).
  // These are stored here.
  float currentSampleSum;
  float currentSampleMax;
  float currentSampleCount;

  // The data values shown to the user.
  float currentValue;
  float priorValue;
  float priorMax;
};

#endif
