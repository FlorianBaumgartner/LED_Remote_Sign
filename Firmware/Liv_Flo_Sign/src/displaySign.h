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
  static constexpr const uint8_t DEFAULT_BRIGHNESS = 10;
  static constexpr const uint8_t MAX_BRIGHTNESS = 120;


  DisplaySign(uint8_t pin, int count) : pixels(count, pin, NEO_GRB + NEO_KHZ800) {}

  void begin(float updateRate = 30);
  void updateTask(void);
  void setBrightness(uint8_t brightness);
  void enable(bool en) { enabled = en; }
  bool getBootStatus() { return booting; }
  void setEvent(bool status) { event = status; }
  void setNightMode(bool status) { nightMode = status; }
  void setBootColor(uint32_t color) { bootColor = color; }
  void setNightLightColor(uint32_t color) { nightLightColor = color; }


 private:
  Adafruit_NeoPixel pixels;
  uint8_t updatePercentage = 0;
  float updateRate = 30;
  bool enabled = false;
  bool nightMode = false;
  bool booting = true;
  bool event = false;

  uint32_t bootColor = 0;
  uint32_t nightLightColor = 0;

  void animationBooting(void);
  void animationNightMode(uint32_t framecount, bool eventFlag);
  void animationSine(uint32_t framecount, bool eventFlag);


  static const float canvas_center[2];
  static const float square_coordinates[268][2];
  static const float canvas_min_max_x[2];
  static const int16_t cos_lut[360];  // Cosine lookup table for values between -1 and 1 scaled to an integer range [-1000, 1000]

  float mapf(float x, float in_min, float in_max, float out_min, float out_max)
  {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
};

#endif
