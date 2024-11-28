/******************************************************************************
 * file    displayMatrix.h
 *******************************************************************************
 * brief   Handles the LED Matrix Display
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-12
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

#ifndef DISPLAYMATRIX_H
#define DISPLAYMATRIX_H

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

class DisplayMatrix
{
 public:
  static constexpr const uint8_t DEFAULT_BRIGHNESS = 10;
  static constexpr const uint8_t MAX_BRIGHTNESS = 120;
  static constexpr const float TEXT_BLANK_SPACE_TIME = 0.5;    // [s]  Time to wait before scrolling the next message


  enum State
  {
    BOOTING,
    IDLE,
    DISCONNECTED,
    SHOW_IP,
    PORTAL_ACTIVE,
    UPDATING
  };

  DisplayMatrix(uint8_t pin, int matrixHeight = 7, int matrixWidth = 40)
      : matrix(matrixWidth, matrixHeight, pin, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE, NEO_GRB + NEO_KHZ800)
  {}

  void begin(float updateRate = 30);
  void updateTask(void);
  void setBrightness(uint8_t brightness) { matrix.setBrightness(brightness > MAX_BRIGHTNESS ? MAX_BRIGHTNESS : brightness); }
  void setState(State newState) { state = newState; }
  void setTextColor(uint32_t color) { textColor = color; }
  void setUpdatePercentage(int percentage);
  void setMessage(const String& msg) { newMessage = msg; }
  void setIpAdress(const String& ipAddr) { ipAddress = ipAddr; }


 private:
  Adafruit_NeoMatrix matrix;
  State state = BOOTING;
  int updatePercentage = 0;
  String currentMessage = "";
  String newMessage = "";
  String ipAddress = "";
  uint32_t textColor = 0xFFFFFF;
  int scrollPosition = matrix.width();
  int textWidth = 0;
  bool scrollTextNecessary = true;
  bool resetScrollPosition = false;
  float updateRate = 30;
  int messageScrollCount = 0;

  size_t printMessage(const String& msg, uint32_t color, int offset = 0);
  bool drawEmoji(int x, int y, uint32_t unicode_index);
  void scrollMessage(const String& msg, uint32_t color, int count = -1);
};

#endif
