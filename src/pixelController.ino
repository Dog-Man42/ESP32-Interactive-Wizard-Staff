#include "pixelController.h"
#include "main.h"
#include <cstdlib>
#include <Arduino.h>
#include <cmath>
#include <algorithm>

PixelController::PixelController(Adafruit_NeoPixel& pixelStrip) 
  : pixels(pixelStrip), currentState(IDLE), prevMilli(0), deltaMilli(0), frame(0), crackleInitialized(false) {
  memset(pixelCrackle, 0, sizeof(pixelCrackle));
}

void PixelController::begin() {
  srand(millis());
  pixels.begin();
  crackleInitialized = false;
  currentState = IDLE;

}

void PixelController::setState(State newState) {
  frame = 0;
  prevMilli = millis();
  if( newState == BLUETOOTH_CONNECT || newState == BLUETOOTH_DISCONNECT) {
    returnState = currentState;
  }
  currentState = newState;
}

void PixelController::tickFrame() {
  frame += 1;
  prevMilli = millis();
  deltaMilli = 0;
}

State PixelController::getCurrentState() {
  return currentState;
}

void PixelController::update() {
  deltaMilli = millis() - prevMilli;
  
  switch (currentState) {
    case IDLE:
      stateIdle();
      break;
    case TEST_ONE:
      stateTestOne();
      break;
    case TEST_TWO:
      stateTestTwo();
      break;
    case FIREBALL:
      stateFireball();
      break;
    case NECROBALL:
      stateNecroball();
      break;
    case COMMAND_SOLID_WRGB:
      stateCommSolidWRGB();
      break;
    case BLUETOOTH_CONNECT:
      stateBluetoothConnect();
      break;
    case BLUETOOTH_DISCONNECT:
      stateBluetoothDisconnect();
      break;
  }
}

void PixelController::stateIdle() {

   dynamicCrackleAnim(40,20,60,10);

}

void PixelController::stateTestOne() {

  
  for (int i = 0; i < NUMPIXELS; i++) {

    float pixelPosition = (float)i + rainbowOffset;
    
    while (pixelPosition >= NUMPIXELS) {
      pixelPosition -= NUMPIXELS;
    }
    while (pixelPosition < 0) {
      pixelPosition += NUMPIXELS;
    }
    
    uint16_t hue = (uint16_t)((pixelPosition / NUMPIXELS) * 65536.0);
    
    uint32_t color = pixels.ColorHSV(hue, 255, 128);
    pixels.setPixelColor(i, color);
  }
  
  pixels.show();
  
  if (deltaMilli > 8) {
    rainbowOffset += 0.05;
    
    if (rainbowOffset >= NUMPIXELS) {
      rainbowOffset -= NUMPIXELS;
    }
    
    prevMilli = millis();
    deltaMilli = 0;
  }
}

void PixelController::stateTestTwo() {
  // Smooth rotating rainbow
  
  for (int i = 0; i < NUMPIXELS; i++) {

    float pixelPosition = (float)i + rainbowOffset;
    
    // Wrap around the ring smoothly
    while (pixelPosition >= NUMPIXELS) {
      pixelPosition -= NUMPIXELS;
    }
    while (pixelPosition < 0) {
      pixelPosition += NUMPIXELS;
    }
    
    uint16_t hue = (uint16_t)((pixelPosition / NUMPIXELS) * 65536.0);
    
    uint8_t brightness = 128 + (rand() % 128); 
    uint32_t color = pixels.ColorHSV(hue, 255, brightness);
    pixels.setPixelColor(i, color);
  }
  
  pixels.show();
  
  if (deltaMilli > 8) {

    rainbowOffset += 0.05; 
    
    if (rainbowOffset >= NUMPIXELS) {
      rainbowOffset -= NUMPIXELS;
    }
    
    prevMilli = millis();
    deltaMilli = 0;
  }
}

uint8_t heat[16];
void PixelController::stateFireball() {
  static bool initialized = false;
  if (!initialized) {
    memset(heat, 0, sizeof(heat));
    initialized = true;
  }
  
  if (deltaMilli > 30) {
    // Cool down every LED slightly
    for (int i = 0; i < NUMPIXELS; i++) {
      heat[i] = constrain(heat[i] - random(0, 10), 0, 255);
    }
    
    // Heat from neighboring LEDs with wrapping
    uint8_t tempHeat[NUMPIXELS];
    for (int i = 0; i < NUMPIXELS; i++) {
      int prev = (i - 1 + NUMPIXELS) % NUMPIXELS;
      int next = (i + 1) % NUMPIXELS;
      tempHeat[i] = (heat[prev] + heat[i] + heat[next]) / 3;
    }
    // Copy back
    for (int i = 0; i < NUMPIXELS; i++) {
      heat[i] = tempHeat[i];
    }
    
    if (random(100) < 30) {
      int pos = random(NUMPIXELS);
      heat[pos] = random(180, 255);
    }
    
    // Sometimes add a second spark
    if (random(100) < 15) {
      int pos = random(NUMPIXELS);
      heat[pos] = random(200, 255);
    }
    
    // Convert heat to colors
    for (int i = 0; i < NUMPIXELS; i++) {
      uint8_t h = heat[i];
      uint8_t white, red, green, blue;
      
      if (h < 85) {
        white = 0;
        red = h * 3;
        green = 0;
      } else if (h < 170) {
        white = (h - 85) * 2;
        red = 255;
        green = (h - 85) * 2;
      } else {
        white = 255;
        red = 255;
        green = 255;
      }
      
      pixels.setPixelColor(i, pixels.Color(red, green, blue, white));
    }
    
    pixels.show();
    tickFrame();
  }
  
  if (frame >= NUMPIXELS) {
    frame = 0;
  }
}

void PixelController::stateNecroball() {
  static bool initialized = false;
  if (!initialized) {
    memset(heat, 0, sizeof(heat));
    initialized = true;
  }
  
  if (deltaMilli > 30) {
    // Cool down every LED slightly
    for (int i = 0; i < NUMPIXELS; i++) {
      heat[i] = constrain(heat[i] - random(0, 10), 0, 255);
    }
    
    // Heat from neighboring LEDs with wrapping
    uint8_t tempHeat[NUMPIXELS];
    for (int i = 0; i < NUMPIXELS; i++) {
      int prev = (i - 1 + NUMPIXELS) % NUMPIXELS;
      int next = (i + 1) % NUMPIXELS;
      tempHeat[i] = (heat[prev] + heat[i] + heat[next]) / 3;
    }
    // Copy back
    for (int i = 0; i < NUMPIXELS; i++) {
      heat[i] = tempHeat[i];
    }
    
    // Randomly ignite new sparks
    if (random(100) < 30) {
      int pos = random(NUMPIXELS);
      heat[pos] = random(180, 255);
    }
    
    // Sometimes add a second spark
    if (random(100) < 15) {
      int pos = random(NUMPIXELS);
      heat[pos] = random(200, 255);
    }
    
    // Convert heat to colors
    for (int i = 0; i < NUMPIXELS; i++) {
      uint8_t h = heat[i];
      uint8_t white, red, green, blue;
      
      if (h < 85) {
        white = 0;
        blue = h;
        green = h * 3;
      } else if (h < 170) {
        white = (h - 85) * 2;
        green = 255;
        blue = (h - 85) * 2;
        red = (h - 85);
      } else {
        white = 255;
        red = 100;
        green = 255;
        blue = 255;
      }
      
      pixels.setPixelColor(i, pixels.Color(red, green, blue, white));
    }
    
    pixels.show();
    tickFrame();
  }
  
  if (frame >= NUMPIXELS) {
    frame = 0;
  }
}

void PixelController::dynamicCrackleAnim(uint8_t baseR, uint8_t baseG, uint8_t baseB, uint8_t baseW){
  if (deltaMilli > 16) {
  
    // Cool down every LED slightly
    for (int i = 0; i < NUMPIXELS; i++) {
      int newHeat = pixelCrackle[i].heat - random(0, 5);
      pixelCrackle[i].heat = constrain(newHeat, 0, 255);
    }

    // Heat from neighboring LEDs
    uint8_t tempHeat[NUMPIXELS];
    for (int i = 0; i < NUMPIXELS; i++) {
      int prev = (i - 1 + NUMPIXELS) % NUMPIXELS;
      int next = (i + 1) % NUMPIXELS;
      tempHeat[i] = (pixelCrackle[prev].heat + pixelCrackle[i].heat + pixelCrackle[next].heat) / 3;
    }
    
    // Copy back
    for (int i = 0; i < NUMPIXELS; i++) {
      pixelCrackle[i].heat = tempHeat[i];
    }

    for(int i = 0; i < NUMPIXELS; i++){
      uint8_t h = pixelCrackle[i].heat;
      float hRatio = h / 255.0f;

      uint8_t redOut, greenOut, blueOut, whiteOut;
      
      uint16_t baseHue;
      uint8_t baseSat;
      uint8_t baseVal;

      uint16_t pixHue;
      uint8_t pixSat;
      uint8_t pixVal;

      rgbToHsv(baseR, baseG, baseB, baseHue, baseSat, baseVal);
      rgbToHsv(pixelCrackle[i].red, pixelCrackle[i].green, pixelCrackle[i].blue, pixHue, pixSat, pixVal);

      if(pixelCrackle[i].heat > 0){
        float adjusted = applyEasing(hRatio, pixelCrackle[i].easeType);

        uint16_t lerpHue;
        int hueDiff = pixHue - baseHue;

        //[-180, 180]
        if (hueDiff > 180) {
          hueDiff -= 360;
        } else if (hueDiff < -180) {
          hueDiff += 360;
        }
        lerpHue = baseHue + static_cast<int16_t>(hueDiff * hRatio);
        
        // Wrap around to [0, 360)
        if (lerpHue < 0) {
          lerpHue += 360;
        } else if (lerpHue >= 360) {
          lerpHue -= 360;
        }

        uint8_t lerpSat = static_cast<uint8_t>(baseSat * (1.0f - hRatio) + pixSat * hRatio);
        uint8_t lerpVal = static_cast<uint8_t>(baseVal * (1.0f - hRatio) + pixVal * hRatio);
        hsvToRgb(lerpHue, lerpSat, lerpVal, redOut, greenOut, blueOut);
        whiteOut = static_cast<uint8_t>(baseW * (1.0f - hRatio) + pixelCrackle[i].white * hRatio);
        pixels.setPixelColor(i, redOut, greenOut, blueOut, whiteOut);
      } else {
        pixels.setPixelColor(i, baseR, baseG, baseB, baseW);
      }    
    }
    
    tickFrame();
  }
  pixels.show();
}

void PixelController::sparkIdle(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t heat, bool heatDesaturate, uint8_t num, EaseType ease){
  if (num == 0) return; 
  for(int i = 0; i < num; i++){
    int pos = random(NUMPIXELS);
    pixelCrackle[pos].heat += heat;
    pixelCrackle[pos].heatSaturation = heatDesaturate;
    pixelCrackle[pos].red = r;
    pixelCrackle[pos].green = g;
    pixelCrackle[pos].blue = b;
    pixelCrackle[pos].white = w;
    pixelCrackle[pos].easeType = ease;
  }

}


float PixelController::applyEasing(float t, int easingType) {
    switch(easingType) {
        case 0: // Linear (original)
            return t;
        
        case 1: // Ease-in quadratic
            return t * t;
        
        case 2: // Ease-out quadratic (RECOMMENDED)
            return t * (2.0f - t);
        
        case 3: // Ease-out cubic (stronger)
            { float t1 = t - 1.0f;
            return t1 * t1 * t1 + 1.0f; }
        
        case 4: // Smoothstep
            return t * t * (3.0f - 2.0f * t);
        
        case 5: // Square root
            return pow(t, 0.5f);  
        
        case 6: // Exponential ease-out
            return (t == 1.0f) ? 1.0f : 1.0f - pow(2.0f, -10.0f * t);
        
        default:
            return t;
    }
}

void PixelController::stateCommSolidWRGB() {
  pixels.fill(pixels.Color(commandArgs[1], commandArgs[2], commandArgs[3], commandArgs[0]));
  
  pixels.show();
}

void PixelController::stateBluetoothConnect() {

  int8_t brightMod = sin(frame / 15.0) * 64;
  pixels.fill(pixels.Color(0, 80 + brightMod, 128 + brightMod, 0));
  pixels.show();
  printSerials("Brightness = " + String(brightMod));
  printSerials("Frame = " + String(frame));
  printSerials("Return State = " + String(returnState));

  if (deltaMilli > 16) {
    tickFrame();
  }
  
  if (frame > 60 * 2) {
    setState(returnState);
  }

}

void PixelController::stateBluetoothDisconnect() {
  printSerials("Disconnect State Frame = " + String(frame));
  int8_t brightMod = tan(frame / 15.0) * 64;
  pixels.fill(pixels.Color(0, 80 + brightMod, 128 + brightMod, 0));
  pixels.show();

  if (deltaMilli > 16) {
    tickFrame();
  }
  
  if (frame > 60 * 2) {
    setState(returnState);
  }
  
}

void PixelController::rgbToHsv(uint8_t r, uint8_t g, uint8_t b, uint16_t& h, uint8_t& s, uint8_t& v){
  float rf = r / 255.0f;
  float gf = g / 255.0f;
  float bf = b / 255.0f;
  
  float max = std::max({rf, gf, bf});
  float min = std::min({rf,gf,bf});
  float delta = max - min;

  v = static_cast<uint8_t>(max * 255.0f);

  if (max == 0.0f) {
    s = 0;
  } else {
    s = static_cast<uint8_t>((delta / max) * 255.0f);
  }

  if(delta == 0.0f){
    h = 0;
  } else {
    float hue;
    if(max == rf) {
      hue = 60.0f * (fmod((gf - bf) / delta, 6.0f));
    } else if (max == gf){
      hue = 60.0f * (((bf - rf) / delta) + 2.0f);
    } else {
      hue = 60.0f * (((rf - gf) / delta) + 4.0f);
    }
    if (hue < 0.0f) {
      hue += 360.0f;
    }

    h = static_cast<uint16_t>(hue);
  }
}

void PixelController::hsvToRgb(uint16_t h, uint8_t s, uint8_t v,
              uint8_t& r, uint8_t& g, uint8_t& b) {
    // Normalize values
    float hf = h % 360;
    float sf = s / 255.0f;
    float vf = v / 255.0f;
    
    float c = vf * sf;
    float x = c * (1.0f - fabsf(fmodf(hf / 60.0f, 2.0f) - 1.0f));
    float m = vf - c;
    
    float rf, gf, bf;
    
    if (hf < 60.0f) {
        rf = c; gf = x; bf = 0;
    } else if (hf < 120.0f) {
        rf = x; gf = c; bf = 0;
    } else if (hf < 180.0f) {
        rf = 0; gf = c; bf = x;
    } else if (hf < 240.0f) {
        rf = 0; gf = x; bf = c;
    } else if (hf < 300.0f) {
        rf = x; gf = 0; bf = c;
    } else {
        rf = c; gf = 0; bf = x;
    }
    
    r = static_cast<uint8_t>((rf + m) * 255.0f);
    g = static_cast<uint8_t>((gf + m) * 255.0f);
    b = static_cast<uint8_t>((bf + m) * 255.0f);
}