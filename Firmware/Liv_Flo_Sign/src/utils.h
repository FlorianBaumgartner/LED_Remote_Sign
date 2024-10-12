
/******************************************************************************
 * file    utils.h
 *******************************************************************************
 * brief   Utility Functions
 *******************************************************************************
 * author  Florian Baumgartner
 * version 1.0
 * date    2024-10-04
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

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <WiFiManager.h>
#include <esp_task_wdt.h>

class Utils
{
 public:
  enum Country
  {
    Switzerland = 0,
    USA = 1,
    Unknown = 2
  };

  static constexpr const char* WIFI_STA_SSID = "Liv Flo Sign";
  static constexpr const float UTILS_UPDATE_RATE = 2.0;    // [Hz]  Interval to check for internet connection and time update
  static const char* resetReasons[];

  static bool begin(void);
  static uint32_t getUnixTime();                      // GMT+0000
  static bool getCurrentTime(struct tm& timeinfo);    // Local time
  static bool isDaylightSavingTime() { return dst_offset != 0; }
  static Country getCountry() { return country; }
  static bool getConnectionState() { return connectionState; }
  static void resetSettings() { wm.resetSettings(); }
  static void resetWatchdog() { esp_task_wdt_reset(); }

 private:
  static WiFiManager wm;

  static Country country;
  static int32_t raw_offset;
  static int32_t dst_offset;

  static bool connectionState;

  static bool updateTimeZoneOffset();
  static void updateTask(void* pvParameter);
};

#endif
