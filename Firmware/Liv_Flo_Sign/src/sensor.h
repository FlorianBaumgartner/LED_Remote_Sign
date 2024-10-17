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
  static constexpr const uint8_t SENSOR_UPDATE_RATE = 10;    // [Hz]

  Sensor() : vcnl4020() {}
  bool begin(void);


 private:
  Adafruit_VCNL4020 vcnl4020;

  uint16_t proxValue = 0;
  uint16_t ambientValue = 0;

  static void updateTask(void* pvParameter);
};


#endif
