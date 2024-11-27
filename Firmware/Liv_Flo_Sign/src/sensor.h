/******************************************************************************
 * file    sensor.h
 *******************************************************************************
 * brief   Handles the sensor data acquisition
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-16
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

#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VCNL4020.h"


class Sensor
{
 public:
  static constexpr const uint8_t SENSOR_UPDATE_RATE = 20;    // [Hz]
  static constexpr const float PROX_AVR_RATE = 0.05;         // Exponential moving average rate
  static constexpr const float AMB_AVR_RATE = 0.03;          // Exponential moving average rate
  static constexpr const int PROX_SNR_THRESHOLD = 50;        // Value must rise over this threshold compared to the average to trigger an event
  static constexpr const float PROX_BLANK_TIME = 1.5;        // [s]  Time to wait before the next event can be triggered
  static constexpr const uint16_t AMB_VALUE_MIN = 20;        // Goes down to 0 when really dark, consider values below 50 as fairly dark
  static constexpr const uint16_t AMB_VALUE_MAX = 30000;     // Not yet tested in direct sunlight, but goes up 65535 in full LED flashlight
  static constexpr const float AMB_POW_PARAM = 0.473;    // Values between 0.1...0.7 seem reasonable (lower values means brighter light in the dark)
  // Function: u = 255 * (x / AMB_VALUE_MAX)^AMB_POW_PARAM

  // A ambient value ~200 is in a slighyly dark room (evening, OK to work)
  // A ambient value ~70 is in a farily dark room (night, OK to read)
  // A ambient value ~35 is in a pretty dark room (night, OK to sleep)
  // A ambient value ~12 is in a very dark room (night, OK to sleep)

  Sensor() : vcnl4020() {}
  bool begin(void);
  bool getProxEvent(bool clear = true)
  {
    bool event = proxEvent;
    if(clear)
    {
      proxEvent = false;
    }
    return event;
  }

  uint8_t getAmbientBrightness(void);
  void enable(bool enable) { enabled = enable; }


 private:
  Adafruit_VCNL4020 vcnl4020;

  int proxValue = 0;
  int ambientValue = 0;
  int proxValueAvr = -1;
  int ambientValueAvr = -1;

  bool proxEvent = false;
  uint32_t proxEventTime = 0;
  bool enabled = true;

  static void updateTask(void* pvParameter);
};


#endif
