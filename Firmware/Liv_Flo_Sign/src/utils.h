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
#include <Preferences.h>
#include <customParameter.h>
#include <customWiFiManager.h>
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

  static constexpr const char* SWITCH_NIGHT_LIGHT = "sw_nightLight";
  static constexpr const char* SWITCH_MOTION_ACTIVATED = "sw_motionAct";

  static CustomWiFiManager wm;
  static Preferences preferences;

  Utils(int buttonPin) { this->buttonPin = buttonPin; }
  static bool begin(void);
  static String getResetReason() { return resetReason; }
  static uint32_t getUnixTime();                      // GMT+0000
  static bool getCurrentTime(struct tm& timeinfo);    // Local time
  static bool isDaylightSavingTime() { return dst_offset != 0; }
  static Country getCountry() { return country; }
  static bool getConnectionState() { return connectionState; }    // True if connected to WiFi
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
  static void resetSettings()
  {
    wm.resetSettings();
    preferences.clear();
  }

  static bool getNightLight() { return pref_nightLight; }
  static bool getMotionActivated() { return pref_motionActivated; }

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

  static ParameterSwitch switch_nightLight;
  static ParameterSwitch switch_motionActivated;
  static CustomWiFiManagerParameter time_interval_slider;

  static bool pref_nightLight;
  static bool pref_motionActivated;

  static void loadPreferences();
  static bool startWiFiManager();
  static bool getOffsetFromWorldTimeAPI();
  static bool getOffsetFromIpapi();
  static std::vector<IPAddress> getConnectedClientIPs(int maxCount = -1);

  static void saveParamsCallback();
  static bool reconnectWiFi(int retries = 1, bool verbose = false);
  static bool updateTimeZoneOffset();
  static void updateTask(void* pvParameter);
  static void timerISR(void);

  static std::vector<const char*> menuItems;
  static constexpr const char* icon =
    "<link rel='icon' type='image/png' "
    "href='data:image/"
    "png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAADQElEQVRoQ+2YjW0VQQyE7Q6gAkgFkAogFUAqgFQAVACpAKiAUAFQAaECQgWECggVGH1PPrRvn3dv9/"
    "YkFOksoUhhfzwz9ngvKrc89JbnLxuA/63gpsCmwCADWwkNEji8fVNgotDM7osI/"
    "x777x5l9F6JyB8R4eeVql4P0y8yNsjM7KGIPBORp558T04A+CwiH1UVUItiUQmZ2XMReSEiAFgjAPBeVS96D+sCYGaUx4cFbLfmhSpnqnrZuqEJgJnd8cQplVLciAgX//"
    "Cf0ToIeOB9wpmloLQAwpnVmAXgdf6pwjpJIz+XNoeZQQZlODV9vhc1Tuf6owrAk/"
    "8qIhFbJH7eI3eEzsvydQEICqBEkZwiALfF70HyHPpqScPV5HFjeFu476SkRA0AzOfy4hYwstj2ZkDgaphE7m6XqnoS7Q0BOPs/"
    "sw0kDROzjdXcCMFCNwzIy0EcRcOvBACfh4k0wgOmBX4xjfmk4DKTS31hgNWIKBCI8gdzogTgjYjQWFMw+o9LzJoZ63GUmjWm2wGDc7EvDDOj/"
    "1IVMIyD9SUAL0WEhpriRlXv5je5S+U1i2N88zdPuoVkeB+ls4SyxCoP3kVm9jsjpEsBLoOBNC5U9SwpGdakFkviuFP1keblATkTENTYcxkzgxTKOI3jyDxqLkQT87pMA++"
    "H3XvJBYtsNbBN6vuXq5S737WqHkW1VgMQNXJ0RshMqbbT33sJ5kpHWymzcJjNTeJIymJZtSQd9NHQHS1vodoFoTMkfbJzpRnLzB2vi6BZAJxWaCr+62BC+"
    "jzAxVJb3dmmiLzLwZhZNPE5e880Suo2AZgB8e8idxherqUPnT3brBDTlPxO3Z66rVwIwySXugdNd+5ejhqp/"
    "+NmgIwGX3Py3QBmlEi54KlwmjkOytQ+iJrLJj23S4GkOeecg8G091no737qvRRdzE+"
    "HLALQoMTBbJgBsCj5RSWUlUVJiZ4SOljb05eLFWgoJ5oY6yTyJp62D39jDANoKKcSocPJD5dQYzlFAFZJflUArgTPZKZwLXAnHmerfJquUkKZEgyzqOb5TuDt1P3nwxobqwPocZA11m4A1mB"
    "x5IxNgRH21ti7KbAGiyNn3HoF/gJ0w05A8xclpwAAAABJRU5ErkJggg==' />";
};

#endif
