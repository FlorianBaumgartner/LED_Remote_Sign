/******************************************************************************
 * file    displaySign.h
 *******************************************************************************
 * brief   Handles the LED Sign Display
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-20
 *******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022 Crelin - Florian Baumgartner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#ifndef DISPLAYSIGN_H
#define DISPLAYSIGN_H

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

class DisplaySign
{
 public:
  static constexpr const uint8_t MAX_BRIGHTNESS = 120;
  static constexpr const size_t EVENT_ANIMATION_DURATION = 10;    // [s]  Time to show event animation

  static const char* const ANIMATION_NAMES[];
  static const size_t ANIMATION_COUNT;


  DisplaySign(uint8_t pin, int count) : pixels(count, pin, NEO_GRB + NEO_KHZ800) {}

  void begin(float updateRate = 30);
  void updateTask(void);
  void setBrightness(uint8_t val) { brightness = constrain(val, 0, MAX_BRIGHTNESS); }
  void enable(bool en) { enabled = en; }
  bool getBootStatus() { return booting; }
  void setEvent(bool status) { eventTimestamp = status ? millis() + EVENT_ANIMATION_DURATION * 1000 : eventTimestamp; }
  void setMotionActivation(bool status) { motionActivation = status; }
  void setMotionEvent(bool status) { motionActiveTimestamp = status ? millis() + motionEventTime * 1000 : motionActiveTimestamp; }
  void setMotionEventTime(uint32_t time) { motionEventTime = time; }
  void setNewMessage(bool status) { newMessageFlag = status; }
  void setNightMode(bool status) { nightMode = status; }
  void setBootColor(uint32_t color) { bootColor = color; }
  void setNightLightColor(uint32_t color) { nightLightColor = color; }
  void setAnimationType(uint8_t type) { animationType = constrain(type, 0, ANIMATION_COUNT - 1); }
  void setAnimationPrimaryColor(uint32_t color) { animationPrimaryColor = color; }
  void setAnimationSecondaryColor(uint32_t color) { animationSecondaryColor = color; }


 private:
  Adafruit_NeoPixel pixels;
  uint8_t updatePercentage = 0;
  float updateRate = 30;
  uint8_t brightness = 0;
  bool enabled = false;
  bool nightMode = false;
  bool booting = true;
  bool motionActivation = false;
  bool newMessageFlag = false;
  uint32_t motionActiveTimestamp = 0;
  uint32_t motionEventTime = 0;
  uint32_t eventTimestamp = 0;

  uint32_t framecount = 0;
  uint8_t animationType = 0;
  uint32_t bootColor = 0;
  uint32_t nightLightColor = 0;
  uint32_t animationPrimaryColor = 0;
  uint32_t animationSecondaryColor = 0;

  void animationBooting(void);
  void animationNewMessage(uint32_t framecount, bool eventFlag);
  void animationOff(uint32_t framecount, bool eventFlag);
  void animationNightMode(uint32_t framecount, bool eventFlag);
  void animationWave(uint32_t framecount, bool eventFlag);
  void animationSprinkle(uint32_t framecount, bool eventFlag);
  void animationHeart(uint32_t framecount, bool eventFlag);


  static const float canvas_center[2];
  static const float square_coordinates[268][2];
  static const float canvas_min_max_x[2];
  static const int16_t cos_lut[360];    // Cosine lookup table for values between -1 and 1 scaled to an integer range [-1000, 1000]

  inline float fmap(float x, float in_min, float in_max, float out_min, float out_max)
  {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
  inline float fmodfNumpy(float a, float b) { return b == 0.0 ? NAN : fmodf(a, b) + (fmodf(a, b) < 0 != b < 0) * b; }
};

#endif
