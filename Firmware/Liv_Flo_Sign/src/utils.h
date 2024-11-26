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
#include <customWiFiManager.h>
#include <customParameter.h>
#include <esp_task_wdt.h>
#include <vector>


class Timer
{
 public:
  Timer() {}
  void start(uint32_t timeout) { time = millis() + timeout; }
  bool expired() { return millis() > time; }

 private:
  uint32_t time = 0;
};


class Utils
{
 public:
  enum Country
  {
    Switzerland = 0,
    USA = 1,
    France = 2,
    Unknown = -1
  };

  static constexpr const float UTILS_UPDATE_RATE = 2.0;           // [Hz]  Interval to check for internet connection and time update
  static constexpr const float WIFI_RECONNECT_INTERVAL = 60.0;    // [s]  Interval to reconnect to WiFi
  static constexpr const float BUTTON_TIMER_RATE = 100.0;         // [Hz]  Timer rate for button press detection
  static constexpr const float BUTTON_LONG_PRESS_TIME = 5.0;      // [s]  Time to hold the button for a long press
  static constexpr const int TIMEZONE_UPDATE_INTERVAL = 60;       // [s]  Interval to update the time zone offset

  static CustomWiFiManager wm;

  Utils(int buttonPin) { this->buttonPin = buttonPin; }
  static bool begin(void);
  static String getResetReason() { return resetReason; }
  static uint32_t getUnixTime();                      // GMT+0000
  static bool getCurrentTime(struct tm& timeinfo);    // Local time
  static bool isDaylightSavingTime() { return dst_offset != 0; }
  static Country getCountry() { return country; }
  static bool getConnectionState() { return connectionState; }    // True if connected to WiFi
  static void resetSettings() { wm.resetSettings(); }
  static void resetWatchdog() { esp_task_wdt_reset(); }
  static bool getButtonShortPressEvent(bool clearFlag = true)
  {
    bool temp = shortPressEvent;
    if(clearFlag)
      shortPressEvent = false;
    return temp;
  }
  static bool getButtonLongPressEvent(bool clearFlag = true)
  {
    bool temp = longPressEvent;
    if(clearFlag)
      longPressEvent = false;
    return temp;
  }

 private:
  static const char* resetReasons[];
  static const char* serialNumber[];
  static String resetReason;
  static Country country;
  static int32_t raw_offset;
  static int32_t dst_offset;
  static bool timezoneValid;
  static bool tryReconnect;

  static bool connectionState;
  static int buttonPin;

  static hw_timer_t* Timer0_Cfg;
  static bool shortPressEvent;
  static bool longPressEvent;

  static ParameterSwitch switch_1;
  static ParameterSwitch switch_2;
  static ParameterSwitch switch_3;

  static WiFiManagerParameter regular_parameter;
  static CustomWiFiManagerParameter time_interval_slider;
  static CustomWiFiManagerParameter switch_parameter;

  static bool startWiFiManager();
  static bool getOffsetFromWorldTimeAPI();
  static bool getOffsetFromIpapi();
  static std::vector<IPAddress> getConnectedClientIPs(int maxCount = -1);

  static void saveParamsCallback();
  static bool reconnectWiFi(int retries = 1, bool verbose = false);
  static bool updateTimeZoneOffset();
  static void updateTask(void* pvParameter);
  static void timerISR(void);
};

#endif
