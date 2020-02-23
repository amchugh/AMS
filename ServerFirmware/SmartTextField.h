
// A SmartTextField is a region of an Adafruit_GFX display that renders
// a single value in a specific location where the value may or may not
// change between renders.  Only when the value changes is anything
// erased and drawn.

#ifndef SMART_TEXT_FIELD_H
#define SMART_TEXT_FIELD_H

template <class T>
class SmartTextField {
public:
  SmartTextField(Adafruit_GFX &_display, int _topLeftX, int _topLeftY,
      int _width, int _height, uint16_t _backgroundColor) : display(_display),
      topLeftX(_topLeftX), topLeftY(_topLeftY), width(_width), height(_height),
      backgroundColor(_backgroundColor) {
    firstValue = true;
  }

  /*
   * render the value within the slot that has been defined for this element.
   * If the value has not changed then this won't do any work.
   */
  void render(T value) {
    if (firstValue == true || priorValue != value) {

      // Clear the space for this element
      display.fillRect(topLeftX, topLeftY, width, height, backgroundColor);

      display.setCursor(topLeftX, topLeftY);
      display.print(value);

      priorValue = value;
      firstValue = false;
    }
  }

private:
  Adafruit_GFX& display;
  uint16_t topLeftX, topLeftY;
  uint16_t width, height;

  uint16_t backgroundColor;

  bool firstValue;
  T priorValue;
};

#endif
