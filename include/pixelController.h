#ifndef PIXEL_CONTROLLER_H
#define PIXEL_CONTROLLER_H

#include <Adafruit_NeoPixel.h>


enum State {
  IDLE, TEST_ONE, TEST_TWO, FIREBALL, NECROBALL, SOLID_WRGB, COMMAND_SOLID_WRGB,
  BEACON, BLUETOOTH_CONNECT, BLUETOOTH_DISCONNECT
};
enum EaseType {
  EASE_LINEAR = 0, EASE_IN_QUADRATIC = 1, EASE_OUT_QUADRATIC = 2, EASE_OUT_CUBIC = 3,
  EASE_SMOOTH_STEP = 4, EASE_ROOT = 5, EASE_EXPONENTIAL = 6
};

class PixelController {
  private:
    Adafruit_NeoPixel& pixels;
    State currentState;
    unsigned long prevMilli;
    unsigned long deltaMilli;
    int frame;
    float rainbowOffset;
    State returnState;
    bool crackleInitialized;

    struct CrackleData {
      uint8_t heat;
      uint8_t red;
      uint8_t green;
      uint8_t blue;
      uint8_t white;
      bool heatSaturation;
      EaseType easeType;
    };
    CrackleData pixelCrackle[16];


    void tickFrame();
    void stateIdle();
    void stateTestOne();
    void stateTestTwo();
    void stateFireball();
    void stateNecroball();
    void dynamicCrackleAnim(uint8_t baseR, uint8_t baseG, uint8_t baseB, uint8_t baseW);
    void stateCommSolidWRGB();
    void stateBluetoothConnect();
    void stateBluetoothDisconnect();
    void stateSolidWRGB(uint8_t w, uint8_t r, uint8_t g, uint8_t b);

    void rgbToHsv(uint8_t r, uint8_t g, uint8_t b,
                  uint16_t& h, uint8_t& s, uint8_t& v);

    void hsvToRgb(uint16_t h, uint8_t s, uint8_t v,
                  uint8_t& r, uint8_t& g, uint8_t& b);

    float applyEasing(float t, int easingType);

    
public:
    PixelController(Adafruit_NeoPixel& pixelStrip);
    void begin();
    void update();
    void setState(State newState);
    State getCurrentState();
    void sparkIdle(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t heat, bool heatDesaturate, uint8_t num, EaseType easeType);

};

#endif