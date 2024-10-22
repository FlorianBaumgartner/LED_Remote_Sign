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
  void setBrightness(uint8_t brightness) { pixels.setBrightness(brightness > MAX_BRIGHTNESS ? MAX_BRIGHTNESS : brightness); }
  void enable(bool enable);


 private:
  Adafruit_NeoPixel pixels;
  uint8_t updatePercentage = 0;
  float updateRate = 30;
  bool enabled = true;
};

#endif
